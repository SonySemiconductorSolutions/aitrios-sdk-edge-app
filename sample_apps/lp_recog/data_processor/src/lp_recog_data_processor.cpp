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

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_processor_api.hpp"
#include "data_processor_utils.hpp"
#include "device.h"
#include "log.h"
#include "lp_recog_utils.hpp"
#include "parson.h"
#include "sensor.h"
#include "sm_utils.hpp"
extern "C" {
#include "base64.h"
}

extern EdgeAppLibSensorStream s_stream;
#define IMX500_MODEL_NAME "lp_detection"
#define CPU_MODEL_NAME "lp_recognition"

using EdgeAppLib::SensorStreamGetProperty;

pthread_mutex_t data_processor_mutex;
EdgeAppLibSendDataType metadata_format = EdgeAppLibSendDataBase64;
char lpd_imx500_model_id[AI_MODEL_BUNDLE_ID_SIZE];
float lpr_threshold = DEFAULT_THRESHOLD_CPU;

DataProcessorCustomParam_LPD detection_param = {
    DEFAULT_MAX_DETECTIONS_IMX500, DEFAULT_THRESHOLD_IMX500,
    DEFAULT_INPUT_TENSOR_WIDTH_IMX500, DEFAULT_INPUT_TENSOR_HEIGHT_IMX500,
    true};

static DataProcessorResultCode (*extractors[])(
    JSON_Object *, DataProcessorCustomParam_LPD *) = {
    ExtractThresholdIMX500,  ExtractInputHeightIMX500,
    ExtractInputWidthIMX500, ExtractMaxDetectionsIMX500,
    ExtractBboxNormIMX500,   VerifyConstraintsIMX500};

DataProcessorResultCode DataProcessorInitialize() {
  LOG_INFO(
      "Successful call, although empty implementation of "
      "DataProcessorInitialize. App will continue to work normally");
  return kDataProcessorOk;
}

DataProcessorResultCode DataProcessorResetState() {
  LOG_INFO(
      "Successful call, although empty implementation of "
      "DataProcessorResetState. App will continue to work normally");
  return kDataProcessorOk;
}

DataProcessorResultCode DataProcessorFinalize() {
  LOG_INFO(
      "Successful call, although empty implementation of "
      "DataProcessorFinalize. App will continue to work normally");
  return kDataProcessorOk;
}
DataProcessorResultCode DataProcessorConfigure(char *config_json,
                                               char **out_config_json) {
  JSON_Value *value = json_parse_string(config_json);
  if (value == nullptr) {
    const char *error_msg = "Error parsing custom settings JSON";
    LOG_ERR("%s", error_msg);
    *out_config_json =
        GetConfigureErrorJson(ResponseCodeInvalidArgument, error_msg, "");
    return kDataProcessorInvalidParam;
  }

  JSON_Object *object = json_object(value);

  // extract parameters of AI model on IMX500
  JSON_Object *imx500_model_json_object =
      json_object_dotget_object(object, "ai_models_imx500." IMX500_MODEL_NAME);
  JSON_Object *imx500_model_params = nullptr;
  if (imx500_model_json_object == nullptr ||
      (imx500_model_params = json_object_dotget_object(
           imx500_model_json_object, "parameters")) == nullptr) {
    const char *error_msg =
        "Error accessing AI model parameters in JSON object.";
    LOG_ERR("%s", error_msg);
    *out_config_json = GetConfigureErrorJson(
        ResponseCodeInvalidArgument, error_msg,
        json_object_dotget_string(object, "res_info.res_id"));
    json_value_free(value);
    return kDataProcessorInvalidParam;
  }

  pthread_mutex_lock(&data_processor_mutex);
  DataProcessorResultCode res = kDataProcessorOk;
  DataProcessorResultCode act = kDataProcessorOk;
  for (auto &extractor : extractors) {
    if ((act = extractor(imx500_model_params, &detection_param)) !=
        kDataProcessorOk)
      res = act;
  }

  const char *ai_model_bundle_id =
      json_object_dotget_string(imx500_model_json_object, "ai_model_bundle_id");
  if (ai_model_bundle_id != nullptr) {
    snprintf(lpd_imx500_model_id, sizeof(lpd_imx500_model_id), "%s",
             ai_model_bundle_id);
  } else {
    LOG_WARN("ai_model_bundle_id not found for IMX500 model.");
  }

  pthread_mutex_unlock(&data_processor_mutex);
  // extract parameters of AI model on CPU
  JSON_Object *object_model_cpu =
      json_object_dotget_object(object, "ai_models_cpu." CPU_MODEL_NAME);
  if (object_model_cpu == nullptr) {
    const char *error_msg =
        "Error accessing AI model parameters in JSON object.";
    LOG_ERR("%s", error_msg);
    *out_config_json = GetConfigureErrorJson(
        ResponseCodeInvalidArgument, error_msg,
        json_object_dotget_string(object, "res_info.res_id"));
    json_value_free(value);
    return kDataProcessorInvalidParam;
  }

  JSON_Object *cpu_params =
      json_object_dotget_object(object_model_cpu, "parameters");
  if (cpu_params != nullptr && json_object_has_value(cpu_params, "threshold")) {
    lpr_threshold =
        static_cast<float>(json_object_dotget_number(cpu_params, "threshold"));
  } else {
    LOG_INFO("threshold not found in CPU parameters, using default value: %f",
             DEFAULT_THRESHOLD_CPU);
    lpr_threshold = DEFAULT_THRESHOLD_CPU;
  }

  // Get metadata settings
  JSON_Object *object_format =
      json_object_get_object(object, "metadata_settings");
  metadata_format =
      EdgeAppLibSendDataType(json_object_get_number(object_format, "format"));

  if (res != kDataProcessorOk)
    *out_config_json = json_serialize_to_string(value);

  json_value_free(value);
  return res;
}

DataProcessorResultCode LPDDataProcessorAnalyze(
    float *in_data, uint32_t in_size, LPDataProcessorAnalyzeParam *param) {
  LOG_TRACE("LPDDataProcessorAnalyze");
  if (in_data == nullptr || param == nullptr) {
    LOG_ERR("indata or out_data is null");
    return kDataProcessorInvalidParam;
  }

  LPAnalysisParam *lp_param = nullptr;
  if (param && param->app_specific) {
    lp_param = static_cast<LPAnalysisParam *>(param->app_specific);
  } else {
    LOG_ERR("DataProcessorAnalyzeParam is not set or app_specific is null");
    return kDataProcessorInvalidParam;
  }

  EdgeAppLibSensorImageCropProperty *roi = lp_param->roi;
  EdgeAppCore::Tensor *tensor = lp_param->tensor;

  pthread_mutex_lock(&data_processor_mutex);
  DataProcessorCustomParam_LPD analyze_params = detection_param;
  pthread_mutex_unlock(&data_processor_mutex);

  Detections *detections =
      CreateLPDetections(in_data, in_size, analyze_params, tensor);

  if (detections == NULL || detections->detection_data == NULL) {
    LOG_ERR("Error while allocating memory for detections.");
    return kDataProcessorMemoryError;
  }

  FilterByParams(&detections, analyze_params);

  // Update the ROI based on the first detection
  if (detections->num_detections > 0) {
    roi->left = detections->detection_data[0].bbox.left;
    roi->top = detections->detection_data[0].bbox.top;
    roi->width = detections->detection_data[0].bbox.right -
                 detections->detection_data[0].bbox.left;
    roi->height = detections->detection_data[0].bbox.bottom -
                  detections->detection_data[0].bbox.top;
  } else {
    LOG_INFO("No objects detected in the metadata.");
  }

  free(detections->detection_data);
  free(detections);
  return kDataProcessorOk;
}

DataProcessorResultCode LPRDataProcessorAnalyze(float *in_data,
                                                uint32_t in_size,
                                                char **out_data,
                                                uint32_t *out_size) {
  LOG_TRACE("LPRDataProcessorAnalyze");
  if (in_data == nullptr || out_data == nullptr) {
    LOG_ERR("indata or out_data is null");
    return kDataProcessorInvalidParam;
  }
  size_t num_preds = in_size / lpr_values_per_prediction / sizeof(float);
  LOG_DBG("Number of predictions: %zu (in_size: %u)", num_preds, in_size);
  if (num_preds == 0) {
    LOG_ERR("No predictions available in input data.");
    return kDataProcessorInvalidParam;
  }

  // Calculate the required size for accessing prediction data
  size_t required_floats =
      num_preds * lpr_values_per_prediction;  // 6 values per prediction
  size_t available_floats = in_size / sizeof(float);
  if (available_floats < required_floats) {
    LOG_ERR("Input data size insufficient: need %zu floats, got %zu",
            required_floats, available_floats);
    return kDataProcessorInvalidParam;
  }
  LOG_DBG("Number of predictions: %zu", num_preds);

  std::vector<Prediction> preds;
  size_t loop_count =
      std::min<size_t>(num_preds, lpr_max_predictions_to_process);
  for (size_t i = 0; i < loop_count; ++i) {
    LOG_DBG(
        "Prediction %zu: [xmin=%f, ymin=%f, xmax=%f, ymax=%f, score=%f, "
        "class_id=%d]",
        i,  // Prediction index
        in_data[lpr_prediction_coordinates_count * i + xmin_index +
                num_preds],  // xmin
        in_data[lpr_prediction_coordinates_count * i + ymin_index +
                num_preds],  // ymin
        in_data[lpr_prediction_coordinates_count * i + xmax_index +
                num_preds],  // xmax
        in_data[lpr_prediction_coordinates_count * i + ymax_index +
                num_preds],                                   // ymax
        in_data[i + num_preds * lpr_prediction_score_index],  // Score
        static_cast<int>(
            in_data[1 + i + num_preds * lpr_prediction_class_index]  // Class ID
            ));
    preds.push_back(Prediction{
        in_data[lpr_prediction_coordinates_count * i + xmin_index +
                num_preds],  // xmin
        in_data[lpr_prediction_coordinates_count * i + ymin_index +
                num_preds],  // ymin
        in_data[lpr_prediction_coordinates_count * i + xmax_index +
                num_preds],  // xmax
        in_data[lpr_prediction_coordinates_count * i + ymax_index +
                num_preds],                                   // ymax
        in_data[i + num_preds * lpr_prediction_score_index],  // Score
        static_cast<int>(
            in_data[1 + i + num_preds * lpr_prediction_class_index]  // Class ID
            )});
  }

  for (size_t j = 0; j < preds.size(); ++j) {
    LOG_DBG(
        "Prediction %zu: x_min=%.2f, y_min=%.2f, x_max=%.2f, y_max=%.2f, "
        "score=%.2f, category=%d",
        j, preds[j].x_min, preds[j].y_min, preds[j].x_max, preds[j].y_max,
        preds[j].score, preds[j].category);
  }

  // Filter predictions by score threshold
  filter_predictions_by_score(preds, lpr_threshold);

  std::string license_plate_str = interpret_predictions(preds);
  LOG_DBG("Recognized License Plate: %s", license_plate_str.c_str());

  // Add double quotes
  std::string quoted_license_plate = "\"" + license_plate_str + "\"";
  LOG_DBG("Quoted License Plate: %s", quoted_license_plate.c_str());

  // Allocate output data
  size_t datalen = quoted_license_plate.length() + 1;
  *out_data = (char *)malloc(datalen);
  if (*out_data == NULL) {
    LOG_ERR("Failed to allocate memory for output data.");
    return kDataProcessorMemoryError;
  }

  strcpy(*out_data, quoted_license_plate.c_str());
  *out_size = datalen;

  return kDataProcessorOk;
}

EdgeAppLibSendDataType DataProcessorGetDataType() { return metadata_format; }
