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

#include "apitest_util.h"
#include "data_export.h"
#include "data_processor_api.hpp"
#include "dcpu_param_parser.h"
#include "log.h"
#include "sensor.h"
#include "sm_utils.hpp"

#define PORTNAME_META "metadata"

#define PORTNAME_INPUT "input"
#define DATA_EXPORT_AWAIT_TIMEOUT 10000
#define SENSOR_GET_FRAME_TIMEOUT 5000
EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;
char *state_topic = NULL;

#define APITEST_LOG_WARN LOG_WARN
static uint32_t s_crop[4] = {};
static bool s_post_process_available = false;
static char s_fmt_buffer[128];
char *Fmt(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(s_fmt_buffer, sizeof(s_fmt_buffer), fmt, ap);
  va_end(ap);
  return s_fmt_buffer;
}

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

  void *metadata_json = NULL;
  uint32_t metadata_json_size = 0;
  DataProcessorResultCode data_processor_ret =
      DataProcessorJsonFormat(data.address, data.size, data.timestamp,
                              (char **)&metadata_json, &metadata_json_size);
  if (data_processor_ret != kDataProcessorOk) {
    DataExportSendState(state_topic, (void *)metadata_json, metadata_json_size);
    return;
  }
  EdgeAppLibDataExportFuture *future =
      DataExportSendData((char *)PORTNAME_META, EdgeAppLibDataExportMetadata,
                         metadata_json, metadata_json_size, data.timestamp);
  if (future) {
    DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
    DataExportCleanup(future);
  }
  // NOTE: requires operation to be finished.
  json_free_serialized_string((char *)metadata_json);
}

int32_t GetDcpuCapabilityInfo(EdgeAppLibSensorStream stream) {
  const char *context = "<GetDcpuCapabilityInfo>";
  EdgeAppLibSensorPostProcessAvailableProperty post_process_available_prop = {
      0};
  int32_t ret = SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY,
      &post_process_available_prop, sizeof(post_process_available_prop));
  if (ret != 0) {
    EdgeAppLibLogError(
        context, Fmt("EdgeAppLibSensorStreamGetProperty[%s]:ret=%d\n",
                     AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY, ret));
    return -1;
  }

  if (!post_process_available_prop.is_available) {
    EdgeAppLibLogError(context,
                       Fmt("post_process_available_prop.is_available:%d\n",
                           post_process_available_prop.is_available));
    return -1;
  }

  return 0;
}

int onCreate() {
  LOG_TRACE("Inside onCreate. Using a pseudo stream key.");

  // apitest
  const char *context = "<onCreate>";
  EdgeAppLibLogTrace(context, "start.");
  EdgeAppLibLogDebug(context, "start.");
  EdgeAppLibLogInfo(context, "start.");
  EdgeAppLibLogWarn(context, "start.");
  EdgeAppLibLogError(context, "start.");
  EdgeAppLibLogCritical(context, "start.");
  LOG_CRITICAL("start.");

  int32_t ret = -1;
  if ((ret = SensorCoreInit(&s_core)) < 0) {
    LOG_ERR("SensorCoreInit : ret=%d", ret);
    return -1;
  }

  // apitest_boundary_check1();

  const char *stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
  if ((ret = SensorCoreOpenStream(s_core, stream_key, &s_stream)) < 0) {
    LOG_ERR("SensorCoreOpenStream : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  ret = GetDcpuCapabilityInfo(s_stream);
  if (ret < 0) {
    EdgeAppLibLogError(context, Fmt("GetDcpuCapabilityInfo:ret=%d\n", ret));
    PrintSensorError();
  } else {
    s_post_process_available = true;
  }

  return 0;
}

int onConfigure(char *topic, void *value, int valuesize) {
  LOG_TRACE("Inside onConfigure.");

  // apitest
  const char *context = "<onConfigure>";
  EdgeAppLibLogTrace(context, "start.");
  EdgeAppLibLogDebug(context, "start.");
  EdgeAppLibLogInfo(context, "start.");
  EdgeAppLibLogWarn(context, "start.");
  EdgeAppLibLogError(context, "start.");
  EdgeAppLibLogCritical(context, "start.");
  LOG_CRITICAL("start.");

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

  if (NeedToRunCurrentApiTestScenario()) {
    int32_t scenario_id = CurrentApiTestScenarioId();
    int32_t res = RunApiTest();
    char res_id[] = "";
    int code = 0;
    char *api_test_result = nullptr;
    asprintf(&api_test_result,
             "{\"res_info\": {\"res_id\":\"%s\",\"code\": "
             "%d,\"detail_msg\":\"apitest, %d, result, %d\"}}",
             res_id, code, scenario_id, res);
    DataExportSendState("edge_app", (void *)api_test_result,
                        strlen(api_test_result));
  }

  // Set post process parameter
  if (s_post_process_available) {
    EPPL_RESULT_CODE result = PPL_NMS_Op3pp_SetProperty(s_stream);
    if (result == E_PPL_OTHER) {
      PrintSensorError();
    }
  }

  return 0;
}

int onIterate() {
  LOG_TRACE("Inside onIterate.");

  // apitest
  const char *context = "<onIterate>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

  // apitest_boundary_check2();
  // return -1;

  // Get post process parameter
  if (s_post_process_available) {
    EPPL_RESULT_CODE result = PPL_GetProperty(s_stream);
    if (result == E_PPL_OTHER) {
      PrintSensorError();
    }
  }

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

  // apitest
  const char *context = "<onStop>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

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

  // apitest
  const char *context = "<onStart>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

  int32_t ret = -1;
  if ((ret = SensorStart(s_stream)) < 0) {
    LOG_ERR("SensorStart : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  // apitest
  // Set crop
  if ((s_crop[2] != 0) && (s_crop[3] != 0)) {
    struct EdgeAppLibSensorImageCropProperty crop;
    crop.left = s_crop[0];
    crop.top = s_crop[1];
    crop.width = s_crop[2];
    crop.height = s_crop[3];
    ret = SensorStreamSetProperty(
        s_stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &crop,
        sizeof(struct EdgeAppLibSensorImageCropProperty));
    if (ret < 0) {
      EdgeAppLibLogError(
          context, Fmt("EdgeAppLibSensorStreamSetProperty crop:ret=%d\n", ret));
      PrintSensorError();
      return -1;
    }
  }

  return 0;
}

int onDestroy() {
  LOG_TRACE("Inside onDestroy.");

  // apitest
  const char *context = "<onDestroy>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");
  fflush(stdout);

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
