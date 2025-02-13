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
#include "draw.h"
#include "log.h"
#include "sensor.h"

#define PORTNAME_INPUT "input"
#define BUFSIZE 128

#define DATA_EXPORT_AWAIT_TIMEOUT 10000
#define SENSOR_GET_FRAME_TIMEOUT 5000

using namespace EdgeAppLib;

EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;

void PrintSensorError() {
  uint32_t length = BUFSIZE;
  char message_buffer[BUFSIZE] = {0};
  SensorGetLastErrorString(
      EdgeAppLibSensorStatusParam::AITRIOS_SENSOR_STATUS_PARAM_MESSAGE,
      message_buffer, &length);

  LOG_ERR("level: %d - cause: %d - message: %s", SensorGetLastErrorLevel(),
          SensorGetLastErrorCause(), message_buffer);
}

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
        "SensorFrameGetChannelFromChannelId : ret=%d. Skipping "
        "sending "
        "input tensor.",
        ret);
    return nullptr;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "SensorChannelGetRawData : ret=%d. Skipping sending input "
        "tensor.",
        ret);
    return nullptr;
  }

  // Modify image buffer using Draw API
  struct EdgeAppLibDrawBuffer buffer = {data.address, data.size,
                                        AITRIOS_DRAW_FORMAT_RGB8, 500, 924};
  DrawRectangle(&buffer, 40, 50, 600, 300, AITRIOS_COLOR_RED);
  DrawRectangle(&buffer, 200, 100, 400, 400, AITRIOS_COLOR_GREEN);

  return DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
                            data.address, data.size, data.timestamp);
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
  LOG_INFO("Inside onConfigure. Not implemented.");
  return 0;
}

int onIterate() {
  LOG_TRACE("Inside onIterate.");

  bool inputTensorEnabled = DataExportIsEnabled(EdgeAppLibDataExportRaw);
  if (!inputTensorEnabled) {
    // Early exit to avoid doing unnecessary work when DataExport is disabled
    return 0;
  }

  EdgeAppLibSensorFrame frame;
  int32_t ret = -1;
  if ((ret = SensorGetFrame(s_stream, &frame, SENSOR_GET_FRAME_TIMEOUT)) < 0) {
    LOG_ERR("SensorGetFrame : ret=%d", ret);
    PrintSensorError();
    return SensorGetLastErrorCause() == AITRIOS_SENSOR_ERROR_TIMEOUT ? 0 : -1;
  }

  EdgeAppLibDataExportFuture *future =
      inputTensorEnabled ? sendInputTensor(&frame) : nullptr;

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
