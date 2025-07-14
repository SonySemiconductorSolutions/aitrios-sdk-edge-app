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

#include "py_data_export.hpp"

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <string>
#include <thread>

#include "data_export.h"
#include "data_export_private.h"
#include "dtdl_model/properties.h"
#include "log.h"
#include "parson.h"
#include "py_shared_state.hpp"
#include "send_data.h"
#include "sensor.h"
#include "sm_context.hpp"

#define DATA_EXPORT_AWAIT_TIMEOUT -1

static const char *g_placeholder_telemetry_key = "placeholder";

using namespace EdgeAppLib;

class MutexLocker {
 public:
  explicit MutexLocker(pthread_mutex_t *mutex) : mutex_(mutex) {
    pthread_mutex_lock(mutex_);
  }
  ~MutexLocker() { pthread_mutex_unlock(mutex_); }

 private:
  pthread_mutex_t *mutex_;
};

/**
 * @brief Sends the Input Tensor to the cloud synchronously.
 *
 * This function sends the input tensor data from the provided frame to the
 * cloud.
 *
 * @param frame Pointer to the current sensor frame.
 */
void sendInputTensorSync(EdgeAppLibSensorFrame *frame) {
  LOG_TRACE("Inside sendInputTensor.");

  EdgeAppLibSensorChannel channel = 0;
  int32_t ret = -1;
  if ((ret = SensorFrameGetChannelFromChannelId(
           *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE,
           &channel)) != 0) {
    LOG_WARN(
        "EdgeAppLibSensorFrameGetChannelFromChannelId : ret=%d. Skipping "
        "sending "
        "input tensor.",
        ret);
    return;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "EdgeAppLibSensorChannelGetRawData : ret=%d. Skipping sending input "
        "tensor.",
        ret);
    return;
  }

  /*
   * Handle subframe properties for large input tensors:
   * - When the input tensor is large, it is divided into subframes.
   * - Retrieve subframe properties using GetProperty.
   */
  struct EdgeAppLibSensorSubFrameProperty subframe = {0};
  if ((ret = SensorChannelGetProperty(
           channel, AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY, &subframe,
           sizeof(struct EdgeAppLibSensorSubFrameProperty))) < 0) {
    LOG_WARN("SensorChannelGetProperty - SubFrame: ret=%d", ret);
  } else {
    LOG_INFO("SensorChannelGetProperty - SubFrame: current=%d, division=%d",
             subframe.current_num, subframe.division_num);
    if (subframe.current_num == 0 && subframe.division_num == 0) {
      /*
       * If both current and division are 0, the data is invalid:
       * - No timestamp is associated.
       * - Data size is 0 bytes.
       */
      return;
    } else {
      /*
       * Includes current_num and division_num for processing valid subframes.
       */
      // EdgeAppLibDataExportFuture *future =
      //     DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
      //                        data.address, data.size, data.timestamp,
      //                        subframe.current_num, subframe.division_num);
      // DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
      // DataExportCleanup(future);
      return;
    }
  }

  // TODO: got segmentation fault here when running from RaspberryPi
  // EdgeAppLibDataExportFuture *future =
  //     DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
  //                        data.address, data.size, data.timestamp);
  // DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
  // DataExportCleanup(future);

  // TODO: since we are using mock EVP, EVP_blobOperation doesn't send the data
  // anywhere, so write it to a file here
  struct stat st;
  if (stat("./images", &st) == -1) {
    mkdir("./images", 0700);
  }
  char fn[512] = {0};
  snprintf(fn, sizeof(fn), "./images/%zu.jpg", (size_t)data.timestamp);
  FILE *f = fopen(fn, "wb");
  fwrite(data.address, data.size, 1, f);
  fclose(f);
}

/**
 * @brief Sends the Metadata to the cloud synchronously.
 *
 * This function sends the post-processed output tensor (metadata) from the
 * provided sensor frame to the cloud.
 *
 * @param frame Pointer to the current sensor frame.
 */
void sendMetadata(EdgeAppLibSensorFrame *frame) {
  LOG_TRACE("Inside sendMetadata.");

  // TODO: when using a webcam stream, this channel ID actually maps to the
  // image channel
  //       so sendMetadata is actually sending the image
  EdgeAppLibSensorChannel channel;
  int32_t ret = -1;
  if ((ret = SensorFrameGetChannelFromChannelId(
           *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel)) < 0) {
    LOG_WARN(
        "EdgeAppLibSensorFrameGetChannelFromChannelId : ret=%d. Skipping "
        "sending "
        "metadata.",
        ret);
    return;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "EdgeAppLibSensorChannelGetRawData : ret=%d. Skipping sending "
        "metadata.",
        ret);
    return;
  }

  LOG_INFO(
      "output_raw_data.address:%p\noutput_raw_data.size:%zu\noutput_raw_data."
      "timestamp:%llu\noutput_raw_data.type:%s",
      data.address, data.size, data.timestamp, data.type);

  /*
   * Retrieve subframe properties:
   * - Subframe properties indicate whether the input tensor is divided into
   *   smaller parts.
   * - Each part, including metadata, is processed based on its subframe number.
   */
  struct EdgeAppLibSensorSubFrameProperty subframe = {0};
  if ((ret = SensorChannelGetProperty(
           channel, AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY, &subframe,
           sizeof(struct EdgeAppLibSensorSubFrameProperty))) < 0) {
    LOG_WARN("SensorChannelGetProperty - SubFrame: ret=%d", ret);
  } else {
    LOG_INFO("SensorChannelGetProperty - SubFrame: current=%d, division=%d",
             subframe.current_num, subframe.division_num);
    /*
     * - Only if current_num is 1, the data includes valid metadata.
     */
    if (subframe.current_num != 1) return;

    /*
     * Proceed with processing metadata:
     * - Metadata is not divided, so no need to append current_num
     *   and division_num.
     */
  }

  EdgeAppLibSendDataResult send_data_res =
      SendDataSyncMeta(data.address, data.size, EdgeAppLibSendDataBase64,
                       data.timestamp, DATA_EXPORT_AWAIT_TIMEOUT);
  if (send_data_res != EdgeAppLibSendDataResultSuccess) {
    LOG_ERR("SendDataSyncMeta failed with EdgeAppLibSendDataResult: %d",
            send_data_res);
  }

  // TODO: since we are using mock EVP, EVP_blobOperation doesn't send the data
  // anywhere, so write it to a file here
  struct stat st;
  if (stat("./metadata", &st) == -1) {
    mkdir("./metadata", 0700);
  }
  char fn[512] = {0};
  snprintf(fn, sizeof(fn), "./metadata/%zu.bin", (size_t)data.timestamp);
  FILE *f = fopen(fn, "wb");
  fwrite(data.address, data.size, 1, f);
  fclose(f);
}

std::string getPortSettingsStr() {
  JSON_Object *obj = DataExportGetPortSettings();
  if (!obj) return "{}";

  JSON_Value *val = json_object_get_wrapping_value(obj);
  char *serialized = json_serialize_to_string(val);

  std::string result(serialized);
  json_free_serialized_string(serialized);

  return result;
}

EdgeAppLibDataExportResult sendState(const std::string &topic,
                                     const std::string &state) {
  // Create a copy of the state that we can safely free
  char *state_copy = (char *)malloc(state.size());
  if (state_copy == nullptr) {
    return EdgeAppLibDataExportResultFailure;
  }
  memcpy(state_copy, state.c_str(), state.size());

  EdgeAppLibDataExportResult result =
      DataExportSendState(topic.c_str(), state_copy, state.size());

  // Note: state_copy is freed by DataExportSendState
  return result;
}

std::string getWorkspaceDirectory() {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  const char *workspace = EVP_getWorkspaceDirectory(context->evp_client,
                                                    EVP_WORKSPACE_TYPE_DEFAULT);
  if (workspace == nullptr) {
    LOG_ERR("Failed to get workspace directory");
    return "";
  }
  return std::string(workspace);
}

std::string formatTimestamp(uint64_t timestamp) {
  char formatted_timestamp[32] = {0};
  DataExportFormatTimestamp(formatted_timestamp, sizeof(formatted_timestamp),
                            timestamp);
  return std::string(formatted_timestamp);
}

std::string getFileSuffix(EdgeAppLibDataExportDataType dataType) {
  char suffix[10] = {0};
  DataExportFileSuffix(suffix, sizeof(suffix), dataType);
  return std::string(suffix);
}

// helper function to validate input parameters
static bool validateInputParameters(const char *filename, const char *url) {
  if (filename == NULL || url == NULL) {
    LOG_ERR("Invalid NULL filename or url provided");
    return false;
  }

  struct stat st;
  if (stat(filename, &st) != 0 || !(st.st_mode & S_IFREG)) {
    LOG_ERR("File does not exist or is not accessible: %s", filename);
    return false;
  }
  return true;
}

// helper function to get send method from given datatype
static int getSendMethod(EdgeAppLibDataExportDataType datatype) {
  JSON_Object *object = DataExportGetPortSettings();
  JSON_Object *port_setting = NULL;
  const char *port_setting_key =
      datatype == EdgeAppLibDataExportRaw ? "input_tensor" : "metadata";

  if (object && json_object_has_value(object, port_setting_key)) {
    port_setting = json_object_get_object(object, port_setting_key);
    if (port_setting && json_object_has_value(port_setting, "method")) {
      return json_object_get_number(port_setting, "method");
    }
  }
  return -1;
}

// helper function to get storage name from port settings
static const char *getStorageName(EdgeAppLibDataExportDataType datatype) {
  JSON_Object *object = DataExportGetPortSettings();
  JSON_Object *port_setting = NULL;
  const char *port_setting_key =
      datatype == EdgeAppLibDataExportRaw ? "input_tensor" : "metadata";

  if (object && json_object_has_value(object, port_setting_key)) {
    port_setting = json_object_get_object(object, port_setting_key);
    if (port_setting && json_object_has_value(port_setting, "storage_name")) {
      return json_object_get_string(port_setting, "storage_name");
    }
  }
  return NULL;
}

// helper function to initialize timeout
static struct timespec initializeTimeout(int timeout_ms) {
  struct timeval now;
  gettimeofday(&now, NULL);
  struct timespec time_to_wait;
  time_to_wait.tv_sec = now.tv_sec + timeout_ms / 1000;
  time_to_wait.tv_nsec = (now.tv_usec + (timeout_ms % 1000) * 1000) * 1000;
  time_to_wait.tv_sec += time_to_wait.tv_nsec / 1000000000;
  time_to_wait.tv_nsec %= 1000000000;
  return time_to_wait;
}

// helper function to wait for other operations to complete
static bool waitForOperationsToComplete(int timeout_ms) {
  struct timespec timeout = initializeTimeout(timeout_ms);

  while (shared_state.process_event_in_progress ||
         shared_state.operation_cb_in_progress) {
    LOG_DBG(
        "waitForOperationsToComplete: "
        "process_event_in_progress=%d, operation_cb_in_progress=%d",
        shared_state.process_event_in_progress,
        shared_state.operation_cb_in_progress);

    int result = pthread_cond_timedwait(&shared_state.cond, &shared_state.mutex,
                                        &timeout);
    if (result == ETIMEDOUT) {
      LOG_ERR("Timeout waiting for other operations to complete");
      shared_state.operation_in_progress = false;
      return false;
    }
  }
  LOG_DBG(
      "waitForOperationsToComplete out: "
      "process_event_in_progress=%d, operation_cb_in_progress=%d",
      shared_state.process_event_in_progress,
      shared_state.operation_cb_in_progress);
  return true;
}

// helper function to wait for callback to complete
static bool waitForCallbackCompletion(int timeout_ms) {
  struct timespec timeout = initializeTimeout(timeout_ms);

  while (shared_state.operation_cb_in_progress) {
    LOG_DBG("waitForCallbackCompletion in: process_event_in_progress=%d",
            shared_state.process_event_in_progress);

    int wait_result = pthread_cond_timedwait(&shared_state.cond,
                                             &shared_state.mutex, &timeout);
    if (wait_result == ETIMEDOUT) {
      LOG_ERR("Timeout waiting for callback to complete");
      shared_state.operation_cb_in_progress = false;
      return false;
    }
  }

  LOG_DBG("waitForCallbackCompletion out: process_event_in_progress=%d",
          shared_state.process_event_in_progress);
  return true;
}

// callback function for blob operation
static void sendFileDoneCallback(EVP_BLOB_CALLBACK_REASON reason,
                                 const void *vp, void *userData) {
  LOG_DBG("SendFileDoneCallback: reason=%d", reason);
  SharedState *shared_state_ = (SharedState *)userData;
  {
    MutexLocker lock(&shared_state_->mutex);
    shared_state_->operation_cb_in_progress = false;
    pthread_cond_signal(&shared_state_->cond);
  }
}

// callback function for telemetry
static void sendTelemetryDoneCallback(EVP_TELEMETRY_CALLBACK_REASON reason,
                                      void *userData) {
  LOG_DBG("SendTelemetryDoneCallback: reason=%d", reason);
  SharedState *shared_state_ = (SharedState *)userData;
  {
    MutexLocker lock(&shared_state_->mutex);
    shared_state_->operation_cb_in_progress = false;
    LOG_TRACE("SendTelemetryDoneCallback: signal");
    pthread_cond_signal(&shared_state_->cond);
    LOG_TRACE("SendTelemetryDoneCallback: end");
  }
}

// perform a blob operation for ordinary HTTP server
static bool performBlobOperationHttp(const char *filename, const char *url,
                                     int timeout_ms, const char *path = NULL,
                                     const char *storage_name = NULL) {
  LOG_TRACE("performBlobOperationHttp: start");
  struct EVP_BlobRequestHttp request = {.url = url};
  struct EVP_BlobLocalStore localstore = {.filename = filename};

  LOG_DBG("url=%s", url);
  LOG_DBG("filename=%s", filename);

  LOG_TRACE("EVP_blobOperation begin");
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EVP_RESULT result = EVP_blobOperation(
      context->evp_client, EVP_BLOB_TYPE_HTTP, EVP_BLOB_OP_PUT, &request,
      &localstore, (EVP_BLOB_CALLBACK)sendFileDoneCallback, &shared_state);
  LOG_DBG("EVP_blobOperation result=%d", result);
  LOG_TRACE("performBlobOperationHttp: end");
  return result == EVP_OK;
}

// perform a blob operation for EVP Storage Provider
static bool performBlobOperationEvpExt(const char *filename, const char *url,
                                       int timeout_ms, const char *path,
                                       const char *storage_name) {
  LOG_TRACE("performBlobOperationEvpExt: start");
  struct EVP_BlobRequestEvpExt ext_request;
  char blob_path[420] = {0};
  const char *basename_ptr = strrchr(filename, '/');
  const char *actual_filename = basename_ptr ? basename_ptr + 1 : filename;
  LOG_DBG("filename=%s", filename);
  LOG_DBG("basename_ptr=%s", basename_ptr);
  LOG_DBG("actual_filename=%s", actual_filename);
  snprintf(blob_path, sizeof(blob_path), "%s/%s", path, actual_filename);
  LOG_DBG("blob_path=%s", blob_path);

  ext_request.remote_name = blob_path;
  ext_request.storage_name = storage_name;

  struct EVP_BlobLocalStore localstore = {.filename = filename};

  LOG_TRACE("EVP_blobOperation begin");
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EVP_RESULT result = EVP_blobOperation(
      context->evp_client, EVP_BLOB_TYPE_EVP_EXT, EVP_BLOB_OP_PUT, &ext_request,
      &localstore, (EVP_BLOB_CALLBACK)sendFileDoneCallback, &shared_state);
  LOG_DBG("EVP_blobOperation result=%d", result);

  LOG_TRACE("performBlobOperationEvpExt: end");
  return result == EVP_OK;
}

// perform send telemetry operation
static bool performSendTelemetry(void *data, int datalen, int timeout_ms) {
  LOG_DBG("performSendTelemetry: data=%p, datalen=%d", data, datalen);
  struct EVP_telemetry_entry entries = {.key = g_placeholder_telemetry_key,
                                        .value = (char *)data};
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EVP_RESULT result = EVP_sendTelemetry(
      context->evp_client, &entries, 1,
      (EVP_TELEMETRY_CALLBACK)sendTelemetryDoneCallback, &shared_state);
  LOG_DBG("performSendTelemetry: EVP_sendTelemetry result=%d", result);
  return result == EVP_OK;
}

// Helper function to run an operation with synchronization
template <typename Operation>
static bool runSynchronizedOperation(Operation operation, int timeout_ms) {
  {
    MutexLocker lock(&shared_state.mutex);
    shared_state.operation_in_progress = true;
    if (!waitForOperationsToComplete(timeout_ms)) {
      return false;
    }
    shared_state.operation_cb_in_progress = true;
  }

  if (!operation()) {
    {
      MutexLocker lock(&shared_state.mutex);
      shared_state.operation_in_progress = false;
      shared_state.operation_cb_in_progress = false;
      pthread_cond_signal(&shared_state.cond);
    }
    return false;
  }

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  LOG_TRACE("EVP_processEvent begin");
  EVP_RESULT result = EVP_processEvent(context->evp_client, 5000);
  LOG_DBG("EVP_processEvent result=%d", result);
  if (result == EVP_SHOULDEXIT) {
    context->SetNextState(STATE_DESTROYING);
    {
      MutexLocker lock(&shared_state.mutex);
      shared_state.operation_in_progress = false;
      shared_state.operation_cb_in_progress = false;
      pthread_cond_signal(&shared_state.cond);
    }
    return false;
  }

  // Update shared state to indicate that the operation has completed
  // so that other operations can proceed.
  {
    MutexLocker lock(&shared_state.mutex);
    shared_state.operation_in_progress = false;
    pthread_cond_signal(&shared_state.cond);
  }

  // Wait for the callback to complete. Usually the callback completes
  // immediately while processing EVP_processEvent.
  {
    MutexLocker lock(&shared_state.mutex);
    if (shared_state.operation_cb_in_progress) {
      if (!waitForCallbackCompletion(timeout_ms)) {
        return false;
      }
    }
  }

  return true;
}

// Helper function to handle blob operation for EVP Storage Provider
static bool handleBlobOperationEvpExt(EdgeAppLibDataExportDataType datatype,
                                      const char *filename, const char *url,
                                      int timeout_ms) {
  const char *port_setting_key =
      datatype == EdgeAppLibDataExportRaw ? "input_tensor" : "metadata";

  JSON_Object *port_settings = DataExportGetPortSettings();
  JSON_Object *port_setting =
      json_object_get_object(port_settings, port_setting_key);

  const char *storage_name = NULL;
  const char *path = NULL;

  if (port_setting) {
    if (json_object_has_value(port_setting, "storage_name")) {
      storage_name = json_object_get_string(port_setting, "storage_name");
    }
    if (json_object_has_value(port_setting, "path")) {
      path = json_object_get_string(port_setting, "path");
    }
  }

  return runSynchronizedOperation(
      [&]() {
        return performBlobOperationEvpExt(filename, url, timeout_ms, path,
                                          storage_name);
      },
      timeout_ms);
}

// Helper function to unlink a file in a background thread
void unlink_in_background(const char *filename) {
  std::string fname(filename);
  std::thread([fname]() {
    LOG_TRACE("background unlink begin");
    unlink(fname.c_str());
    LOG_TRACE("background unlink end");
  }).detach();
}

bool sendFileSync(EdgeAppLibDataExportDataType datatype, const char *filename,
                  const char *url, int timeout_ms) {
  LOG_TRACE("sendFileSync start");

  if (!DataExportIsEnabled(datatype)) {
    LOG_ERR("datatype is not enabled");
    return false;
  }

  if (!validateInputParameters(filename, url)) {
    LOG_ERR("Invalid filename or url provided");
    return false;
  }

  int sendMethod = getSendMethod(datatype);
  if (sendMethod == -1) {
    LOG_ERR("Invalid sendMethod=%d", sendMethod);
    return false;
  }

  bool result = false;
  switch (sendMethod) {
    case METHOD_HTTP_STORAGE:
      LOG_TRACE("sendMethod is HTTP_STORAGE");
      result = runSynchronizedOperation(
          [&]() {
            return performBlobOperationHttp(filename, url, timeout_ms, NULL,
                                            NULL);
          },
          timeout_ms);
      break;

    case METHOD_BLOB_STORAGE:
      LOG_TRACE("sendMethod is BLOB_STORAGE");
      result = handleBlobOperationEvpExt(datatype, filename, url, timeout_ms);
      break;

    default:
      LOG_ERR("sendMethod=%d is not supported", sendMethod);
      return false;
  }

  if (filename) {
    // Note: unlink sometimes takes several seconds to complete, though not
    // always. To avoid blocking the running thread, perform unlink in a
    // background thread. However, similar delays can also occur with file I/O
    // operations.
    unlink_in_background(filename);
  }

  LOG_DBG("sendFileSync: result=%d", result);
  return result;
}

bool sendTelemetrySync(void *data, int datalen, int timeout_ms) {
  LOG_TRACE("sendTelemetrySync start");

  if (!DataExportIsEnabled(EdgeAppLibDataExportMetadata)) {
    LOG_ERR("datatype is not enabled");
    return false;
  }
  if (!data || datalen == 0) {
    LOG_ERR("invalid data or datalen");
    return false;
  }

  int sendMethod = getSendMethod(EdgeAppLibDataExportMetadata);
  if (sendMethod != METHOD_MQTT) {
    LOG_ERR("sendMethod=%d is not supported", sendMethod);
    return false;
  }

  bool result = runSynchronizedOperation(
      [&]() { return performSendTelemetry(data, datalen, timeout_ms); },
      timeout_ms);

  if (!result) {
    LOG_ERR("Failed to send telemetry data");
    return false;
  }

  LOG_TRACE("DataExportSendTelemetry: finished successfully");
  return true;
}

EdgeAppLibDataExportResult sendFile(EdgeAppLibDataExportDataType dataType,
                                    const std::string &filePath,
                                    const std::string &url, int timeout_ms) {
  bool success =
      sendFileSync(dataType, filePath.c_str(), url.c_str(), timeout_ms);
  if (!success) {
    LOG_ERR("Failed to send file:");
    LOG_ERR("- filePath: %s", filePath.c_str());
    LOG_ERR("- url: %s", url.c_str());
    return EdgeAppLibDataExportResultFailure;
  }
  return EdgeAppLibDataExportResultSuccess;
}

EdgeAppLibDataExportResult sendTelemetry(void *data, int datalen,
                                         int timeout_ms) {
  bool success = sendTelemetrySync(data, datalen, timeout_ms);
  if (!success) {
    LOG_ERR("Failed to send telemetry");
    return EdgeAppLibDataExportResultFailure;
  }
  return EdgeAppLibDataExportResultSuccess;
}
