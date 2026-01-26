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
uint32_t lpr_input_tensor_width = 0;
uint32_t lpr_input_tensor_height = 0;

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
  if (cpu_params != nullptr && json_object_has_value(cpu_params, "width")) {
    lpr_input_tensor_width =
        static_cast<uint32_t>(json_object_dotget_number(cpu_params, "width"));
  } else {
    LOG_INFO("width not found in CPU parameters, using 0");
    lpr_input_tensor_width = 0;
  }
  if (cpu_params != nullptr && json_object_has_value(cpu_params, "height")) {
    lpr_input_tensor_height =
        static_cast<uint32_t>(json_object_dotget_number(cpu_params, "height"));
  } else {
    LOG_INFO("height not found in CPU parameters, using 0");
    lpr_input_tensor_height = 0;
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
    const std::vector<EdgeAppCore::Tensor> &tensors,
    LPDataProcessorAnalyzeParam *param) {
  LOG_TRACE("LPDDataProcessorAnalyze");
  if (tensors.empty() || param == nullptr) {
    LOG_ERR("tensors empty or param is null");
    return kDataProcessorInvalidParam;
  }

  LOG_DBG("LPD model returned %zu tensors", tensors.size());

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

  // Handle different tensor configurations flexibly
  Detections *detections = nullptr;

  if (tensors.size() >= 4) {
    // Multiple tensors - use new function
    detections = CreateLPDetectionsFromTensors(tensors, analyze_params);
  } else if (tensors.size() == 1) {
    // Single flattened tensor
    float *in_data = static_cast<float *>(tensors[0].data);
    uint32_t in_size = tensors[0].size;
    detections = CreateLPDetections(in_data, in_size, analyze_params, tensor);
  } else {
    LOG_ERR("Unsupported tensor configuration: %zu tensors", tensors.size());
    return kDataProcessorInvalidParam;
  }

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
    roi->height = (detections->detection_data[0].bbox.bottom -
                   detections->detection_data[0].bbox.top) /
                  2;  // Only half height for LPR
  } else {
    LOG_INFO("No objects detected in the metadata.");
  }

  free(detections->detection_data);
  free(detections);
  return kDataProcessorOk;
}

// Legacy signature for backward compatibility
DataProcessorResultCode LPDDataProcessorAnalyze(
    float *in_data, uint32_t in_size, LPDataProcessorAnalyzeParam *param) {
  // Convert raw float data to tensor vector
  std::vector<EdgeAppCore::Tensor> tensors;
  if (in_data != nullptr && in_size > 0) {
    EdgeAppCore::Tensor tensor;
    tensor.data = in_data;
    tensor.size = in_size;
    tensor.type = EdgeAppCore::TensorTypeFloat32;
    tensors.push_back(tensor);
  }
  return LPDDataProcessorAnalyze(tensors, param);
}

// Legacy signature for backward compatibility
DataProcessorResultCode LPRDataProcessorAnalyze(float *in_data,
                                                uint32_t in_size,
                                                char **out_data,
                                                uint32_t *out_size) {
  // Convert raw float data to tensor vector
  std::vector<EdgeAppCore::Tensor> tensors;
  if (in_data != nullptr && in_size > 0) {
    EdgeAppCore::Tensor tensor;
    tensor.data = in_data;
    tensor.size = in_size;
    tensor.type = EdgeAppCore::TensorTypeFloat32;
    tensors.push_back(tensor);
  }
  return LPRDataProcessorAnalyze(tensors, out_data, out_size);
}

DataProcessorResultCode LPRDataProcessorAnalyze(
    const std::vector<EdgeAppCore::Tensor> &tensors, char **out_data,
    uint32_t *out_size) {
  LOG_TRACE("LPRDataProcessorAnalyze");
  if (tensors.empty() || out_data == nullptr) {
    LOG_ERR("tensors empty or out_data is null");
    return kDataProcessorInvalidParam;
  }
  LOG_DBG("LPR model returned %zu tensors", tensors.size());

  std::vector<Prediction> preds;

  // Handle different tensor configurations flexibly
  if (tensors.size() == 1) {
    // Single flattened tensor - use original logic
    const auto &tensor = tensors[0];
    if (tensor.data == nullptr) {
      LOG_ERR("Tensor data is null");
      return kDataProcessorInvalidParam;
    }
    float *in_data = static_cast<float *>(tensor.data);
    uint32_t in_size = tensor.size;

    // lpr_values_per_prediction is now 28 (sequence_length)
    // Total tensor size should be: sequence_length * vocab_size = 28 * 248
    size_t expected_vocab_size = 248;
    size_t sequence_length = tensor.size / expected_vocab_size / sizeof(float);
    LOG_DBG(
        "Sequence length: %zu (tensors.size: %u bytes"
        "%zu)",
        sequence_length, tensor.size);

    if (sequence_length == 0) {
      LOG_ERR("Invalid sequence length calculated from input data.");
      return kDataProcessorInvalidParam;
    }

    // Verify that we have the expected tensor size: 28 * 248 floats

    size_t expected_total_floats =
        lpr_values_per_prediction * expected_vocab_size;
    size_t available_floats = in_size / sizeof(float);

    if (available_floats < expected_total_floats) {
      LOG_ERR("Input data size insufficient: need %zu floats (28*248), got %zu",
              expected_total_floats, available_floats);
      return kDataProcessorInvalidParam;
    }

    // Get character dictionary for OCR decoding
    const std::vector<std::string> &char_map = GetLPCategoriesOCR();
    LOG_DBG("Character dictionary size: %zu", char_map.size());

    // Determine tensor layout
    // TFLite output is row-major: if shape is [28, 248], data is stored as:
    // [t0_c0, t0_c1, ..., t0_c247, t1_c0, t1_c1, ..., t1_c247, ...]
    // where t=timestep, c=character class
    size_t vocab_size = char_map.size();  // 248

    LOG_DBG("Tensor layout: sequence_length=%zu, vocab_size=%zu",
            sequence_length, vocab_size);

    // CTC Greedy Decoding: find best character at each timestep
    std::string license_plate_str = "";
    std::vector<int> best_path;
    std::vector<float> confidences;

    // Step 1: Find highest probability character at each position
    // TFLite output is row-major: data[t * vocab_size + c]
    // where t=timestep, c=character class index
    for (size_t t = 0; t < sequence_length; t++) {
      // Find max probability across all character classes for this position
      float max_prob = -1.0f;
      int best_idx = 0;

      std::string debug_str = "";
      for (size_t c = 0; c < vocab_size; c++) {
        // Row-major access: row=t, col=c, index = t * vocab_size + c
        float prob = in_data[t * vocab_size + c];
        debug_str.append(std::to_string(prob) + " ");
        if (prob > max_prob) {
          max_prob = prob;
          best_idx = static_cast<int>(c);
        }
        if (c % 11 == 0 && c != 0) {
          LOG_TRACE("%zu:%s", t, debug_str.c_str());
          debug_str = "";
        }
      }
      // LOG_TRACE("%zu:[%s]", t, debug_str.c_str());

      best_path.push_back(best_idx);
      confidences.push_back(max_prob);

      // Log top prediction for each position
      std::string char_display =
          (best_idx < char_map.size()) ? char_map[best_idx] : "?";
      LOG_DBG("Position %2zu: idx=%d, char='%s', confidence=%.3f", t, best_idx,
              char_display.c_str(), max_prob);
    }

    // Step 2: CTC collapse - remove repeated characters and skip "?" (blank
    // token)
    int prev_idx = -1;
    for (size_t t = 0; t < best_path.size(); t++) {
      int idx = best_path[t];

      // Skip if same as previous (CTC collapse rule)
      if (idx == prev_idx) {
        continue;
      }
      prev_idx = idx;

      // Skip blank token (index 0 which is "?")
      if (idx == 0) {
        continue;
      }

      // Append character to result
      if (idx < char_map.size()) {
        const std::string &ch = char_map[idx];
        // Skip "?" characters
        if (ch != "?") {
          license_plate_str += ch;
        }
      }
    }

    LOG_INFO("Decoded license plate text: '%s'", license_plate_str.c_str());

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
  }
  return kDataProcessorOk;
}

EdgeAppLibSendDataType DataProcessorGetDataType() { return metadata_format; }
