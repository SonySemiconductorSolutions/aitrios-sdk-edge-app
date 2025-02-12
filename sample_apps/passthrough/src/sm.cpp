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

#include "sm.h"

#include <stdlib.h>

#include "data_export.h"
#include "data_processor_api.hpp"
#include "log.h"
#include "send_data.h"
#include "sensor.h"
#include "sm_utils.hpp"

#define PORTNAME_META "metadata"

#define PORTNAME_INPUT "input"
#define DATA_EXPORT_AWAIT_TIMEOUT 10000
#define SENSOR_GET_FRAME_TIMEOUT 5000
EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;
char *state_topic = NULL;

using namespace EdgeAppLib;

/**
 * @brief Sends the Input Tensor to the cloud asynchronously.
 *
 * This function sends the input tensor data from the provided frame to the
 * cloud. It returns a future object representing the asynchronous operation.
 *
 * By returning a future, this function allows for non-blocking execution.
 * The caller can await this future after sending the output tensor, ensuring
 * that both awaits are done consecutively without blocking the sending of the
 * rest of the data.
 *
 * @param frame Pointer to the current sensor frame.
 * @return A future representing the asynchronous operation of sending the input
 * tensor.
 */
static EdgeAppLibDataExportFuture *sendInputTensor(
    EdgeAppLibSensorFrame *frame) {
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
    return nullptr;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "EdgeAppLibSensorChannelGetRawData : ret=%d. Skipping sending input "
        "tensor.",
        ret);
    return nullptr;
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
      return nullptr;
    } else {
      /*
       * Includes current_num and division_num for processing valid subframes.
       */
      return DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
                                data.address, data.size, data.timestamp,
                                subframe.current_num, subframe.division_num);
    }
  }

  return DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
                            data.address, data.size, data.timestamp);
}

/**
 * @brief Sends the Metadata to the cloud synchronously.
 *
 * This function sends the post-processed output tensor (metadata) from the
 * provided sensor frame to the cloud.
 *
 * @param frame Pointer to the current sensor frame.
 */
static void sendMetadata(EdgeAppLibSensorFrame *frame) {
  LOG_TRACE("Inside sendMetadata.");

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
}

int onCreate() {
  LOG_TRACE("Inside onCreate. Using a pseudo stream key.");
  int32_t ret = -1;
  if ((ret = SensorCoreInit(&s_core)) < 0) {
    LOG_ERR("SensorCoreInit : ret=%d", ret);
    return -1;
  }
  const char *stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
  if ((ret = SensorCoreOpenStream(s_core, stream_key, &s_stream)) < 0) {
    LOG_ERR("SensorCoreOpenStream : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  return 0;
}

int onConfigure(char *topic, void *value, int valuesize) {
  LOG_TRACE("Inside onConfigure.");
  if (value == NULL) {
    LOG_ERR("[onConfigure] Invalid param : value=NULL");
    return -1;
  }
  LOG_INFO("[onConfigure] topic:%s\nvalue:%s\nvaluesize:%i\n", topic,
           (char *)value, valuesize);

  char *output = NULL;
  DataProcessorResultCode res;
  state_topic = topic;
  if ((res = DataProcessorConfigure((char *)value, &output)) !=
      kDataProcessorOk) {
    DataExportSendState(topic, (void *)output, strlen(output));
    free(value);
    return (res == kDataProcessorInvalidParam) ? 0 : -1;
  }
  DataExportSendState(topic, value, valuesize);
  return 0;
}

int onIterate() {
  LOG_TRACE("Inside onIterate.");

  bool inputTensorEnabled = DataExportIsEnabled(EdgeAppLibDataExportRaw);
  bool metadataEnabled = DataExportIsEnabled(EdgeAppLibDataExportMetadata);
  if (!inputTensorEnabled && !metadataEnabled) {
    // Early exit to avoid doing unnecessary work when DataExport is disabled
    return 0;
  }

  EdgeAppLibSensorFrame frame;
  int32_t ret = -1;
  if ((ret = SensorGetFrame(s_stream, &frame, SENSOR_GET_FRAME_TIMEOUT)) < 0) {
    LOG_ERR("EdgeAppLibSensorGetFrame : ret=%d", ret);
    PrintSensorError();
    return SensorGetLastErrorCause() == AITRIOS_SENSOR_ERROR_TIMEOUT ? 0 : -1;
  }

  EdgeAppLibDataExportFuture *future =
      inputTensorEnabled ? sendInputTensor(&frame) : nullptr;
  if (metadataEnabled) {
    sendMetadata(&frame);
  }

  if (future) {
    DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
    DataExportCleanup(future);
  }

  if ((ret = SensorReleaseFrame(s_stream, frame)) < 0) {
    LOG_ERR("SensorReleaseFrame : ret= %d", ret);
    PrintSensorError();
    return -1;
  }
  return 0;
}

int onStop() {
  LOG_TRACE("Inside onStop.");
  int32_t ret = -1;
  if ((ret = SensorStop(s_stream)) < 0) {
    LOG_ERR("SensorStop : ret=%d", ret);
    PrintSensorError();
    return -1;
  }
  return 0;
}

int onStart() {
  LOG_TRACE("Inside onStart.");
  int32_t ret = -1;
  if ((ret = SensorStart(s_stream)) < 0) {
    LOG_ERR("SensorStart : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  return 0;
}

int onDestroy() {
  LOG_TRACE("Inside onDestroy.");
  int32_t ret = -1;
  if ((ret = SensorCoreCloseStream(s_core, s_stream)) < 0) {
    LOG_ERR("SensorCoreCloseStream : ret=%d", ret);
    PrintSensorError();
    return -1;
  }
  if ((ret = SensorCoreExit(s_core)) < 0) {
    LOG_ERR("SensorCoreExit : ret=%d", ret);
    PrintSensorError();
    return -1;
  }
  return 0;
}
