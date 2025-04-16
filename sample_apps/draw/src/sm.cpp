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
#include "detection_utils.hpp"
#include "draw.h"
#include "log.h"
#include "objectdetection_generated.h"
#include "send_data.h"
#include "sensor.h"
#include "sm_utils.hpp"

#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"

#define DATA_EXPORT_AWAIT_TIMEOUT 10000
#define SENSOR_GET_FRAME_TIMEOUT 5000
#define METADATA_MAX_LENTH 500

using namespace EdgeAppLib;

EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;
char *state_topic = NULL;
int32_t res_release_frame = -1;
char s_metadata[METADATA_MAX_LENTH] = "";

char *GetConfigureErrorJsonSm(ResponseCode code, const char *message,
                              const char *res_id) {
  char *config_error = nullptr;
  asprintf(
      &config_error,
      "{\"res_info\": {\"res_id\":\"%s\",\"code\": %d,\"detail_msg\":\"%s\"}}",
      res_id, code, message);
  return config_error;
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
        "SensorFrameGetChannelFromChannelId 0x00000001 : ret=%d. Skipping "
        "sending "
        "input tensor.",
        ret);
    if ((res_release_frame = SensorReleaseFrame(s_stream, *frame)) < 0) {
      LOG_ERR("SensorReleaseFrame : ret= %d", res_release_frame);
      PrintSensorError();
    }
    return nullptr;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "SensorChannelGetRawData : ret=%d. Skipping sending input "
        "tensor.",
        ret);
    if ((res_release_frame = SensorReleaseFrame(s_stream, *frame)) < 0) {
      LOG_ERR("SensorReleaseFrame : ret= %d", res_release_frame);
      PrintSensorError();
    }
    return nullptr;
  }

  LOG_TRACE("Create draw buffer");
  extern DataProcessorCustomParam detection_param;
  uint32_t img_w = detection_param.input_width;
  uint32_t img_h = detection_param.input_height;
  EdgeAppLibDrawFormat img_format = AITRIOS_DRAW_FORMAT_UNDEFINED;
  EdgeAppLibSensorImageProperty property = {};
  if (SensorStreamGetProperty(s_stream, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                              &property, sizeof(property)) != 0) {
    free(data.address);
    LOG_ERR("SensorStreamGetProperty failed for %s",
            AITRIOS_SENSOR_IMAGE_PROPERTY_KEY);
    if ((res_release_frame = SensorReleaseFrame(s_stream, *frame)) < 0) {
      LOG_ERR("SensorReleaseFrame : ret= %d", res_release_frame);
      PrintSensorError();
    }
    return nullptr;
  }
  if (strncmp(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
              strlen(AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR)) == 0) {
    img_format = AITRIOS_DRAW_FORMAT_RGB8_PLANAR;
  } else if (strncmp(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB24,
                     strlen(AITRIOS_SENSOR_PIXEL_FORMAT_RGB24)) == 0) {
    img_format = AITRIOS_DRAW_FORMAT_RGB8;
  }
  struct EdgeAppLibDrawBuffer buffer = {data.address, data.size, img_format,
                                        img_w, img_h};
  // Draw box
  if (strlen(s_metadata) > 0) {
    auto object_detection_root =
        SmartCamera::GetObjectDetectionRoot(s_metadata);
    auto obj_detection_data =
        object_detection_root->metadata_as_ObjectDetectionTop()
            ->perception()
            ->object_detection_list();
    for (int i = 0; i < obj_detection_data->size(); ++i) {
      auto general_object = obj_detection_data->Get(i);

      auto bbox = general_object->bounding_box_as_BoundingBox2d();
      LOG_DBG("box[%d]=[ %d, %d, %d, %d]", i, bbox->left(), bbox->top(),
              bbox->right(), bbox->bottom());
      DrawRectangle(&buffer, bbox->left(), bbox->top(), bbox->right(),
                    bbox->bottom(), AITRIOS_COLOR_RED);
    }
  }
  if ((res_release_frame = SensorReleaseFrame(s_stream, *frame)) < 0) {
    free(data.address);
    LOG_ERR("SensorReleaseFrame : ret= %d", res_release_frame);
    PrintSensorError();
    return nullptr;
  }
  return DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
                            buffer.address, buffer.size, data.timestamp);
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
  memset(s_metadata, '\0', METADATA_MAX_LENTH);

  if ((ret = SensorFrameGetChannelFromChannelId(
           *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel)) < 0) {
    LOG_WARN(
        "SensorFrameGetChannelFromChannelId : ret=%d. Skipping "
        "sending "
        "metadata.",
        ret);
    return;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "SensorChannelGetRawData : ret=%d. Skipping sending "
        "metadata.",
        ret);
    return;
  }

  LOG_INFO(
      "output_raw_data.address:%p\noutput_raw_data.size:%zu\noutput_raw_data."
      "timestamp:%llu\noutput_raw_data.type:%s",
      data.address, data.size, data.timestamp, data.type);

  void *metadata = NULL;
  uint32_t metadata_size = 0;

  DataProcessorResultCode data_processor_ret = DataProcessorAnalyze(
      (float *)data.address, data.size, (char **)&metadata, &metadata_size);

  if (data_processor_ret != kDataProcessorOk) {
    LOG_WARN("DataProcessorAnalyze: ret=%d", data_processor_ret);
    return;
  }

  if (metadata_size >= METADATA_MAX_LENTH) {
    LOG_WARN("Metadata size exceeds s_metadata capacity.");
    metadata_size = METADATA_MAX_LENTH - 1;
  }
  memcpy(s_metadata, metadata, metadata_size);
  s_metadata[metadata_size] = '\0';

  EdgeAppLibSendDataResult result =
      SendDataSyncMeta(metadata, metadata_size, DataProcessorGetDataType(),
                       data.timestamp, DATA_EXPORT_AWAIT_TIMEOUT);
  if (result != EdgeAppLibSendDataResultSuccess &&
      result != EdgeAppLibSendDataResultEnqueued) {
    void *metadata_json = NULL;
    uint32_t metadata_json_size = 0;
    const char *error_msg = "Error SendDataSyncMeta.";
    LOG_ERR("%s : result=%d", error_msg, result);
    metadata_json = GetConfigureErrorJsonSm(ResponseCodeUnknown, error_msg, "");
    metadata_json_size = strlen((const char *)metadata_json);
    DataExportSendState(state_topic, (void *)metadata_json, metadata_json_size);
  }
  free(metadata);
}

int onCreate() {
  LOG_TRACE("Inside onCreate.");
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
  state_topic = topic;
  char *output = NULL;
  DataProcessorResultCode res;
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
    LOG_ERR("SensorGetFrame : ret=%d", ret);
    PrintSensorError();
    return SensorGetLastErrorCause() == AITRIOS_SENSOR_ERROR_TIMEOUT ? 0 : -1;
  }

  EdgeAppLibDataExportFuture *future = nullptr;

  if (metadataEnabled) {
    sendMetadata(&frame);
  }
  if (inputTensorEnabled) {
    future = sendInputTensor(&frame);
  }

  if (future) {
    DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
    DataExportCleanup(future);
  }

  if (!inputTensorEnabled) {
    if ((res_release_frame = SensorReleaseFrame(s_stream, frame)) < 0) {
      LOG_ERR("SensorReleaseFrame : ret= %d", res_release_frame);
      PrintSensorError();
      return -1;
    }
    res_release_frame = -1;
  } else if (res_release_frame < 0) {
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
  struct EdgeAppLibSensorImageCropProperty crop = {0};
  if ((ret = SensorStreamGetProperty(s_stream,
                                     AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                                     &crop, sizeof(crop))) != 0) {
    LOG_ERR("SensorStreamGetProperty : ret=%d", ret);
    PrintSensorError();
    return -1;
  }
  LOG_INFO("Crop: [x=%d, y=%d, w=%d, h=%d]", crop.left, crop.top, crop.width,
           crop.height);
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
