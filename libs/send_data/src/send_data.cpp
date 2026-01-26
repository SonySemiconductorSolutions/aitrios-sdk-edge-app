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

#include "send_data.h"

#include <pthread.h>
#include <stdlib.h>

#include "data_export.h"
#include "log.h"
#include "process_format.hpp"
#include "send_data_private.h"
#include "sm_api.hpp"
#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif

static InfElem output_tensor_vec[MAX_NUMBER_OF_INFERENCE_QUEUE] = {
    {nullptr, nullptr}};

static pthread_mutex_t inf_mutex = PTHREAD_MUTEX_INITIALIZER;

EdgeAppLibSendDataResult SendDataSyncImage(
    void *data, int datalen, EdgeAppLibImageProperty *image_property,
    uint64_t timestamp, int timeout_ms, uint32_t current, uint32_t division) {
  LOG_TRACE("Entering SendDataSyncImage");

  if (data == nullptr || image_property == nullptr) {
    const char *error_msg = "Invalid data param";
    LOG_ERR("%s", error_msg);
    return EdgeAppLibSendDataResultInvalidParam;
  }

  /* If the data length is less than the size of the image,
   * send the raw data directly without processing assuming that it is already
   * encoded.
   */
  if ((datalen > 0) &&
      (datalen < image_property->stride_bytes * image_property->height)) {
    LOG_DBG(
        "Data length is less than the size of the image. "
        "Processing the data as raw data.");
    EdgeAppLibDataExportFuture *future =
        DataExportSendData((char *)PORTNAME_META, EdgeAppLibDataExportRaw, data,
                           datalen, timestamp, current, division);
    EdgeAppLibDataExportResult send_ret = DataExportAwait(future, timeout_ms);
    DataExportCleanup(future);

    if (send_ret == EdgeAppLibDataExportResultSuccess)
      return EdgeAppLibSendDataResultSuccess;
    else
      return EdgeAppLibSendDataResultFailure;
  }

  LOG_DBG(
      "Data length is greater than the size of the image. "
      "Processing the data as an image.");
  JSON_Object *json_object = getCodecSettings();
  int codec_number = json_object_get_number(json_object, "format");

  LOG_WARN("Codec number from settings: %d", codec_number);

  void *codec_buffer = NULL;
  int32_t codec_size = 0;
  MemoryRef codec_memory_ref = {};
  codec_memory_ref.type = MEMORY_MANAGER_MAP_TYPE;
  codec_memory_ref.u.p = data;

  ProcessFormatResult ret;  // Declare ret first
  ret = ProcessFormatInput(codec_memory_ref, datalen,
                           (ProcessFormatImageType)codec_number, image_property,
                           timestamp, &codec_buffer, &codec_size);

  if (ret != kProcessFormatResultOk) {
    LOG_ERR("ProcessFormatImage failed. Exit with return %d.", ret);
    if (codec_buffer != NULL) {
      free(codec_buffer);
    }
    return EdgeAppLibSendDataResultFailure;
  }

  EdgeAppLibDataExportFuture *future = DataExportSendData(
      (char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw, codec_buffer, codec_size,
      timestamp, current, division);
  EdgeAppLibDataExportResult send_ret = DataExportAwait(future, timeout_ms);
  DataExportCleanup(future);

  if (send_ret == EdgeAppLibDataExportResultSuccess)
    return EdgeAppLibSendDataResultSuccess;
  else
    return EdgeAppLibSendDataResultFailure;
}

EdgeAppLibSendDataResult SendDataSyncMeta(void *data, int datalen,
                                          EdgeAppLibSendDataType datatype,
                                          uint64_t timestamp, int timeout_ms) {
  LOG_TRACE("Entering SendDataSyncMeta");

  if (data == nullptr) {
    const char *error_msg = "Invalid data param";
    LOG_ERR("%s", error_msg);
    return EdgeAppLibSendDataResultInvalidParam;
  }

  // Calculate the size of the Base64 encoded data
  int base64_size = ((datalen + 2) / 3) * 4;  // For Base64 encoding
  int json_overhead = 256;                    // For JSON formatting
  int extra_padding = 0;                      // For extra padding

  size_t buffer_size = base64_size + json_overhead + extra_padding;

  // pre-allocate memory for JSON buffer
  char *json_buffer = (char *)malloc(buffer_size);
  if (!json_buffer) {
    LOG_ERR("Failed to allocate memory for JSON buffer");
    return EdgeAppLibSendDataResultDataTooLarge;
  }

  // Pass pre-allocated memory to ProcessFormatMeta
  ProcessFormatResult process_format_ret = ProcessFormatMeta(
      data, datalen, datatype, timestamp, json_buffer, buffer_size);
  if (process_format_ret != kProcessFormatResultOk) {
    LOG_ERR("ProcessFormatMeta failed. Exit with return %d.",
            process_format_ret);
    free(json_buffer);
    return EdgeAppLibSendDataResultFailure;
  }

  // Keep simple for Single Inference case
  // TODO : Refactor for multiple inferences
  if (getNumOfInfPerMsg() == 1) {
    EdgeAppLibDataExportFuture *future =
        DataExportSendData((char *)PORTNAME_META, EdgeAppLibDataExportMetadata,
                           json_buffer, strlen(json_buffer), timestamp);
    EdgeAppLibDataExportResult send_ret = DataExportAwait(future, timeout_ms);
    DataExportCleanup(future);
    free(json_buffer);
    pthread_mutex_unlock(&inf_mutex);

    if (send_ret == EdgeAppLibDataExportResultSuccess)
      return EdgeAppLibSendDataResultSuccess;
    else
      return EdgeAppLibSendDataResultFailure;
  }
  // Append one inference to Output Tensor
  JSON_Value *output_tensor_value = json_parse_string((char *)json_buffer);
  free(json_buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  const char *model_id =
      json_object_get_string(output_tensor_object, "ModelID");
  if (SendDataAppendOutputTensor(model_id, output_tensor_value) !=
      EdgeAppLibSendDataResultSuccess) {
    LOG_ERR("SendDataAppendOutputTensor failed");
    json_value_free(output_tensor_value);
    return EdgeAppLibSendDataResultFailure;
  }

  // Check number_of_inference_per_message
  static uint32_t inf_cnt = 1;
  if (inf_cnt < getNumOfInfPerMsg()) {
    inf_cnt++;
    return EdgeAppLibSendDataResultEnqueued;
  } else {
    // Send Data
    EdgeAppLibSendDataResult send_ret = EdgeAppLibSendDataResultSuccess;
    pthread_mutex_lock(&inf_mutex);
    for (int i = 0; i < MAX_NUMBER_OF_INFERENCE_QUEUE; ++i) {
      if (output_tensor_vec[i].key == nullptr) break;
      // Get smallest timestamp
      JSON_Object *output_tensor_object_buf =
          json_value_get_object(output_tensor_vec[i].value);
      JSON_Array *infs_json_array =
          json_object_get_array(output_tensor_object_buf, "Inferences");
      JSON_Object *first_inf = json_array_get_object(infs_json_array, 0);
      const char *smallest_timestamp_str =
          json_object_get_string(first_inf, "T");
      uint64_t samllest_timestamp =
          SendDataConvertTimeToNanoseconds(smallest_timestamp_str);
      char *send_buffer = json_serialize_to_string(output_tensor_vec[i].value);
      // Send Data
      EdgeAppLibDataExportFuture *future = DataExportSendData(
          (char *)PORTNAME_META, EdgeAppLibDataExportMetadata, send_buffer,
          strlen(send_buffer), samllest_timestamp);
      EdgeAppLibDataExportResult ret = DataExportAwait(future, timeout_ms);

      if (ret != EdgeAppLibDataExportResultSuccess) {
        send_ret = EdgeAppLibSendDataResultFailure;
      }
      DataExportCleanup(future);
      json_free_serialized_string((char *)send_buffer);
      json_value_free(output_tensor_vec[i].value);
      output_tensor_vec[i] = (InfElem){.key = nullptr, .value = nullptr};
    }
    pthread_mutex_unlock(&inf_mutex);

    inf_cnt = 1;
    return send_ret;
  }
}

EdgeAppLibSendDataResult SendDataAppendOutputTensor(const char *key,
                                                    JSON_Value *value) {
  pthread_mutex_lock(&inf_mutex);
  bool is_set = false;
  for (int i = 0; i < MAX_NUMBER_OF_INFERENCE_QUEUE; ++i) {
    if (output_tensor_vec[i].key == nullptr) {
      output_tensor_vec[i].key = key;
      output_tensor_vec[i].value = value;
      is_set = true;
      break;
    } else if (strcmp(output_tensor_vec[i].key, key) == 0) {
      // Append value to output_tensor_vec[i].value
      JSON_Object *output_tensor_object_buf =
          json_value_get_object(output_tensor_vec[i].value);
      JSON_Array *infs_json_array =
          json_object_get_array(output_tensor_object_buf, "Inferences");
      JSON_Object *output_tensor_object = json_value_get_object(value);
      JSON_Array *inf_value =
          json_object_get_array(output_tensor_object, "Inferences");
      JSON_Value *inf_first = json_array_get_value(inf_value, 0);
      json_array_append_value(infs_json_array, json_value_deep_copy(inf_first));
      json_value_free(value);
      is_set = true;
      break;
    }
  }
  pthread_mutex_unlock(&inf_mutex);
  return is_set ? EdgeAppLibSendDataResultSuccess
                : EdgeAppLibSendDataResultDataTooLarge;
}

uint64_t SendDataConvertTimeToNanoseconds(const char *datetime) {
  struct tm tm;
  char milliseconds[4];
  uint64_t nanoseconds;
  strptime(datetime, "%Y%m%d%H%M%S", &tm);
  sscanf(datetime + 14, "%3s", milliseconds);
  time_t epoch_time = timegm(&tm);
  nanoseconds = (uint64_t)epoch_time * 1000000000ULL;
  nanoseconds += (uint64_t)atoi(milliseconds) * 1000000ULL;
  return nanoseconds;
}

#ifdef __cplusplus
}
#endif

}  // namespace EdgeAppLib
