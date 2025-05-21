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
#include "dcpu_param_parser.h"

#include <edgeapp/log.h>
#include <edgeapp/sensor.h>
#include <edgeapp/sm.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "apitest_util.h"
#include "parson.h"
#include "sm_utils.hpp"

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

EPPL_RESULT_CODE PPL_GetProperty(EdgeAppLibSensorStream stream) {
  // Get post process parameter
  struct EdgeAppLibSensorPostProcessParameterProperty pp_param_get = {};
  int32_t ret_get_property = EdgeAppLib::SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY, &pp_param_get,
      sizeof(struct EdgeAppLibSensorPostProcessParameterProperty));
  if (ret_get_property != 0) {
    ERR_PRINTF("EdgeAppLibSensorStreamGetProperty post process parameter %d",
               ret_get_property);
    return E_PPL_OTHER;
  }

  /* "param1"  : 32bit float (maxDetections) */
  float *tmp_p = (float *)&(pp_param_get.param[0]);
  *tmp_p = ssd_param_nms.maxDetections; /* Little endian */
  endian_reverse((uint8_t *)tmp_p);     /* Big endian */

  INFO_PRINTF("param1(float): %f", (float)*tmp_p);
  INFO_PRINTF("pp_param_get.param[0]: 0x%02x", pp_param_get.param[0]);
  INFO_PRINTF("pp_param_get.param[1]: 0x%02x", pp_param_get.param[1]);
  INFO_PRINTF("pp_param_get.param[2]: 0x%02x", pp_param_get.param[2]);
  INFO_PRINTF("pp_param_get.param[3]: 0x%02x", pp_param_get.param[3]);
}

EPPL_RESULT_CODE PPL_NMS_Op3pp_SetProperty(EdgeAppLibSensorStream stream) {
  struct EdgeAppLibSensorPostProcessParameterProperty pp_param = {};

  if (!ssd_param_nms.use_post_process_parameter) {
    ERR_PRINTF("ssd_param_nms.use_post_process_parameter  %d",
               ssd_param_nms.use_post_process_parameter);
    return E_PPL_INVALID_STATE;
  }

  /* "param1"  : 32bit float (maxDetections) */
  float *tmp_p = (float *)&(pp_param.param[0]);
  *tmp_p = ssd_param_nms.maxDetections; /* Little endian */
  endian_reverse((uint8_t *)tmp_p);     /* Big endian */

  INFO_PRINTF("param1(float): %f", (float)*tmp_p);
  INFO_PRINTF("pp_param.param[0]: 0x%02x", pp_param.param[0]);
  INFO_PRINTF("pp_param.param[1]: 0x%02x", pp_param.param[1]);
  INFO_PRINTF("pp_param.param[2]: 0x%02x", pp_param.param[2]);
  INFO_PRINTF("pp_param.param[3]: 0x%02x", pp_param.param[3]);

  /* "param2"  : 32bit float (score_threshold:person_male) */
  tmp_p = (float *)&(pp_param.param[4]);
  *tmp_p = ssd_param_nms.score_threshold_0;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param3"  : 32bit float (score_threshold:person_female) */
  tmp_p = (float *)&(pp_param.param[8]);
  *tmp_p = ssd_param_nms.score_threshold_1;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param4"  : 32bit float (score_threshold:person_unknown) */
  tmp_p = (float *)&(pp_param.param[12]);
  *tmp_p = ssd_param_nms.score_threshold_2;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param5"  : 32bit float (score_threshold:head) */
  tmp_p = (float *)&(pp_param.param[16]);
  *tmp_p = ssd_param_nms.score_threshold_3;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param6"  : 32bit float (score_threshold:face) */
  tmp_p = (float *)&(pp_param.param[20]);
  *tmp_p = ssd_param_nms.score_threshold_4;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param7"  : 32bit float (iou_threshold:person) */
  tmp_p = (float *)&(pp_param.param[24]);
  *tmp_p = ssd_param_nms.iou_threshold_0;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param8"  : 32bit float (iou_threshold:head) */
  tmp_p = (float *)&(pp_param.param[28]);
  *tmp_p = ssd_param_nms.iou_threshold_1;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param9"  : 32bit float (iou_threshold:face) */
  tmp_p = (float *)&(pp_param.param[32]);
  *tmp_p = ssd_param_nms.iou_threshold_2;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param10" : 32bit float (input_width) */
  tmp_p = (float *)&(pp_param.param[36]);
  *tmp_p = ssd_param_nms.input_Width;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  /* "param11" : 32bit float (input_height) */
  tmp_p = (float *)&(pp_param.param[40]);
  *tmp_p = ssd_param_nms.input_Height;
  endian_reverse((uint8_t *)tmp_p); /* Big endian */

  int32_t ret_set_property = EdgeAppLib::SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY, &pp_param,
      sizeof(struct EdgeAppLibSensorPostProcessParameterProperty));
  if (ret_set_property != 0) {
    ERR_PRINTF("EdgeAppLib::SensorStreamSetProperty  %d", ret_set_property);
    return E_PPL_OTHER;
  }

  return E_PPL_OK;
}

int ParsePostProcessParameter(JSON_Value *root_value, const char *value) {
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
