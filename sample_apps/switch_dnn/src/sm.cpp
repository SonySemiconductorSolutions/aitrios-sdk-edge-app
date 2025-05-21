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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_export.h"
#include "log.h"
#include "send_data.h"
#include "sensor.h"
#include "switch_dnn_analyzer.h"

#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"

#define DATA_EXPORT_AWAIT_TIMEOUT -1
#define SENSOR_GET_FRAME_TIMEOUT 5000

#define BUFSIZE 128
#define TAG "<SwitchDNN>"
#define DTDL_CODE_INVALID_ARG 3

char *state_topic = NULL;

char *GetConfigureErrorJsonSm(ResponseCode code, const char *message,
                              const char *res_id) {
  char *config_error = nullptr;
  asprintf(
      &config_error,
      "{\"res_info\": {\"res_id\":\"%s\",\"code\": %d,\"detail_msg\":\"%s\"}}",
      res_id, code, message);
  return config_error;
}

using namespace EdgeAppLib;

class Allocator : public AnalyzerBase::Allocator {
 public:
  void *Malloc(size_t size) const { return malloc(size); }
  void Free(void *ptr) const { free(ptr); }
};

static EdgeAppLibSensorCore s_core = 0;
static EdgeAppLibSensorStream s_stream = 0;
static char s_print_buffer[BUFSIZE];
static AnalyzerOd s_analyzer_od;
static AnalyzerIc s_analyzer_ic;
static Allocator s_allocator;

static char *Format(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(s_print_buffer, sizeof(s_print_buffer), format, ap);
  va_end(ap);
  return s_print_buffer;
}

static void PrintLastError() {
  uint32_t length = BUFSIZE;
  char message_buffer[BUFSIZE] = {0};
  SensorGetLastErrorString(
      EdgeAppLibSensorStatusParam::AITRIOS_SENSOR_STATUS_PARAM_MESSAGE,
      message_buffer, &length);
  EdgeAppLibLogError(TAG "GetLastError:", message_buffer);
}

static int32_t ReleaseFrame(EdgeAppLibSensorStream stream,
                            EdgeAppLibSensorFrame frame) {
  int32_t ret = 0;
  if ((ret = SensorReleaseFrame(stream, frame)) < 0) {
    EdgeAppLibLogError(TAG, "Failed to release frame");
    PrintLastError();
  }
  return ret;
}

/**
 * Check DNN (channel)
 */
static bool IsValidDNNChannel(bool is_od, EdgeAppLibSensorChannel channel) {
  struct EdgeAppLibSensorAiModelBundleIdProperty bundle_id = {};
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
  AnalyzerBase *analyzer = nullptr;
  /* Determine analyzer */
  if (is_od) {
    analyzer = &s_analyzer_od;
  } else {
    analyzer = &s_analyzer_ic;
  }
  /* Get Expected NetworkID */
  analyzer->GetNetworkId(network_id);
  /* Get Current NetworkID */
  if (0 == SensorChannelGetProperty(
               channel, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
               &bundle_id,
               sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty))) {
    EdgeAppLibLogInfo(TAG "DNN CHECK(channel):",
                      Format("OD=%d, NetworkID=%s, BundleID=%s", is_od,
                             network_id, bundle_id.ai_model_bundle_id));
    if (strncmp(network_id, bundle_id.ai_model_bundle_id, strlen(network_id)) ==
        0) {
      return true;
    }
  } else {
    EdgeAppLibLogError(TAG, "Failed to get BundleID from channel");
  }
  return false;
}

/**
 *  Compare two structures of crop
 */
static bool IsSameCrop(EdgeAppLibSensorImageCropProperty *a,
                       EdgeAppLibSensorImageCropProperty *b) {
  return (a->left == b->left) && (a->top == b->top) && (a->width == b->width) &&
         (a->height == b->height);
}

/**
 * Check CROP (stream)
 */
static bool IsValidCropStream(EdgeAppLibSensorImageCropProperty *expected,
                              EdgeAppLibSensorStream stream) {
  struct EdgeAppLibSensorImageCropProperty current = {0};

  if (0 == SensorStreamGetProperty(stream,
                                   AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                                   &current, sizeof(current))) {
    EdgeAppLibLogInfo(TAG "CROP CHECK(stream):",
                      Format("Expected(%u, %u, %u, %u)", expected->left,
                             expected->top, expected->width, expected->height));
    EdgeAppLibLogInfo(TAG "CROP CHECK(stream):",
                      Format("Current(%u, %u, %u, %u)", current.left,
                             current.top, current.width, current.height));
    if (IsSameCrop(expected, &current)) {
      return true;
    }
  } else {
    EdgeAppLibLogError(TAG, "Failed to get crop from stream");
  }
  return false;
}

static int GetDummyCropIc(EdgeAppLibSensorImageCropProperty *od,
                          EdgeAppLibSensorImageCropProperty *ic) {
  ic->left = 50;
  ic->top = 50;
  ic->width = 1000;
  ic->height = 1000;

  return 0;
}
/**
 * Get CROP for Image Classification
 */
static int GetCropIc(EdgeAppLibSensorImageCropProperty *od,
                     EdgeAppLibSensorImageCropProperty *ic) {
  /* Get analyzed data */
  AnalyzerOd::DetectionData data;
  s_analyzer_od.GetAnalyzedData(data);

  /* Get index of object whose score is highest */
  uint8_t num = data.num_of_detections_;
  uint8_t index;
  for (index = 0; index < num; ++index) {
    if (data.v_is_used_for_cropping_[index]) {
      break;
    }
  }
  if (index >= num) {
    EdgeAppLibLogError(TAG, Format("Not Detected? num=%hhu", num));
    return -1;
  }
  /* Get bbox */
  uint16_t xmin = data.v_bbox_[index].m_xmin_;
  uint16_t ymin = data.v_bbox_[index].m_ymin_;
  uint16_t xmax = data.v_bbox_[index].m_xmax_;
  uint16_t ymax = data.v_bbox_[index].m_ymax_;
  EdgeAppLibLogInfo(TAG "Detected Object:[xmin,ymin,xmax,ymax]=",
                    Format("[%hu,%hu,%hu,%hu]", xmin, ymin, xmax, ymax));

  /* Get scale */
  uint16_t width;
  uint16_t height;
  s_analyzer_od.GetInputTensorSize(width, height);
  const float scale_x = od->width / width;
  const float scale_y = od->height / height;

  /* Convert to crop */
  ic->left = xmin * scale_x;
  ic->top = ymin * scale_y;
  ic->width = (xmax - xmin) * scale_x;
  ic->height = (ymax - ymin) * scale_y;

  /* TODO */
  /* Check whether to correct or not, and apply the correction */
  /* Crop size can not be smaller than InputTensorSize */
  /* But so far there is no way to get the InputTensor size before GetFrame */

  return 0;
}

/**
 * Send Image
 */
static EdgeAppLibDataExportFuture *SendImage(EdgeAppLibSensorFrame frame) {
  /* Get Channel */
  EdgeAppLibSensorChannel channel;
  int32_t err = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  if (err) {
    EdgeAppLibLogError(TAG, "Failed to get channel (Image)");
    PrintLastError();
    return nullptr;
  }
  /* Get Image */
  struct EdgeAppLibSensorRawData raw_data;
  err = SensorChannelGetRawData(channel, &raw_data);
  if (err) {
    EdgeAppLibLogError(TAG, "Failed to get raw data (Meta)");
    PrintLastError();
    return nullptr;
  }
  /* Send Data */
  return DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
                            raw_data.address, raw_data.size,
                            raw_data.timestamp);
}

static void WaitImage(EdgeAppLibDataExportFuture *future) {
  EdgeAppLibDataExportResult response;
  response = DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
  if (response == EdgeAppLibDataExportResultSuccess) {
    EdgeAppLibLogInfo(TAG, "Send image done");
  } else {
    EdgeAppLibLogError(TAG, Format("Failed to send image:%d", response));
  }
}

/**
 * Send Data
 */
static int SendData(bool is_od, uint64_t timestamp) {
  AnalyzerBase::ResultCode result = AnalyzerBase::ResultCode::kOk;

  void *buff = nullptr;
  uint32_t size = 0;
  int32_t err = 0;

  /* Serialize */
  if (is_od) {
    result = s_analyzer_od.Serialize(&buff, &size, &s_allocator);
  } else {
    result = s_analyzer_ic.Serialize(&buff, &size, &s_allocator);
  }
  if (result != AnalyzerBase::ResultCode::kOk) {
    EdgeAppLibLogError(TAG "Failed to serialize", Format("(OD=%d)", is_od));
    err = 1;
  }
  /* Send Data */
  if (!err) {
    EdgeAppLibSendDataResult result =
        SendDataSyncMeta(buff, size, EdgeAppLibSendDataBase64, timestamp,
                         DATA_EXPORT_AWAIT_TIMEOUT);
    if (result != EdgeAppLibSendDataResultSuccess &&
        result != EdgeAppLibSendDataResultEnqueued) {
      void *err_json = NULL;
      uint32_t err_json_size = 0;
      const char *error_msg = "Error SendDataSyncMeta.";
      LOG_ERR("%s : result=%d", error_msg, result);
      err_json = GetConfigureErrorJsonSm(ResponseCodeUnavaiable, error_msg, "");
      err_json_size = strlen((const char *)err_json);
      DataExportSendState(state_topic, (void *)err_json, err_json_size);
    }

    free(buff);
  }
  return err;
}

/**
 * Send Error State
 */
static void SendErrorState(char *topic, void *info, int code,
                           const char *detail) {
  if (topic == nullptr) {
    EdgeAppLibLogError(TAG "Failed to send error state", "topic is NULL");
    return;
  }
  const char *res_info = R"({
    "res_info": {
        "res_id": "0000",
        "code": 0,
        "detail_msg": "0000"
    },
  })";
  JSON_Value *out_value = json_parse_string(res_info);
  JSON_Object *out_object = json_value_get_object(out_value);
  if (info != nullptr) {
    /* Copy res_id */
    JSON_Value *in_value = json_parse_string((const char *)info);
    JSON_Object *in_object = json_value_get_object(in_value);
    const char *res_id =
        json_object_dotget_string(in_object, "res_info.res_id");
    if (res_id) {
      json_object_dotset_string(out_object, "res_info.res_id", res_id);
    }
    json_value_free(in_value);
  }
  /* Set code */
  json_object_dotset_number(out_object, "res_info.code", code);
  /* Set detail_msg */
  json_object_dotset_string(out_object, "res_info.detail_msg", detail);
  /* Make JSON string */
  char *string = json_serialize_to_string(out_value);
  char *state =
      strdup(string); /* free called in EdgeAppLibDataExportSendState */
  /* Cleanup */
  json_free_serialized_string(string);
  json_value_free(out_value);
  /* Send state */
  if (DataExportSendState(topic, (void *)state, strlen(state)) !=
      EdgeAppLibDataExportResultSuccess) {
    free(state);
  }
}

/**
 * Send State
 */
static void SendState(char *topic, void *info) {
  JSON_Value *value = json_parse_string((const char *)info);
  JSON_Object *object = json_value_get_object(value);
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};

  /* Update Object Detection Parameters */
  AnalyzerOd::PPLParam param_od = {0};
  s_analyzer_od.GetParam(param_od);
  s_analyzer_od.GetNetworkId(network_id);
  json_object_dotset_string(
      object, "ai_models.detection_bird.ai_model_bundle_id", network_id);
  json_object_dotset_number(object,
                            "ai_models.detection_bird.param.max_detections",
                            param_od.max_detections_);
  json_object_dotset_number(object, "ai_models.detection_bird.param.threshold",
                            param_od.threshold_);
  json_object_dotset_number(object,
                            "ai_models.detection_bird.param.input_width",
                            param_od.input_width_);
  json_object_dotset_number(object,
                            "ai_models.detection_bird.param.input_height",
                            param_od.input_height_);

  /* Update Image Classification Parameters */
  AnalyzerIc::PPLParam param_ic = {0};
  s_analyzer_ic.GetParam(param_ic);
  s_analyzer_ic.GetNetworkId(network_id);
  json_object_dotset_string(
      object, "ai_models.classification_bird.ai_model_bundle_id", network_id);
  json_object_dotset_number(
      object, "ai_models.classification_bird.param.max_predictions",
      param_ic.max_predictions_);

  /* Make JSON string */
  char *string = json_serialize_to_string(value);
  char *state =
      strdup(string); /* free called in EdgeAppLibDataExportSendState */

  /* Cleanup */
  json_free_serialized_string(string);
  json_value_free(value);

  /* Send state */
  if (DataExportSendState(topic, (void *)state, strlen(state)) !=
      EdgeAppLibDataExportResultSuccess) {
    free(state);
  }
}

int onCreate() {
  EdgeAppLibLogDebug(TAG, "onCreate.");
  int32_t ret = -1;
  if ((ret = SensorCoreInit(&s_core)) < 0) {
    EdgeAppLibLogError(TAG "SensorCoreInit: ret=", Format("%d", ret));
    return -1;
  }

  const char *stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
  if ((ret = SensorCoreOpenStream(s_core, stream_key, &s_stream)) < 0) {
    EdgeAppLibLogError(TAG "SensorCoreOpenStream: ret=", Format("%d", ret));
    PrintLastError();
    return -1;
  }

  return 0;
}

int onConfigure(char *topic, void *value, int valuesize) {
  EdgeAppLibLogDebug(TAG, "onConfigure.");

  /* Check Arguments */
  if (value == nullptr) {
    EdgeAppLibLogError(TAG "[onConfigure] Invalid param:", "value is NULL");
    SendErrorState(topic, value, DTDL_CODE_INVALID_ARG, "value is NULL");
    return -1;
  }
  if (topic == nullptr) {
    EdgeAppLibLogError(TAG "[onConfigure] Invalid param:", "topic is NULL");
    SendErrorState(topic, value, DTDL_CODE_INVALID_ARG, "topic is NULL");
    free(value);
    return -1;
  }

  EdgeAppLibLogInfo(TAG "[onConfigure]",
                    Format("topic:%s, value:%s, valuesize:%i\n", topic,
                           (char *)value, valuesize));
  if (strcmp((char *)value, "") == 0) {
    EdgeAppLibLogError(TAG "[onConfigure]", "config is empty.");
    SendErrorState(topic, value, DTDL_CODE_INVALID_ARG, "Empty config");
    free(value);
    return -1;
  }

  /* Clear Configurations */
  s_analyzer_od.ClearValidatingParam();
  s_analyzer_ic.ClearValidatingParam();

  /* Configure OD */
  AnalyzerBase::ResultCode result = s_analyzer_od.ValidateParam(value);
  if (result == AnalyzerBase::ResultCode::kOk) {
    result = s_analyzer_od.SetValidatedParam(value);
    if (result == AnalyzerBase::ResultCode::kOk) {
      EdgeAppLibLogInfo(TAG "[onConfigure]", "OD: successfully configured");
    } else {
      EdgeAppLibLogError(TAG "[onConfigure]", "OD: failed to configure");
    }
  } else {
    EdgeAppLibLogError(TAG "[onConfigure]", "OD: failed to validate");
  }

  /* Configure IC */
  if (result == AnalyzerBase::ResultCode::kOk) {
    result = s_analyzer_ic.ValidateParam(value);
    if (result == AnalyzerBase::ResultCode::kOk) {
      result = s_analyzer_ic.SetValidatedParam(value);
      if (result == AnalyzerBase::ResultCode::kOk) {
        EdgeAppLibLogInfo(TAG "[onConfigure]", "IC: successfully configured");
      } else {
        EdgeAppLibLogError(TAG "[onConfigure]", "IC: failed to configure");
      }
    } else {
      EdgeAppLibLogError(TAG "[onConfigure]", "IC: failed to validate");
    }
  }
  if (result != AnalyzerBase::ResultCode::kOk) {
    s_analyzer_od.ClearValidatingParam();
    s_analyzer_ic.ClearValidatingParam();
    SendErrorState(topic, value, DTDL_CODE_INVALID_ARG, "Invalid config");
    free(value);
    return -1;
  }
  SendState(topic, value);
  free(value);
  /* Set DNN to OD */
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
  s_analyzer_od.GetNetworkId(network_id);
  struct EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  memcpy(bundle_id.ai_model_bundle_id, network_id, AI_MODEL_BUNDLE_ID_SIZE);
  if (0 != SensorStreamSetProperty(
               s_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
               &bundle_id, sizeof(bundle_id))) {
    EdgeAppLibLogError(TAG, "Failed to set BundleID(OD)");
    return -1;
  }
  return 0;
}

int onIterate() {
  EdgeAppLibLogDebug(TAG, "onIterate.");
  const uint32_t kRetryMax = 10;
  uint32_t retry = 0;
  uint64_t timestamp = 0;
  bool is_od = true;
  bool all_done = false;
  EdgeAppLibSensorCameraImageSizeProperty cameraImageSize = {};
  EdgeAppLibSensorImageCropProperty crop_od = {0};
  EdgeAppLibSensorImageCropProperty crop_ic = {0};
  if (0 == SensorStreamGetProperty(
               s_stream, AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY,
               &cameraImageSize,
               sizeof(EdgeAppLibSensorCameraImageSizeProperty)))
    crop_od = {.left = 0,
               .top = 0,
               .width = cameraImageSize.width,
               .height = cameraImageSize.height};
  else {
    EdgeAppLibLogError(TAG, "Failed to get crop property");
    return -1;
  }
  for (retry = 0; retry < kRetryMax; ++retry) {
    EdgeAppLibSensorFrame frame;
    EdgeAppLibSensorChannel channel;
    struct EdgeAppLibSensorRawData raw_data = {0};
    int32_t timeout_msec = SENSOR_GET_FRAME_TIMEOUT;
    int32_t err = 0;
    EdgeAppLibDataExportFuture *future = nullptr;
    bool done = false;

    /* Get Frame */
    err = SensorGetFrame(s_stream, &frame, timeout_msec);
    if (err) {
      EdgeAppLibLogError(TAG, "Failed to get frame");
      PrintLastError();
      EdgeAppLibSensorErrorCause cause = SensorGetLastErrorCause();
      if (cause == AITRIOS_SENSOR_ERROR_TIMEOUT) {
        continue;
      } else {
        return 0;
      }
    }

    /* Get Channel */
    err = SensorFrameGetChannelFromChannelId(
        frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel);
    if (err) {
      EdgeAppLibLogError(TAG, "Failed to get channel (Meta)");
      PrintLastError();
    }

    /* Check Channel DNN */
    if ((!err) && (!IsValidDNNChannel(is_od, channel))) {
      EdgeAppLibLogError(TAG, "IsValidDNNChannel");
      err = 1;
    }

    /* Check Stream Crop */
    EdgeAppLibSensorImageCropProperty *crop = is_od ? &crop_od : &crop_ic;
    if ((!err) && (!IsValidCropStream(crop, s_stream))) {
      /* crop might not follow is_od, so try to repair */
      (void)SensorStreamSetProperty(s_stream,
                                    AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                                    crop, sizeof(*crop));
      EdgeAppLibLogInfo(TAG, "IsValidCropStream");
      err = 1;
    }

    /* Get Raw Data */
    if (!err) {
      err = SensorChannelGetRawData(channel, &raw_data);
      if (err) {
        EdgeAppLibLogError(TAG, "Failed to get raw data (Meta)");
        PrintLastError();
      } else {
        EdgeAppLibLogInfo(
            TAG "Raw Data:",
            Format("addr=%p, size=%zu, type=%s, time=%llu", raw_data.address,
                   raw_data.size, raw_data.type, raw_data.timestamp));
      }
    }

    /* Send Image */
    future = SendImage(frame);

    /* Post Process */
    if (!err) {
      const float *address = static_cast<const float *>(raw_data.address);
      uint32_t size = raw_data.size / 4;
      AnalyzerBase::ResultCode result = AnalyzerBase::ResultCode::kOk;
      if (is_od) {
        timestamp = raw_data.timestamp;
        /* Analyze(OD) */
        result = s_analyzer_od.Analyze(address, size, timestamp);
        if (result == AnalyzerBase::ResultCode::kOk) {
          AnalyzerOd::DetectionData data;
          s_analyzer_od.GetAnalyzedData(data);
          /* Object Detected */
          if (data.num_of_detections_ > 0) {
            done = true;
            EdgeAppLibLogInfo(TAG, "Object Detected");
          } else {
            EdgeAppLibLogInfo(TAG, "No Object Detected");
            AnalyzerOd::PPLParam param = {0};
            s_analyzer_od.GetParam(param);
            if (param.force_switch_ == 1) {
              done = true;
              EdgeAppLibLogWarn(TAG, "Force Switch");
            }
          }
        } else {
          EdgeAppLibLogError(TAG, "Failed to analyze(OD)");
          err = 1;
        }
      } else {
        /* Analyze(IC) */
        result = s_analyzer_ic.Analyze(address, size, timestamp);
        if (result == AnalyzerBase::ResultCode::kOk) {
          done = true;
        } else {
          EdgeAppLibLogError(TAG, "Failed to analyze(IC)");
          err = 1;
        }
      }
    }
    if (done) {
      /* Send Data */
      err = SendData(is_od, raw_data.timestamp);
      if (!err) {
        char network_id[AI_MODEL_BUNDLE_ID_SIZE];
        if (is_od) {
          /* Set Crop for IC */
          AnalyzerOd::PPLParam param = {0};
          s_analyzer_od.GetParam(param);
          if (param.force_switch_ == 1) {
            err = GetDummyCropIc(&crop_od, &crop_ic);
          } else {
            err = GetCropIc(&crop_od, &crop_ic);
          }
          if (!err) {
            err = SensorStreamSetProperty(
                s_stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &crop_ic,
                sizeof(crop_ic));
            /* Get NetworkID for IC */
            s_analyzer_ic.GetNetworkId(network_id);
            EdgeAppLibLogError("s_analyzer_ic.GetNetworkId", network_id);
          }
        } else {
          /* Set Crop for OD */
          err = SensorStreamSetProperty(s_stream,
                                        AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                                        &crop_od, sizeof(crop_od));
          /* Get NetworkID for OD */
          s_analyzer_od.GetNetworkId(network_id);
          EdgeAppLibLogInfo("s_analyzer_od.GetNetworkId", network_id);
        }
        /* Switch DNN */
        if (!err) {
          struct EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
          memcpy(bundle_id.ai_model_bundle_id, network_id, sizeof(network_id));
          EdgeAppLibLogDebug(TAG, "SensorStreamSetProperty");
          if (0 != SensorStreamSetProperty(
                       s_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
                       &bundle_id, sizeof(bundle_id))) {
            EdgeAppLibLogError(TAG,
                               Format("Failed to set BundleID(OD=%d)", is_od));
            err = 1;
          }
        }
        if (!err) {
          if (is_od) {
            /* Change to IC mode */
            is_od = false;
          } else {
            /* OD/IC done */
            all_done = true;
          }
        }
      }
    }
    /* Wait for sending image */
    if (future) {
      WaitImage(future);
      DataExportCleanup(future);
    }
    /* Release Frame Anyway */
    if (0 != ReleaseFrame(s_stream, frame)) {
      EdgeAppLibLogError(TAG, "Failed to release frame");
    }
    /* Finish */
    if (all_done) {
      break;
    }
  }
  if (retry >= kRetryMax) {
    EdgeAppLibLogError(TAG, "Retry limit exceeded");
  }
  return 0;
}

int onStop() {
  EdgeAppLibLogDebug(TAG, "onStop.");
  int32_t ret = -1;
  if ((ret = SensorStop(s_stream)) < 0) {
    EdgeAppLibLogError(TAG "SensorStop:", Format("ret=%d", ret));
    PrintLastError();
    return -1;
  }
  return 0;
}

int onStart() {
  EdgeAppLibLogDebug(TAG, "onStart.");
  int32_t ret = -1;
  if ((ret = SensorStart(s_stream)) < 0) {
    EdgeAppLibLogError(TAG "SensorStart:", Format("ret=%d", ret));
    PrintLastError();
    return -1;
  }
  struct EdgeAppLibSensorImageCropProperty crop = {0};
  ret = SensorStreamGetProperty(
      s_stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &crop, sizeof(crop));
  if (ret != 0) {
    EdgeAppLibLogError(TAG "SensorStreamGetProperty:", Format("ret=%d", ret));
    PrintLastError();
    return -1;
  }
  EdgeAppLibLogInfo(TAG, Format("Crop:[x=%u, y=%u, w=%u, h=%u]", crop.left,
                                crop.top, crop.width, crop.height));
  return 0;
}

int onDestroy() {
  EdgeAppLibLogDebug(TAG, "onDestroy.");
  int32_t ret = -1;
  if ((ret = SensorCoreCloseStream(s_core, s_stream)) < 0) {
    EdgeAppLibLogError(TAG "SensorCoreCloseStream:", Format("ret=%d", ret));
    PrintLastError();
    return -1;
  }
  if ((ret = SensorCoreExit(s_core)) < 0) {
    EdgeAppLibLogError(TAG "SensorCoreExit:", Format("ret=%d", ret));
    return -1;
  }
  return 0;
}
