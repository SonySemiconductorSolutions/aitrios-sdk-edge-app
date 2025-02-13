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

#include <string.h>

#include <string>

#include "data_export.h"
#include "log.h"
#include "parson.h"
#include "sensor.h"

/* -------------------------------------------------------- */
/* macro define                                             */
/* -------------------------------------------------------- */
#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"

#define DATA_EXPORT_AWAIT_TIMEOUT 10000
#define SENSOR_GET_FRAME_TIMEOUT 5000
#define NETWORK_ID_LEN (6)

#define DCPU_CAPABILITYINFO_NAME_SIZE_MAX \
  (48) /* same of size : SPL_CAPABILITYINFO_NAME_SIZE_MAX (48) */
#define PPL_OT_TMP_STR_BUFSIZE \
  (128) /* Output Tensor  string size (Bbox 1factor) */
#define PPL_OT_ALL_STR_BUFSIZE \
  (12800) /* Output Tensors string size (Bbox ALL)     */

#define LOGBUGSIZE 128
#define ERR_PRINTF(fmt, ...)                        \
  {                                                 \
    char buf[LOGBUGSIZE];                           \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    EdgeAppLibLogError("", buf);                    \
  }
#define WARN_PRINTF(fmt, ...)                       \
  {                                                 \
    char buf[LOGBUGSIZE];                           \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    EdgeAppLibLogWarn("", buf);                     \
  }
#define INFO_PRINTF(fmt, ...)                       \
  {                                                 \
    char buf[LOGBUGSIZE];                           \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    EdgeAppLibLogInfo("", buf);                     \
  }
#define DBG_PRINTF(fmt, ...)                        \
  {                                                 \
    char buf[LOGBUGSIZE];                           \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    EdgeAppLibLogDebug("", buf);                    \
  }
#define VER_PRINTF(fmt, ...)                        \
  {                                                 \
    char buf[LOGBUGSIZE];                           \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    EdgeAppLibLogTrace("", buf);                    \
  }

namespace senscord_error_info {
static const char *s_level_str[] = {
    "SENSCORD_LEVEL_UNDEFINED", "SENSCORD_LEVEL_FAIL", "SENSCORD_LEVEL_FATAL"};
static const char *s_cause_str[] = {"SENSCORD_ERROR_NONE",
                                    "SENSCORD_ERROR_NOT_FOUND",
                                    "SENSCORD_ERROR_INVALID_ARGUMENT",
                                    "SENSCORD_ERROR_RESOURCE_EXHAUSTED",
                                    "SENSCORD_ERROR_PERMISSION_DENIED",
                                    "SENSCORD_ERROR_BUSY",
                                    "SENSCORD_ERROR_TIMEOUT",
                                    "SENSCORD_ERROR_CANCELLED",
                                    "SENSCORD_ERROR_ABORTED",
                                    "SENSCORD_ERROR_ALREADY_EXISTS",
                                    "SENSCORD_ERROR_INVALID_OPERATION",
                                    "SENSCORD_ERROR_OUT_OF_RANGE",
                                    "SENSCORD_ERROR_DATA_LOSS",
                                    "SENSCORD_ERROR_HARDWARE_ERROR",
                                    "SENSCORD_ERROR_NOT_SUPPORTED",
                                    "SENSCORD_ERROR_UNKNOWN"};
}  // namespace senscord_error_info

static void PrintError();
static int32_t ReleaseFrame(EdgeAppLibSensorStream stream,
                            EdgeAppLibSensorFrame frame);
static int ParseAiModelBundleId(JSON_Value *root_value, const char *value);
static int ParsePostProcessParameter(JSON_Value *root_value, const char *value);
static int ConvertNetworkId(const char *ai_model_bundle_id,
                            uint32_t &network_id);

EdgeAppLibSensorCore core = 0;
static EdgeAppLibSensorStream stream = 0;
static uint32_t network_id_ = 0;
static uint32_t network_id_2_ = 0;
static uint32_t crop_[4] = {};

typedef struct tagPPL_SsdParamNMS { /* Parameters other than
                                       use_post_process_parameter are 32bit
                                       float. (same type as T2R development
                                       environment)  */
  bool use_post_process_parameter;  /* "imx500"  : If true, use the following
                       parameters. If  false, use internal default parameters(in
                       DCPU have). */
  float maxDetections;     /* "param1"  : 32bit float (maxDetections) */
  float score_threshold_0; /* "param2"  : 32bit float
                              (score_threshold:person_male) */
  float score_threshold_1; /* "param3"  : 32bit float
                              (score_threshold:person_female) */
  float score_threshold_2; /* "param4"  : 32bit float
                              (score_threshold:person_unknown) */
  float score_threshold_3; /* "param5"  : 32bit float (score_threshold:head) */
  float score_threshold_4; /* "param6"  : 32bit float (score_threshold:face) */
  float iou_threshold_0;   /* "param7"  : 32bit float (iou_threshold:person) */
  float iou_threshold_1;   /* "param8"  : 32bit float (iou_threshold:head) */
  float iou_threshold_2;   /* "param9"  : 32bit float (iou_threshold:face) */
  float input_Width;       /* "param10" : 32bit float (input_width) */
  float input_Height;      /* "param11" : 32bit float (input_height) */
} PPL_SsdParamNMS;

static PPL_SsdParamNMS ssd_param_nms = {0};
static bool g_post_process_available = false;

/* enum */
typedef enum {
  E_PPL_OK,
  E_PPL_INVALID_PARAM,
  E_PPL_E_MEMORY_ERROR,
  E_PPL_INVALID_STATE,
  E_PPL_OTHER
} EPPL_RESULT_CODE;

typedef struct NmsOp3OutputTensor {
  float m_class;
  float m_precision;
  float m_xstart;
  float m_ystart;
  float m_xend;
  float m_yend;
  float m_yaw;
  float m_pitch;
  float m_age;
} PPL_OT;

int32_t PPL_NmsOp3Base64Text(float *p_data, uint32_t in_size, void **pp_out_buf,
                             uint32_t *p_out_size) {
  if (p_data == NULL) {
    ERR_PRINTF("PPL_NmsOp3Base64Text pdata=NULL");
    return -1;
  }

  float *p_ptr = p_data;
  int32_t ot_det_num = (int32_t) * (p_ptr++);
  char src[PPL_OT_TMP_STR_BUFSIZE];
  char *dst = (char *)malloc(PPL_OT_ALL_STR_BUFSIZE);
  int32_t wp = 0;

  if (dst == NULL) {
    ERR_PRINTF("malloc failed for OT upload string. size=%d",
               PPL_OT_ALL_STR_BUFSIZE);
    return -1;
  }

  INFO_PRINTF("[b64text] det_num=%d (%08x)", ot_det_num, ot_det_num);

  for (int32_t i = 0; i < ot_det_num; i++) {
    PPL_OT *p_ot = (PPL_OT *)p_ptr;

    INFO_PRINTF(
        "[b64text] { \"C\":%.2f, \"P\":%.2f, \"X\":%.2f, \"Y\":%.2f, "
        "\"x\":%.2f, \"y\":%.2f, \"yaw\":%.2f, \"pitch\":%.2f, \"age\":%.2f "
        "}%s",
        p_ot->m_class, p_ot->m_precision, p_ot->m_xstart, p_ot->m_ystart,
        p_ot->m_xend, p_ot->m_yend, p_ot->m_yaw, p_ot->m_pitch, p_ot->m_age,
        ((i + 1) == ot_det_num) ? "" : ",");

    snprintf(src, sizeof(src),
             "{ \"C\":%.2f, \"P\":%.2f, \"X\":%.2f, \"Y\":%.2f, \"x\":%.2f, "
             "\"y\":%.2f, \"yaw\":%.2f, \"pitch\":%.2f, \"age\":%.2f }%s",
             p_ot->m_class, p_ot->m_precision, p_ot->m_xstart, p_ot->m_ystart,
             p_ot->m_xend, p_ot->m_yend, p_ot->m_yaw, p_ot->m_pitch,
             p_ot->m_age, ((i + 1) == ot_det_num) ? "" : ",");

    for (int32_t j = 0; src[j] != '\0'; j++) {
      dst[wp++] = src[j];
    }

    p_ptr += (sizeof(PPL_OT) / 4);
  }

  dst[wp++] = '\0';

  uint32_t buf_size = wp;
  uint8_t *p_out_param = (uint8_t *)malloc(buf_size);
  if (p_out_param == NULL) {
    ERR_PRINTF("malloc failed for creating p_out_param, malloc size=%u",
               buf_size);
    free(dst);
    return -1;
  }

  memcpy(p_out_param, dst, buf_size);

  *pp_out_buf = p_out_param;
  *p_out_size = buf_size;

  free(dst);

  INFO_PRINTF("[b64text] buf_addr=%p, buf_size=%u", p_out_param, buf_size);
  return 0;
}

EPPL_RESULT_CODE PPL_NMS_Op3pp_SsdParamInit(JSON_Value *root_value,
                                            PPL_SsdParamNMS *p_ssd_param) {
  int ret = json_object_has_value(json_object(root_value), "imx500");
  if (ret) {
    p_ssd_param->use_post_process_parameter = true;
    DBG_PRINTF("%s: imx500 (use_post_process_parameter : true)", __func__);
  } else {
    p_ssd_param->use_post_process_parameter = false;
    DBG_PRINTF(
        "%s: json file does not have parameter imx500  "
        "(use_post_process_parameter : false)",
        __func__);
    return E_PPL_OK; /* DCPU default parameter = OK */
  }

  /* "imx500"  */
  ret = json_object_has_value_of_type(json_object(root_value), "imx500",
                                      JSONObject);
  if (ret) {
    JSON_Object *json_imx500 =
        json_object_get_object(json_object(root_value), "imx500");

    ret = json_object_has_value(json_imx500, "param1");
    if (ret) {
      float maxDetections =
          (float)json_object_get_number(json_imx500, "param1");
      p_ssd_param->maxDetections = maxDetections;  // float
      DBG_PRINTF("%s: param1: %f", __func__, maxDetections);
    } else {
      p_ssd_param->maxDetections = 0.0;
      DBG_PRINTF("%s json file does not have param1", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param2");
    if (ret) {
      float score_threshold_0 =
          (float)json_object_get_number(json_imx500, "param2");
      DBG_PRINTF("%s: param2: %f", __func__, score_threshold_0);
      p_ssd_param->score_threshold_0 = score_threshold_0;
    } else {
      p_ssd_param->score_threshold_0 = 0.0;
      DBG_PRINTF("%s json file does not have param2", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param3");
    if (ret) {
      float score_threshold_1 =
          (float)json_object_get_number(json_imx500, "param3");
      DBG_PRINTF("%s:  param3: %f", __func__, score_threshold_1);
      p_ssd_param->score_threshold_1 = score_threshold_1;
    } else {
      p_ssd_param->score_threshold_1 = 0.0;
      DBG_PRINTF("%s json file does not have param3", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param4");
    if (ret) {
      float score_threshold_2 =
          (float)json_object_get_number(json_imx500, "param4");
      DBG_PRINTF("%s:  param4: %f", __func__, score_threshold_2);
      p_ssd_param->score_threshold_2 = score_threshold_2;
    } else {
      p_ssd_param->score_threshold_2 = 0.0;
      DBG_PRINTF("%s json file does not have param4", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param5");
    if (ret) {
      float score_threshold_3 =
          (float)json_object_get_number(json_imx500, "param5");
      DBG_PRINTF("%s:  param5: %f", __func__, score_threshold_3);
      p_ssd_param->score_threshold_3 = score_threshold_3;
    } else {
      p_ssd_param->score_threshold_3 = 0.0;
      DBG_PRINTF("%s json file does not have param5", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param6");
    if (ret) {
      float score_threshold_4 =
          (float)json_object_get_number(json_imx500, "param6");
      DBG_PRINTF("%s:  param6: %f", __func__, score_threshold_4);
      p_ssd_param->score_threshold_4 = score_threshold_4;
    } else {
      p_ssd_param->score_threshold_4 = 0.0;
      DBG_PRINTF("%s json file does not have param6", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param7");
    if (ret) {
      float iou_threshold_0 =
          (float)json_object_get_number(json_imx500, "param7");
      DBG_PRINTF("%s:  param7: %f", __func__, iou_threshold_0);
      p_ssd_param->iou_threshold_0 = iou_threshold_0;
    } else {
      p_ssd_param->iou_threshold_0 = 0.0;
      DBG_PRINTF("%s json file does not have param7", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param8");
    if (ret) {
      float iou_threshold_1 =
          (float)json_object_get_number(json_imx500, "param8");
      DBG_PRINTF("%s:  param8: %f", __func__, iou_threshold_1);
      p_ssd_param->iou_threshold_1 = iou_threshold_1;
    } else {
      p_ssd_param->iou_threshold_1 = 0.0;
      DBG_PRINTF("%s json file does not have param8", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param9");
    if (ret) {
      float iou_threshold_2 =
          (float)json_object_get_number(json_imx500, "param9");
      DBG_PRINTF("%s:  param9: %f", __func__, iou_threshold_2);
      p_ssd_param->iou_threshold_2 = iou_threshold_2;
    } else {
      p_ssd_param->iou_threshold_2 = 0.0;
      DBG_PRINTF("%s json file does not have param9", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param10");
    if (ret) {
      float input_Width = (float)json_object_get_number(json_imx500, "param10");
      DBG_PRINTF("%s:  param10: %f", __func__, input_Width);
      p_ssd_param->input_Width = input_Width;
    } else {
      p_ssd_param->input_Width = 0.0;
      DBG_PRINTF("%s json file does not have param10", __func__);
      return E_PPL_INVALID_PARAM;
    }

    ret = json_object_has_value(json_imx500, "param11");
    if (ret) {
      float input_Height =
          (float)json_object_get_number(json_imx500, "param11");
      DBG_PRINTF("%s:  param11: %f", __func__, input_Height);
      p_ssd_param->input_Height = input_Height;
    } else {
      p_ssd_param->input_Height = 0.0;
      DBG_PRINTF("%s json file does not have param11", __func__);
      return E_PPL_INVALID_PARAM;
    }
  } else {
    ERR_PRINTF("%s json file does not have param1", __func__);
    return E_PPL_OTHER;
  }

  return E_PPL_OK;
}

void endian_reverse(uint8_t *param) {
  /* Big Endian */
  uint8_t tmp = param[0];
  param[0] = param[3];
  param[3] = tmp;
  tmp = param[1];
  param[1] = param[2];
  param[2] = tmp;
}

EPPL_RESULT_CODE PPL_NMS_Op3pp_SetProperty(EdgeAppLibSensorStream stream,
                                           PPL_SsdParamNMS *p_ssd_param) {
  struct EdgeAppLibSensorPostProcessParameterProperty pp_param = {};

  /* "param1"  : 32bit float (maxDetections) */
  float *tmp_p = (float *)&(pp_param.param[0]);
  *tmp_p = p_ssd_param->maxDetections; /* Little endian */
  endian_reverse((uint8_t *)tmp_p);    /* Big endian */

  DBG_PRINTF("param1(float): %f", (float)*tmp_p);
  DBG_PRINTF("pp_param.param[0]: 0x%02x", pp_param.param[0]);
  DBG_PRINTF("pp_param.param[1]: 0x%02x", pp_param.param[1]);
  DBG_PRINTF("pp_param.param[2]: 0x%02x", pp_param.param[2]);
  DBG_PRINTF("pp_param.param[3]: 0x%02x", pp_param.param[3]);

  /* "param2"  : 32bit float (score_threshold:person_male) */
  tmp_p = (float *)&(pp_param.param[4]);
  *tmp_p = p_ssd_param->score_threshold_0;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param3"  : 32bit float (score_threshold:person_female) */
  tmp_p = (float *)&(pp_param.param[8]);
  *tmp_p = p_ssd_param->score_threshold_1;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param4"  : 32bit float (score_threshold:person_unknown) */
  tmp_p = (float *)&(pp_param.param[12]);
  *tmp_p = p_ssd_param->score_threshold_2;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param5"  : 32bit float (score_threshold:head) */
  tmp_p = (float *)&(pp_param.param[16]);
  *tmp_p = p_ssd_param->score_threshold_3;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param6"  : 32bit float (score_threshold:face) */
  tmp_p = (float *)&(pp_param.param[20]);
  *tmp_p = p_ssd_param->score_threshold_4;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param7"  : 32bit float (iou_threshold:person) */
  tmp_p = (float *)&(pp_param.param[24]);
  *tmp_p = p_ssd_param->iou_threshold_0;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param8"  : 32bit float (iou_threshold:head) */
  tmp_p = (float *)&(pp_param.param[28]);
  *tmp_p = p_ssd_param->iou_threshold_1;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param9"  : 32bit float (iou_threshold:face) */
  tmp_p = (float *)&(pp_param.param[32]);
  *tmp_p = p_ssd_param->iou_threshold_2;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param10" : 32bit float (input_width) */
  tmp_p = (float *)&(pp_param.param[36]);
  *tmp_p = p_ssd_param->input_Width;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param11" : 32bit float (input_height) */
  tmp_p = (float *)&(pp_param.param[40]);
  *tmp_p = p_ssd_param->input_Height;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  int32_t ret_set_property = EdgeAppLibSensorStreamSetProperty(
      stream, AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY, &pp_param,
      sizeof(struct EdgeAppLibSensorPostProcessParameterProperty));
  if (ret_set_property != 0) {
    ERR_PRINTF("EdgeAppLibSensorStreamSetProperty  %d", ret_set_property);
    return E_PPL_OTHER;
  }

  return E_PPL_OK;
}

int32_t GetDcpuCapabilityInfo(EdgeAppLibSensorStream stream) {
  EdgeAppLibSensorPostProcessAvailableProperty post_process_available_prop = {
      0};
  int32_t ret = EdgeAppLibSensorStreamGetProperty(
      stream, AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY,
      &post_process_available_prop, sizeof(post_process_available_prop));
  if (ret != 0) {
    ERR_PRINTF("EdgeAppLibSensorStreamGetProperty %s %d",
               AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY, ret);
    return -1;
  }

  if (!post_process_available_prop.is_available) {
    ERR_PRINTF("post_process_available_prop.is_available :%d",
               post_process_available_prop.is_available);
    return -1;
  }

  return 0;
}

int onCreate() {
  const char *context = "<onCreate>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

  int32_t ret = -1;

  if ((ret = EdgeAppLibSensorCoreInit(&core)) < 0) {
    EdgeAppLibLogError(context, std::string("EdgeAppLibSensorCoreInit : ret=" +
                                            std::to_string(ret))
                                    .c_str());
    return -1;
  }

  const char *stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
  if ((ret = EdgeAppLibSensorCoreOpenStream(core, stream_key, &stream)) < 0) {
    EdgeAppLibLogError(context,
                       std::string("EdgeAppLibSensorCoreOpenStream : ret=" +
                                   std::to_string(ret))
                           .c_str());
    PrintError();
    return -1;
  }

  ret = GetDcpuCapabilityInfo(stream);
  if (ret < 0) {
    ERR_PRINTF("GetDcpuCapabilityInfo:%d", ret);
    PrintError();
  } else {
    g_post_process_available = true;
  }

  return 0;
}

int onConfigure(char *topic, void *value, int valuesize) {
  const char *context = "<onConfigure>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

  if (value == NULL) {
    EdgeAppLibLogError(context, "Invalid param : value=NULL");
    return -1;
  }
  EdgeAppLibLogInfo(context,
                    std::string("topic:" + std::string(topic) +
                                "\nvalue:" + std::string((char *)value) +
                                "\nvaluesize:" + std::to_string(valuesize))
                        .c_str());

  if (std::string((const char *)value).empty()) {
    EdgeAppLibLogInfo(context, "ConfigurationCallback: config is empty.");
    free(value);
    return -1;
  }

  // Parse custom_settings in configuration json
  JSON_Value *root_value = json_parse_string((const char *)value);
  JSON_Value_Type type = json_value_get_type(root_value);
  if (type != JSONObject) {
    json_value_free(root_value);
    EdgeAppLibLogError(context, "Invalid configuration");
    free(value);
    return -1;
  }

  // Parse custom_settings in configuration json and get network_id
  if (ParseAiModelBundleId(root_value, (const char *)value) < 0) {
    EdgeAppLibLogError(context, "ParseAiModelBundleId error");
    free(value);
    return -1;
  }

  // Parse custom_settings in configuration json and get post process
  // parameter
  if (ParsePostProcessParameter(root_value, (const char *)value) < 0) {
    EdgeAppLibLogInfo(context, "ParsePostProcessParameter nothing");
  }

  json_value_free(root_value);

  // Set dnn
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;
  ai_model_bundle.ai_model_bundle_id = network_id_;
  int32_t ret = EdgeAppLibSensorStreamSetProperty(
      stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY, &ai_model_bundle,
      sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty));
  if (ret < 0) {
    EdgeAppLibLogError(context,
                       std::string("EdgeAppLibSensorStreamSetProperty : ret=" +
                                   std::to_string(ret))
                           .c_str());
    PrintError();
    free(value);
    return -1;
  }

  // Set post process parameter
  if (g_post_process_available && ssd_param_nms.use_post_process_parameter) {
    PPL_NMS_Op3pp_SetProperty(stream, &ssd_param_nms);
  }

  free(value);
  return 0;
}

static EdgeAppLibDataExportFuture *sendInputTensor(
    EdgeAppLibSensorFrame *frame) {
  const char *context = "<sendInputTensor>";
  int32_t ret = -1;
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;

  // Input image
  EdgeAppLibSensorChannel channel;
  if ((ret = EdgeAppLibSensorFrameGetChannelFromChannelId(
           *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel)) <
      0) {
    EdgeAppLibLogError(
        context,
        std::string(
            "EdgeAppLibSensorFrameGetChannelFromChannelId input image : ret=" +
            std::to_string(ret))
            .c_str());
    PrintError();
    return nullptr;
  }

  if ((ret = EdgeAppLibSensorChannelGetProperty(
           channel, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
           &ai_model_bundle,
           sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty))) < 0) {
    EdgeAppLibLogError(context,
                       std::string("EdgeAppLibSensorChannelGetProperty : ret=" +
                                   std::to_string(ret))
                           .c_str());
    PrintError();
    return nullptr;
  }
  EdgeAppLibLogInfo(
      context, std::string("EdgeAppLibSensorChannelGetProperty dnn:" +
                           std::to_string(ai_model_bundle.ai_model_bundle_id))
                   .c_str());

  struct EdgeAppLibSensorRawData input_raw_data;
  if ((ret = EdgeAppLibSensorChannelGetRawData(channel, &input_raw_data)) < 0) {
    EdgeAppLibLogError(
        context,
        std::string("EdgeAppLibSensorChannelGetRawData input image : ret=" +
                    std::to_string(ret))
            .c_str());
    PrintError();
    return nullptr;
  }

  EdgeAppLibLogInfo(
      context, std::string("input_raw_data.address:" +
                           std::to_string((uint32_t)input_raw_data.address))
                   .c_str());
  EdgeAppLibLogInfo(context, std::string("input_raw_data.size:" +
                                         std::to_string(input_raw_data.size))
                                 .c_str());
  EdgeAppLibLogInfo(context,
                    std::string("input_raw_data.timestamp:" +
                                std::to_string(input_raw_data.timestamp))
                        .c_str());
  EdgeAppLibLogInfo(context, std::string("input_raw_data.type:" +
                                         std::string(input_raw_data.type))
                                 .c_str());
#if 0  // for one-pass test
  void *p_input_buf = malloc(input_raw_data.size);
  if (p_input_buf == NULL) {
    EdgeAppLibLogError(context, std::string("input image malloc failed, malloc size=" +
                               std::to_string(input_raw_data.size))
                       .c_str());
    ReleaseFrame(stream, frame);
    return -1;
  }
  memcpy(p_input_buf, input_raw_data.address, input_raw_data.size);
#endif
  EdgeAppLibDataExportFuture *future_input = EdgeAppLibDataExportSendData(
      (char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw, input_raw_data.address,
      input_raw_data.size, input_raw_data.timestamp);
  return future_input;
}

static void sendMetadata(EdgeAppLibSensorFrame *frame) {
  const char *context = "<sendMetadata>";
  int32_t ret = -1;
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;

  // Output data
  EdgeAppLibSensorChannel channel;
  if ((ret = EdgeAppLibSensorFrameGetChannelFromChannelId(
           *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel)) < 0) {
    EdgeAppLibLogError(
        context,
        std::string(
            "EdgeAppLibSensorFrameGetChannelFromChannelId output : ret=" +
            std::to_string(ret))
            .c_str());
    PrintError();
    return;
  }

  // for full wasm test
  if ((ret = EdgeAppLibSensorChannelGetProperty(
           channel, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
           &ai_model_bundle,
           sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty))) < 0) {
    EdgeAppLibLogError(context,
                       std::string("EdgeAppLibSensorChannelGetProperty : ret=" +
                                   std::to_string(ret))
                           .c_str());
    PrintError();
    return;
  }
  EdgeAppLibLogInfo(
      context, std::string("EdgeAppLibSensorChannelGetProperty dnn:" +
                           std::to_string(ai_model_bundle.ai_model_bundle_id))
                   .c_str());

  struct EdgeAppLibSensorRawData output_raw_data;
  if ((ret = EdgeAppLibSensorChannelGetRawData(channel, &output_raw_data)) <
      0) {
    EdgeAppLibLogError(
        context, std::string("EdgeAppLibSensorChannelGetRawData output : ret=" +
                             std::to_string(ret))
                     .c_str());
    PrintError();
    return;
  }

  EdgeAppLibLogInfo(
      context, std::string("output_raw_data.address:" +
                           std::to_string((uint32_t)output_raw_data.address))
                   .c_str());
  EdgeAppLibLogInfo(context, std::string("output_raw_data.size:" +
                                         std::to_string(output_raw_data.size))
                                 .c_str());
  EdgeAppLibLogInfo(context,
                    std::string("output_raw_data.timestamp:" +
                                std::to_string(output_raw_data.timestamp))
                        .c_str());
  EdgeAppLibLogInfo(context, std::string("output_raw_data.type:" +
                                         std::string(output_raw_data.type))
                                 .c_str());

  if (g_post_process_available && ssd_param_nms.use_post_process_parameter) {
    void *p_out_buf = NULL;
    uint32_t p_out_size = 0;
    int32_t nms_ret =
        PPL_NmsOp3Base64Text((float *)output_raw_data.address,
                             output_raw_data.size, &p_out_buf, &p_out_size);
    INFO_PRINTF("PPL_NmsOp3Base64Text ret %d", nms_ret);

    // EdgeAppLibDataExportFuture *future_output = EdgeAppLibDataExportSendData(
    //     (char *)PORTNAME_META, EdgeAppLibDataExportMetadata, p_out_buf,
    //     p_out_size, output_raw_data.timestamp);

    // ade_res = EdgeAppLibDataExportAwait(future_output, -1);
    free(p_out_buf);
    // ade_res = EdgeAppLibDataExportCleanup(future_output);
  }

  if (1) {
    void *p_out_buf = malloc(output_raw_data.size);
    if (p_out_buf == NULL) {
      EdgeAppLibLogError(context,
                         std::string("output data malloc failed, malloc size=" +
                                     std::to_string(output_raw_data.size))
                             .c_str());
      return;
    }

    memcpy(p_out_buf, output_raw_data.address, output_raw_data.size);
    EdgeAppLibDataExportFuture *future_output = EdgeAppLibDataExportSendData(
        (char *)PORTNAME_META, EdgeAppLibDataExportMetadata, p_out_buf,
        output_raw_data.size, output_raw_data.timestamp);

    EdgeAppLibDataExportResult ade_res =
        EdgeAppLibDataExportAwait(future_output, DATA_EXPORT_AWAIT_TIMEOUT);
    free(p_out_buf);
    ade_res = EdgeAppLibDataExportCleanup(future_output);
  }
}

int onIterate() {
  const char *context = "<onIterate>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

  int32_t ret = -1;
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;

  if ((ret = EdgeAppLibSensorStreamGetProperty(
           stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
           &ai_model_bundle,
           sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty))) < 0) {
    EdgeAppLibLogError(context,
                       std::string("EdgeAppLibSensorStreamGetProperty : ret=" +
                                   std::to_string(ret))
                           .c_str());
    PrintError();
    return -1;
  }
  EdgeAppLibLogInfo(
      context, std::string("EdgeAppLibSensorStreamGetProperty dnn:" +
                           std::to_string(ai_model_bundle.ai_model_bundle_id))
                   .c_str());

  // Get post process parameter
  struct EdgeAppLibSensorPostProcessParameterProperty pp_param_get = {};
  int32_t ret_get_property = EdgeAppLibSensorStreamGetProperty(
      stream, AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY, &pp_param_get,
      sizeof(struct EdgeAppLibSensorPostProcessParameterProperty));
  if (ret_get_property != 0) {
    ERR_PRINTF("EdgeAppLibSensorStreamGetProperty post process parameter %d",
               ret_get_property);
    PrintError();
    return -1;
  }

  EdgeAppLibSensorFrame frame;
  int32_t timeout_msec = SENSOR_GET_FRAME_TIMEOUT;
  if ((ret = EdgeAppLibSensorGetFrame(stream, &frame, timeout_msec)) < 0) {
    EdgeAppLibLogError(context, std::string("EdgeAppLibSensorGetFrame : ret=" +
                                            std::to_string(ret))
                                    .c_str());
    PrintError();
    return EdgeAppLibSensorGetLastErrorCause() == AITRIOS_SENSOR_ERROR_TIMEOUT
               ? 0
               : -1;
  }

  EdgeAppLibDataExportFuture *future_input = sendInputTensor(&frame);
  if (future_input) {
    EdgeAppLibDataExportResult ade_res =
        EdgeAppLibDataExportAwait(future_input, DATA_EXPORT_AWAIT_TIMEOUT);
    ade_res = EdgeAppLibDataExportCleanup(future_input);
  }

  sendMetadata(&frame);

  if (ReleaseFrame(stream, frame) < 0) {
    return -1;
  }

  return 0;
}

int onStop() {
  const char *context = "<onStop>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

  int32_t ret = -1;
  if ((ret = EdgeAppLibSensorStop(stream)) < 0) {
    EdgeAppLibLogError(context, std::string("EdgeAppLibSensorStop : ret=" +
                                            std::to_string(ret))
                                    .c_str());
    PrintError();
    return -1;
  }

  return 0;
}

int onStart() {
  const char *context = "<onStart>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");

  int32_t ret = -1;
  if ((ret = EdgeAppLibSensorStart(stream)) < 0) {
    EdgeAppLibLogError(context, std::string("EdgeAppLibSensorStart : ret=" +
                                            std::to_string(ret))
                                    .c_str());
    PrintError();
    return -1;
  }

  // Set dnn
  if (network_id_2_ != 0) {
    struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;
    ai_model_bundle.ai_model_bundle_id = network_id_2_;
    ret = EdgeAppLibSensorStreamSetProperty(
        stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
        &ai_model_bundle,
        sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty));
    if (ret < 0) {
      EdgeAppLibLogError(
          context, std::string("EdgeAppLibSensorStreamSetProperty dnn : ret=" +
                               std::to_string(ret))
                       .c_str());
      PrintError();
      return -1;
    }
  }
#if 0  // for full wasm test
  // Set dnn index
  struct EdgeAppLibSensorAiModelIndexProperty ai_model_index;
  ai_model_index.ai_model_index = 0;
  ret = EdgeAppLibSensorStreamSetProperty(
      stream, AITRIOS_SENSOR_AI_MODEL_INDEX_PROPERTY_KEY, &ai_model_index,
      sizeof(struct EdgeAppLibSensorAiModelIndexProperty));
  if (ret < 0) {
    EdgeAppLibLogError(context, std::string("EdgeAppLibSensorStreamSetProperty dnn index : ret=" +
                               std::to_string(ret))
                       .c_str());
    PrintError();
    //return -1;
  }
#endif
  // Set crop
  if ((crop_[2] != 0) && (crop_[3] != 0)) {
    struct EdgeAppLibSensorImageCropProperty crop;
    crop.left = crop_[0];
    crop.top = crop_[1];
    crop.width = crop_[2];
    crop.height = crop_[3];
    ret = EdgeAppLibSensorStreamSetProperty(
        stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &crop,
        sizeof(struct EdgeAppLibSensorImageCropProperty));
    if (ret < 0) {
      EdgeAppLibLogError(
          context, std::string("EdgeAppLibSensorStreamSetProperty crop : ret=" +
                               std::to_string(ret))
                       .c_str());
      PrintError();
      return -1;
    }
  }

  return 0;
}

int onDestroy() {
  const char *context = "<onDestroy>";
  EdgeAppLibLogTrace(context, "start.");
  // EdgeAppLibLogDebug(context, "start.");
  // EdgeAppLibLogInfo(context, "start.");
  // EdgeAppLibLogWarn(context, "start.");
  // EdgeAppLibLogError(context, "start.");
  // EdgeAppLibLogCritical(context, "start.");
  fflush(stdout);

  int32_t ret = -1;
  if ((ret = EdgeAppLibSensorCoreCloseStream(core, stream)) < 0) {
    EdgeAppLibLogError(context,
                       std::string("EdgeAppLibSensorCoreCloseStream : ret=" +
                                   std::to_string(ret))
                           .c_str());
    PrintError();
    return -1;
  }

  if ((ret = EdgeAppLibSensorCoreExit(core)) < 0) {
    EdgeAppLibLogError(context, std::string("EdgeAppLibSensorCoreExit : ret=" +
                                            std::to_string(ret))
                                    .c_str());
    return -1;
  }

  return 0;
}

static void PrintError() {
  std::string level_str =
      senscord_error_info::s_level_str[EdgeAppLibSensorGetLastErrorLevel()];
  std::string cause_str =
      senscord_error_info::s_cause_str[EdgeAppLibSensorGetLastErrorCause()];
  uint32_t length = 64;
  char *buffer = (char *)malloc(length);
  EdgeAppLibSensorGetLastErrorString(
      EdgeAppLibSensorStatusParam::AITRIOS_SENSOR_STATUS_PARAM_MESSAGE, buffer,
      &length);
  EdgeAppLibLogInfo(
      "[EdgeAppLibSensor]",
      std::string("status:\n - level  : " + level_str +
                  "\n - cause  : " + cause_str + "\n - message: " + buffer)
          .c_str());
  free(buffer);
}

static int32_t ReleaseFrame(EdgeAppLibSensorStream stream,
                            EdgeAppLibSensorFrame frame) {
  int32_t ret;
  if ((ret = EdgeAppLibSensorReleaseFrame(stream, frame)) < 0) {
    EdgeAppLibLogError(
        "<ReleaseFrame>",
        std::string("EdgeAppLibSensorReleaseFrame : ret=" + std::to_string(ret))
            .c_str());
    PrintError();
  }
  return ret;
}

static int ParseAiModelBundleId(JSON_Value *root_value, const char *value) {
  const char *context = "[ParseAiModelBundleId]";

  const char *ai_model_bundle_id_str = nullptr;
  const char *ai_model_bundle_id_2_str = nullptr;
  uint32_t left = 0;
  uint32_t top = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  if (json_object_has_value(json_object(root_value), "ai_models")) {
    if (json_object_has_value_of_type(json_object(root_value), "ai_models",
                                      JSONObject)) {
      JSON_Object *ai_models =
          json_object_get_object(json_object(root_value), "ai_models");
      if (json_object_has_value(ai_models, "one_pass_model")) {
        if (json_object_has_value_of_type(ai_models, "one_pass_model",
                                          JSONObject)) {
          JSON_Object *one_pass_model =
              json_object_get_object(ai_models, "one_pass_model");
          // ai model bundle id
          if (json_object_has_value(one_pass_model, "ai_model_bundle_id")) {
            ai_model_bundle_id_str =
                json_object_get_string(one_pass_model, "ai_model_bundle_id");
          } else {
            EdgeAppLibLogError(context,
                               "json file does not have ai_model_bundle_id");
            return -1;
          }
          // second ai model bundle id
          if (json_object_has_value(one_pass_model, "ai_model_bundle_id_2")) {
            ai_model_bundle_id_2_str =
                json_object_get_string(one_pass_model, "ai_model_bundle_id_2");
          } else {
            EdgeAppLibLogInfo(context,
                              "json file does not have ai_model_bundle_id_2");
          }
          // crop
          if (json_object_has_value_of_type(one_pass_model, "crop",
                                            JSONObject)) {
            JSON_Object *crop = json_object_get_object(one_pass_model, "crop");
            if (json_object_has_value(crop, "left")) {
              left = json_object_get_number(crop, "left");
            } else {
              EdgeAppLibLogError(context, "json file does not have left");
              return -1;
            }
            if (json_object_has_value(crop, "top")) {
              top = json_object_get_number(crop, "top");
            } else {
              EdgeAppLibLogError(context, "json file does not have top");
              return -1;
            }
            if (json_object_has_value(crop, "width")) {
              width = json_object_get_number(crop, "width");
            } else {
              EdgeAppLibLogError(context, "json file does not have width");
              return -1;
            }
            if (json_object_has_value(crop, "height")) {
              height = json_object_get_number(crop, "height");
            } else {
              EdgeAppLibLogError(context, "json file does not have height");
              return -1;
            }
            crop_[0] = left;
            crop_[1] = top;
            crop_[2] = width;
            crop_[3] = height;

          } else {
            EdgeAppLibLogInfo(context, "json file does not have crop");
            for (int i = 0; i < 4; i += 1) {
              crop_[i] = 0;
            }
          }
        } else {
          EdgeAppLibLogError(context, "one_pass_model is not JSONObject");
          return -1;
        }
      } else {
        EdgeAppLibLogError(context, "json file does not have one_pass_model");
        return -1;
      }
    } else {
      EdgeAppLibLogError(context, "ai_models is not JSONObject");
      return -1;
    }
  } else {
    EdgeAppLibLogError(context, "json file does not have ai_models");
    return -1;
  }

  int ret = -1;
  // id 1
  if (ai_model_bundle_id_str != nullptr) {
    ret = ConvertNetworkId(ai_model_bundle_id_str, network_id_);
    if (ret < 0) {
      EdgeAppLibLogError(context, " ai_model_bundle_id is invalid");
      return -1;
    }
    EdgeAppLibLogInfo(context, std::string("ai_model_bundle_id is " +
                                           std::to_string(network_id_))
                                   .c_str());
  } else {
    EdgeAppLibLogError(context, " ai_model_bundle_id is invalid");
    return -1;
  }

  // id_2
  if (ai_model_bundle_id_2_str != nullptr) {
    ret = ConvertNetworkId(ai_model_bundle_id_2_str, network_id_2_);
    if (ret < 0) {
      EdgeAppLibLogError(context, " ai_model_bundle_id_2 is invalid");
      return -1;
    }
    EdgeAppLibLogInfo(context, std::string("ai_model_bundle_id_2 is " +
                                           std::to_string(network_id_2_))
                                   .c_str());
  } else {
    network_id_2_ = 0;
  }

  return 0;
}

static int ParsePostProcessParameter(JSON_Value *root_value,
                                     const char *value) {
  DBG_PRINTF("ParsePostProcessParameter");
  JSON_Value_Type type = json_value_get_type(root_value);
  if (type != JSONObject) {
    json_value_free(root_value);
    ERR_PRINTF("ParsePostProcessParameter Invalid configuration");
    return -1;
  }

  EPPL_RESULT_CODE ret = PPL_NMS_Op3pp_SsdParamInit(root_value, &ssd_param_nms);
  if (ret != E_PPL_OK) {
    INFO_PRINTF("ParsePostProcessParameter Err[%d] use (%d)", ret,
                ssd_param_nms.use_post_process_parameter);
  } else {
    DBG_PRINTF("ParsePostProcessParameter OK use (%d)",
               ssd_param_nms.use_post_process_parameter);
  }

  return 0;
}

int ConvertNetworkId(const char *ai_model_bundle_id, uint32_t &network_id) {
  const char *context = "[ConvertNetworkId]";
  int ret = strnlen(ai_model_bundle_id, 7);
  if (ret != NETWORK_ID_LEN) {
    EdgeAppLibLogError(context, "ai_model_bundle_id must be six characters");
    return -1;
  }
  char *end;
  network_id = strtol(ai_model_bundle_id, &end, 16);
  if (network_id == 0) {
    EdgeAppLibLogError(context, "ai_model_bundle_id is invalid");
    return -1;
  }
  return 0;
}
