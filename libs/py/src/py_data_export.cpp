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

#include <string>

#include "data_export.h"
#include "log.h"
#include "parson.h"
#include "send_data.h"
#include "sensor.h"

#define DATA_EXPORT_AWAIT_TIMEOUT -1

using namespace EdgeAppLib;

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

std::string getPortSettings() {
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
