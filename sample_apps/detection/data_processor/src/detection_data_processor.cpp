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

#include <edgeapp/log.h>
#include <edgeapp/sensor.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_processor_api.hpp"
#include "data_processor_utils.hpp"
#include "detection_utils.hpp"
#include "parson.h"
#include "sm_utils.hpp"
extern "C" {
#include "base64.h"
}

extern EdgeAppLibSensorStream s_stream;
#define MODEL_NAME "detection"

using EdgeAppLib::SensorStreamGetProperty;

pthread_mutex_t data_processor_mutex;
EdgeAppLibSendDataType metadata_format = EdgeAppLibSendDataBase64;
Area area = {};
bool send_area_counts = false;

DataProcessorCustomParam detection_param = {DEFAULT_MAX_DETECTIONS,
                                            DEFAULT_THRESHOLD,
                                            DEFAULT_INPUT_TENSOR_WIDTH,
                                            DEFAULT_INPUT_TENSOR_HEIGHT,
                                            "yxyx",
                                            true,
                                            "cls_score"};

static DataProcessorResultCode (*extractors[])(JSON_Object *,
                                               DataProcessorCustomParam *) = {
    ExtractThreshold,     ExtractInputHeight, ExtractInputWidth,
    ExtractMaxDetections, ExtractBboxOrder,   ExtractBboxNorm,
    ExtractClassOrder,    VerifyConstraints};

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

  // extract parameters
  JSON_Object *object_model =
      json_object_dotget_object(object, "ai_models." MODEL_NAME);
  JSON_Object *object_params = nullptr;
  if (object_model == nullptr || (object_params = json_object_dotget_object(
                                      object_model, "parameters")) == nullptr) {
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
    if ((act = extractor(object_params, &detection_param)) != kDataProcessorOk)
      res = act;
  }
  pthread_mutex_unlock(&data_processor_mutex);

  if (SetEdgeAppLibNetwork(s_stream, object_model) != 0) {
    res = kDataProcessorInvalidParamSetError;
  }

  // Get Area settings
  JSON_Object *area_obj = json_object_get_object(object, "area");
  if (area_obj != NULL) {
    JSON_Object *area_coordinates_obj =
        json_object_get_object(area_obj, "coordinates");
    area.coordinates.left =
        json_object_get_number(area_coordinates_obj, "left");
    area.coordinates.top = json_object_get_number(area_coordinates_obj, "top");
    area.coordinates.right =
        json_object_get_number(area_coordinates_obj, "right");
    area.coordinates.bottom =
        json_object_get_number(area_coordinates_obj, "bottom");
    area.overlap = json_object_get_number(area_obj, "overlap");

    JSON_Array *class_id_array = json_object_get_array(area_obj, "class_id");
    area.num_of_class = json_array_get_count(class_id_array);
    if (area.num_of_class > CLASS_IDS_SIZE) {
      LOG_ERR(
          "The number of class_ids specified is %d. It exceeds the "
          "limitation(=%d).",
          area.num_of_class, CLASS_IDS_SIZE);
      json_value_free(value);
      return kDataProcessorInvalidParam;
    }
    for (int i = 0; i < area.num_of_class; i++) {
      JSON_Value *class_id_value = json_array_get_value(class_id_array, i);
      area.class_ids[i] = json_value_get_number(class_id_value);
    }
    send_area_counts = true;
  } else {
    send_area_counts = false;
    area = {};
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

DataProcessorResultCode DataProcessorAnalyze(float *in_data, uint32_t in_size,
                                             char **out_data,
                                             uint32_t *out_size) {
  LOG_TRACE("DataProcessorAnalyze");
  if (in_data == nullptr) {
    LOG_ERR("Invalid in_data param");
    return kDataProcessorInvalidParam;
  }

  pthread_mutex_lock(&data_processor_mutex);
  DataProcessorCustomParam analyze_params = detection_param;
  pthread_mutex_unlock(&data_processor_mutex);

  Detections *detections = CreateDetections(in_data, in_size, analyze_params);

  if (detections == NULL || detections->detection_data == NULL) {
    LOG_ERR("Error while allocating memory for detections.");
    return kDataProcessorMemoryError;
  }

  FilterByParams(&detections, analyze_params);

  if (send_area_counts) {
    LOG_INFO("Send the result of area counts.");
    AreaCount *area_count = CreateAreaCount(&detections, area);
    switch (metadata_format) {
      case EdgeAppLibSendDataBase64: {
        flatbuffers::FlatBufferBuilder builder =
            flatbuffers::FlatBufferBuilder();
        MakeAreaFlatbuffer(detections, area_count, &builder, area.num_of_class);
        /* LCOV_EXCL_START: null pointer check*/
        uint8_t *buf_ptr = builder.GetBufferPointer();
        if (buf_ptr == nullptr) {
          LOG_ERR("Error while getting flatbuffers pointer");
          return kDataProcessorOther;
        }
        uint32_t buf_size = builder.GetSize();
        char *p_out_param = (char *)malloc(buf_size);
        if (p_out_param == nullptr) {
          LOG_ERR("Error while allocating memory for flatbuffers of size %d",
                  buf_size);
          return kDataProcessorMemoryError;
        }
        /* LCOV_EXCL_STOP */
        memcpy(p_out_param, buf_ptr, buf_size);
        *out_data = p_out_param;
        *out_size = buf_size;

        free(detections->detection_data);
        free(detections);
        free(area_count);
        return kDataProcessorOk;
      }
      case EdgeAppLibSendDataJson: {
        JSON_Value *tensor_output =
            MakeAreaJson(detections, area_count, area.num_of_class);
        *out_data = json_serialize_to_string(tensor_output);
        *out_size = json_serialization_size(tensor_output);
        json_value_free(tensor_output);
        free(detections->detection_data);
        free(detections);
        free(area_count);
        return kDataProcessorOk;
      }
      default:
        LOG_ERR("Unknown metadata format: %d.", metadata_format);
        free(detections->detection_data);
        free(detections);
        free(area_count);
        return kDataProcessorInvalidParam;
    }
    return kDataProcessorOk;

  } else {
    LOG_INFO("Send the result of detections.");
    switch (metadata_format) {
      case EdgeAppLibSendDataBase64: {
        flatbuffers::FlatBufferBuilder builder =
            flatbuffers::FlatBufferBuilder();
        MakeDetectionFlatbuffer(detections, &builder);
        /* LCOV_EXCL_START: null pointer check*/
        uint8_t *buf_ptr = builder.GetBufferPointer();
        if (buf_ptr == nullptr) {
          LOG_ERR("Error while getting flatbuffers pointer");
          return kDataProcessorOther;
        }
        uint32_t buf_size = builder.GetSize();
        char *p_out_param = (char *)malloc(buf_size);
        if (p_out_param == nullptr) {
          LOG_ERR("Error while allocating memory for flatbuffers of size %d",
                  buf_size);
          return kDataProcessorMemoryError;
        }
        /* LCOV_EXCL_STOP */
        memcpy(p_out_param, buf_ptr, buf_size);
        *out_data = p_out_param;
        *out_size = buf_size;

        free(detections->detection_data);
        free(detections);
        return kDataProcessorOk;
      }
      case EdgeAppLibSendDataJson: {
        JSON_Value *tensor_output = MakeDetectionJson(detections);
        *out_data = json_serialize_to_string(tensor_output);
        *out_size = json_serialization_size(tensor_output);
        json_value_free(tensor_output);
        free(detections->detection_data);
        free(detections);
        return kDataProcessorOk;
      }
      default:
        LOG_ERR("Unknown metadata format: %d.", metadata_format);
        free(detections->detection_data);
        free(detections);
        return kDataProcessorInvalidParam;
    }
  }
}

EdgeAppLibSendDataType DataProcessorGetDataType() { return metadata_format; }
