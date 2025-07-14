/****************************************************************************
 * Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#include "data_export.h"

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "data_export_private.h"
#include "data_export_types.h"
#include "dtdl_model/properties.h"
#include "log.h"
#include "map.hpp"
#include "memory_manager.hpp"
#include "sdk.h"
#include "sm_api.hpp"
#include "sm_types.h"

static Context *context_;
static int registered_send_data_callback = 0;

struct EVP_client *evp_client_;
static const char *g_placeholder_telemetry_key = "placeholder";
static EVP_BLOB_IO_RESULT blob_io_cb(void *buf, size_t buflen, void *userData) {
  module_vars_t *module_vars = (module_vars_t *)userData;

  if ((module_vars == nullptr) || (buf == nullptr)) {
    LOG_ERR("Blob operation: module_vars is null. data might be corrupted");
    return EVP_BLOB_IO_RESULT_ERROR;
  }

  if (module_vars->identifier != 0x12345678) {
    LOG_ERR("Blob operation: data might be corrupted");
    return EVP_BLOB_IO_RESULT_ERROR;
  }

  if (module_vars->blob_buff_offset > module_vars->blob_buff_size) {
    LOG_ERR("Blob operation: offset information is corrupted");
    return EVP_BLOB_IO_RESULT_ERROR;
  }

  memcpy(buf, module_vars->blob_buff + module_vars->blob_buff_offset, buflen);
  module_vars->blob_buff_offset += buflen;

  LOG_DBG(
      "Sending--> buf: %p, send block of %zu, accumulated %d, "
      "total %d\n",
      buf, buflen, module_vars->blob_buff_offset, module_vars->blob_buff_size);
  return EVP_BLOB_IO_RESULT_SUCCESS;
}

static EdgeAppLibDataExportFuture *InitializeFuture() {
  EdgeAppLibDataExportFuture *future =
      (EdgeAppLibDataExportFuture *)xmalloc(sizeof(EdgeAppLibDataExportFuture));
  if (future == NULL) {
    LOG_ERR("Error when performing malloc for the future.");
    return NULL;
  }
  future->result = EdgeAppLibDataExportResultUninitialized;
  future->cond = PTHREAD_COND_INITIALIZER;
  future->mutex = PTHREAD_MUTEX_INITIALIZER;
  future->is_processed = false;
  future->is_cleanup_requested = false;
  return future;
}

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Deletes a future if the callback of an EVP operation and
 * EdgeAppLibDataExportCleanup has been called.
 *
 * @param future parameter to cleanup. Assumption: future is locked.
 */
static void DataExportCleanupOrUnlock(EdgeAppLibDataExportFuture *future) {
  if (future->is_processed && future->is_cleanup_requested) {
    LOG_DBG("Deleting future: callback and user requested.");
    pthread_mutex_unlock(&future->mutex);
    if (future->is_cleanup_sent_data && future->module_vars.blob_buff) {
      free(future->module_vars.blob_buff);
      future->module_vars.blob_buff = nullptr;
    }
    pthread_mutex_destroy(&future->mutex);
    pthread_cond_destroy(&future->cond);
    free(future);
    return;
  }
  LOG_DBG("Keeping future: callback (%d) and user (%d).", future->is_processed,
          future->is_cleanup_requested);
  pthread_mutex_unlock(&future->mutex);
}

/**
 * @brief Cleans up the data buffer
 *
 * @return
 */
static void DataExportSendDataDoneCallback(EVP_BLOB_CALLBACK_REASON reason,
                                           const void *vp, void *userData) {
  const struct EVP_BlobResultEvp *result;

  module_vars_t *module_vars = (module_vars_t *)userData;
  LOG_TRACE("Entering DataExportSendDataDoneCallback");
  EdgeAppLibDataExportFuture *future =
      (EdgeAppLibDataExportFuture *)map_pop(module_vars);
  if (future == nullptr) {
    LOG_ERR(
        "State might be corrupted. SendData called but buffer not in "
        "map.");
    return;
  }
  pthread_mutex_lock(&future->mutex);
  future->is_processed = true;

  switch (reason) {
    case EVP_BLOB_CALLBACK_REASON_DONE:
      future->result = EdgeAppLibDataExportResultSuccess;
      result = (EVP_BlobResultEvp *)vp;
      LOG_DBG(
          "EVP_BLOB_CALLBACK_REASON_DONE result=%u "
          "http_status=%u error=%d\n",
          result->result, result->http_status, result->error);

      break;
    case EVP_BLOB_CALLBACK_REASON_EXIT:
      future->result = EdgeAppLibDataExportResultDenied;
      LOG_DBG("EVP_BLOB_CALLBACK_REASON_EXIT\n");

      break;
    default:
      future->result = EdgeAppLibDataExportResultFailure;
      LOG_CRITICAL(
          "The result of BlobOperation didn't match any "
          "EVP_BLOB_CALLBACK_REASON.");
  }

  /* After blob operation free memory used to pass url to request. It has
   * to be handled here to avoid conifg_cb call free memory while sdk is
   * using it. SDK can call config_cb at any moment (inclusive with any
   * change in configuration) */
  // free(cb_data->blob_url);//url is saved in stack.
  pthread_cond_signal(&future->cond);
  DataExportCleanupOrUnlock(future);
}

/**
 * @brief Cleans up the data buffer
 *
 * @return
 */
static void DataExportTelemetryDoneCallback(
    EVP_TELEMETRY_CALLBACK_REASON reason, void *userData) {
  module_vars_t *module_vars = (module_vars_t *)userData;
  EdgeAppLibDataExportFuture *future =
      (EdgeAppLibDataExportFuture *)map_pop(module_vars);
  if (future == nullptr) {
    LOG_ERR(
        "State might be corrupted. SendData called but buffer not in "
        "map.");
    return;
  }
  pthread_mutex_lock(&future->mutex);
  future->is_processed = true;
  switch (reason) {
    case EVP_TELEMETRY_CALLBACK_REASON_SENT:
      future->result = EdgeAppLibDataExportResultSuccess;
      LOG_INFO("EVP_TELEMETRY_CALLBACK_REASON_DONE\n");
      break;
    case EVP_TELEMETRY_CALLBACK_REASON_DENIED:
    case EVP_TELEMETRY_CALLBACK_REASON_EXIT:
    case EVP_TELEMETRY_CALLBACK_REASON_ERROR:
      future->result = EdgeAppLibDataExportResultDenied;
      LOG_INFO("EVP_TELEMETRY_CALLBACK_REASON_EXIT\n");
      break;
    default:
      future->result = EdgeAppLibDataExportResultFailure;
      LOG_CRITICAL(
          "The result of SendTelemetry didn't match any "
          "EVP_TELEMETRY_CALLBACK_REASON.");
  }
  pthread_cond_signal(&future->cond);
  DataExportCleanupOrUnlock(future);
}

EdgeAppLibDataExportResult DataExportInitialize(Context *context,
                                                void *evp_client) {
  context_ = context;

  evp_client_ = (EVP_client *)evp_client;

  return EdgeAppLibDataExportResultSuccess;
}

EdgeAppLibDataExportResult DataExportUnInitialize() {
  return EdgeAppLibDataExportResultSuccess;
}

EdgeAppLibDataExportFuture *DataExportSendData(
    char *portname, EdgeAppLibDataExportDataType datatype, void *data,
    int datalen, uint64_t timestamp, uint32_t current, uint32_t division) {
  LOG_TRACE("Entering SendData");

  if (!DataExportIsEnabled(datatype)) {
    return nullptr;
  }

  EdgeAppLibDataExportFuture *future = InitializeFuture();
  if (future == nullptr) {
    return nullptr;
  }

  // Release sent data inside DataExportCleanupOrUnlock
  future->is_cleanup_sent_data = (datatype != EdgeAppLibDataExportMetadata);

  if (map_set((void *)&(future->module_vars), future) == -1) {
    // TODO: add more meaningful result
    LOG_ERR("map_set failed");
    future->result = EdgeAppLibDataExportResultDenied;
    future->is_processed = true;
    /* Set buff for cleanup */
    future->module_vars.blob_buff = (char *)data;
    return future;
  }

  future->result = EdgeAppLibDataExportResultEnqueued;
  LOG_DBG("Sending data %p, %d", data, datalen);
  METHOD sendMethod;
  JSON_Object *object = getPortSettings();
  JSON_Object *port_setting = NULL;
  const char *port_setting_key =
      datatype == EdgeAppLibDataExportRaw ? "input_tensor" : "metadata";
  if (object && json_object_has_value(object, port_setting_key)) {
    port_setting = json_object_get_object(object, port_setting_key);
    if (port_setting && json_object_has_value(port_setting, "method")) {
      sendMethod = (METHOD)json_object_get_number(port_setting, "method");
    }
  }
  future->module_vars.localStore.filename = NULL;
  future->module_vars.blob_buff_offset = 0;
  future->module_vars.blob_buff_size = datalen;
  future->module_vars.blob_buff = (char *)data;
  future->module_vars.localStore.io_cb = blob_io_cb;
  future->module_vars.localStore.blob_len = future->module_vars.blob_buff_size;
  future->module_vars.identifier = 0x12345678;

  EVP_RESULT result = EVP_OK;
  if (sendMethod == METHOD_HTTP_STORAGE || sendMethod == METHOD_BLOB_STORAGE) {
    /* Data used for blob_cb to share instance context and current url
     * used, and free configuration after use it */
    char filename[32] = {0};
    char filename_extension[10] = {0};
    const char *path = NULL;
    if (json_object_has_value(port_setting, "path")) {
      path = json_object_get_string(port_setting, "path");
    }
    DataExportFileSuffix(filename_extension, sizeof(filename_extension),
                         datatype);
    DataExportFormatTimestamp(filename, sizeof(filename), timestamp);

    /*
     * Append current and division info to the file name based on subframe data:
     * - If current and division are both 0, no valid input tensor.
     *   Example: current/division=0/0 -> data has no timestamp, size=0 bytes.
     *
     * - If current = 1, no data exists for the input tensor.
     *   (e.g., only metadata is present.)
     *
     * - For current >= 2 and division > 1, valid input tensor is processed.
     *   "_<current>_of_<division>" is appended to the file name.
     *   Examples:
     *   - current/division=2/5: 20250117095712459_2_of_5.bin, size=2092872.
     *   - current/division=3/5: 20250117095712459_3_of_5.bin, size=2092872.
     *   - current/division=4/5: 20250117095712459_4_of_5.bin, size=2092872.
     *   - current/division=5/5: 20250117095712459_5_of_5.bin, size=716328.
     */
    if (current >= 2 && division > 1) {
      char temp[64];
      snprintf(temp, sizeof(temp), "_%u_of_%u", current, division);
      strncat(filename, temp, strlen(temp));
    }

    strncat(filename, filename_extension, strlen(filename_extension));
    void *request;
    EVP_BLOB_TYPE blob_type;
    if (sendMethod == METHOD_HTTP_STORAGE) {
      char url[420] = {0};
      const char *endpoint = NULL;
      if (port_setting && json_object_has_value(port_setting, "endpoint")) {
        endpoint = json_object_get_string(port_setting, "endpoint");
      }
      snprintf(url, sizeof(url), "%s/%s/%s", endpoint, path, filename);
      struct EVP_BlobRequestHttp request;
      request.url = url;
      result =
          EVP_blobOperation(evp_client_, EVP_BLOB_TYPE_HTTP, EVP_BLOB_OP_PUT,
                            &request, &future->module_vars.localStore,
                            (EVP_BLOB_CALLBACK)DataExportSendDataDoneCallback,
                            &future->module_vars);
    } else {
      struct EVP_BlobRequestEvpExt ext_request;
      char blob_path[420] = {0};
      const char *storage_name = NULL;
      snprintf(blob_path, sizeof(blob_path), "%s/%s", path, filename);
      if (port_setting && json_object_has_value(port_setting, "storage_name")) {
        storage_name = json_object_get_string(port_setting, "storage_name");
      }
      ext_request.remote_name = blob_path;
      ext_request.storage_name = storage_name;
      result =
          EVP_blobOperation(evp_client_, EVP_BLOB_TYPE_EVP_EXT, EVP_BLOB_OP_PUT,
                            &ext_request, &future->module_vars.localStore,
                            (EVP_BLOB_CALLBACK)DataExportSendDataDoneCallback,
                            &future->module_vars);
    }
    if (result != EVP_OK) {
      LOG_ERR("EVP_blobOperation: result=%d", result);
    }
  } else if (sendMethod == METHOD_MQTT) {
    /* Inference Result telemetry send entry info */
    struct EVP_telemetry_entry telemetry_entry = {
        .key = g_placeholder_telemetry_key, .value = (char *)data};
    result = EVP_sendTelemetry(
        evp_client_, &telemetry_entry, 1,
        (EVP_TELEMETRY_CALLBACK)DataExportTelemetryDoneCallback,
        &future->module_vars);

    if (result != EVP_OK) {
      LOG_ERR("EVP_sendTelemetry: result=%d", result);
    }
  } else {
    result = EVP_INVAL;
    const char *error_msg = "An invalid argument was specified.";
    LOG_ERR("%s", error_msg);
    char *config_error = nullptr;
    asprintf(&config_error,
             "{\"res_info\": {\"res_id\":\"%s\",\"code\": "
             "%d,\"detail_msg\":\"%s\"}}",
             "", ResponseCodeInvalidArgument, error_msg);
    DataExportSendState("custom_settings", config_error, strlen(config_error));
  }
  if (result != EVP_OK) {
    future->is_processed = true;
    future->result = EdgeAppLibDataExportResultFailure;
    map_pop((void *)&(future->module_vars));
  }

  LOG_TRACE("Exit SendData");
  return future;
}

EdgeAppLibDataExportResult DataExportSendState(const char *topic, void *state,
                                               int statelen) {
  LOG_TRACE("Entering SendState");

  updateCustomSettings(state, statelen);
  free(state);
  LOG_TRACE("Exit SendState");
  return EdgeAppLibDataExportResultSuccess;
}

EdgeAppLibDataExportResult DataExportAwait(EdgeAppLibDataExportFuture *future,
                                           int timeout_ms) {
  pthread_mutex_lock(&future->mutex);

  LOG_TRACE("EdgeAppLibDataExportAwait waiting for signal");
  EdgeAppLibDataExportResult output;
  if (future->result == EdgeAppLibDataExportResultEnqueued) {
    int res = 0;
    if (timeout_ms >= 0) {
      LOG_WARN("Replaced the timeout_ms to -1 to disable the timeout");
      timeout_ms = -1;
    }
    if (timeout_ms < 0) {
      LOG_DBG("pthread_cond_wait");
      do {
        res = pthread_cond_wait(&future->cond, &future->mutex);
      } while (res == 0 &&
               future->result == EdgeAppLibDataExportResultEnqueued);
    } else {
      struct timeval now;
      gettimeofday(&now, NULL);
      struct timespec time_to_wait;
      time_to_wait.tv_sec = now.tv_sec + timeout_ms / 1000;
      time_to_wait.tv_nsec = (now.tv_usec + (timeout_ms % 1000) * 1000) * 1000;
      time_to_wait.tv_sec += time_to_wait.tv_nsec / 1000000000;
      time_to_wait.tv_nsec %= 1000000000;
      LOG_DBG("pthread_cond_timedwait");
      do {
        res = pthread_cond_timedwait(&future->cond, &future->mutex,
                                     &time_to_wait);
      } while (res == 0 &&
               future->result == EdgeAppLibDataExportResultEnqueued);
    }

    LOG_INFO("Result of conditional wait: %d", res);
    output = EdgeAppLibDataExportResultFailure;
    if (res == 0)
      output = EdgeAppLibDataExportResultSuccess;
    else if (res == ETIMEDOUT)
      output = EdgeAppLibDataExportResultTimeout;
  } else {
    output = future->result;
  }
  LOG_TRACE("EdgeAppLibDataExportAwait stop waiting");
  pthread_mutex_unlock(&future->mutex);
  return output;
}

EdgeAppLibDataExportResult DataExportCleanup(
    EdgeAppLibDataExportFuture *future) {
  LOG_INFO("Cleaning up things");
  pthread_mutex_lock(&future->mutex);
  future->is_cleanup_requested = true;
  DataExportCleanupOrUnlock(future);
  LOG_INFO("Exit Clean");
  return EdgeAppLibDataExportResultSuccess;
}

EdgeAppLibDataExportResult DataExportStopSelf() {
  LOG_INFO("Setting stat to Idle");
  context_->SetNextState(STATE_IDLE);
  context_->EnableNotification();
  return EdgeAppLibDataExportResultSuccess;
}

bool DataExportHasPendingOperations() { return not map_is_empty(); }

bool DataExportIsEnabled(EdgeAppLibDataExportDataType datatype) {
  const char *port_setting_key =
      datatype == EdgeAppLibDataExportRaw ? "input_tensor" : "metadata";
  JSON_Object *port_setting =
      json_object_get_object(getPortSettings(), port_setting_key);
  bool enabled = json_object_get_boolean(port_setting, "enabled") == 1;
  return enabled;
}

JSON_Object *DataExportGetPortSettings(void) { return getPortSettings(); }

void DataExportFormatTimestamp(char *buffer, size_t buffer_size,
                               uint64_t timestamp) {
  // convert nanoseconds to milliseconds
  uint64_t timestamp_ms = timestamp / 1000000;
  // ...and to seconds, since time_t is in seconds
  time_t timestamp_sec = timestamp_ms / 1000;
  struct tm tm;
  gmtime_r(&timestamp_sec, &tm);

  int remaining_ms = timestamp_ms % 1000;
  int num = strftime(buffer, buffer_size, "%Y%m%d%H%M%S", &tm);
  if (num > 0) {
    snprintf(buffer + num, buffer_size - num, "%03d", remaining_ms);
  }
}

void DataExportFileSuffix(char *buffer, size_t buffer_size,
                          EdgeAppLibDataExportDataType datatype) {
  const char *extension = NULL;
  if (datatype == EdgeAppLibDataExportRaw) {
    JSON_Object *json_object = getCodecSettings();
    int codec_number = json_object_get_number(json_object, "format");
    switch (codec_number) {
      case 0:
        extension = "bin";
        break;
      case 1:
        extension = "jpg";
        break;
      case 2:
        extension = "bmp";
        break;
      default:
        extension = NULL;
        break;
    }
  } else if (datatype == EdgeAppLibDataExportMetadata) {
    extension = "txt";
  } else {
    extension = "bmp";
  }
  if (extension) {
    snprintf(buffer, buffer_size, ".%s", extension);
  }
}

#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib
