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

#include <pthread.h>

#include "classification_utils.hpp"
#include "data_processor_utils.hpp"
#include "device.h"
#include "log.h"
#include "parson.h"
#include "sensor.h"
#include "sm_types.h"
#include "sm_utils.hpp"
#include "time.h"
extern "C" {
#include "base64.h"
}
extern EdgeAppLibSensorStream s_stream;

using EdgeAppLib::SensorStreamGetProperty;

#define MODEL_NAME "classification"

pthread_mutex_t data_processor_mutex;
EdgeAppLibSendDataType metadata_format = EdgeAppLibSendDataBase64;

DataProcessorCustomParam cls_param = {DEFAULT_MAX_PREDICTIONS};

static DataProcessorResultCode (*extractors[])(JSON_Object *,
                                               DataProcessorCustomParam *) = {
    ExtractMaxPredictions};

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
  // LCOV_EXCL_BR_START
  if (object_model == nullptr ||
      (object_params = json_object_dotget_object(object_model, "parameters")) ==
          nullptr) {  // LCOV_EXCL_BR_STOP
    const char *error_msg =
        "Error accesing AI model parameters in JSON object.";
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
    if ((act = extractor(object_params, &cls_param)) != kDataProcessorOk)
      res = act;
  }
  pthread_mutex_unlock(&data_processor_mutex);

  if (SetEdgeAppLibNetwork(s_stream, object_model) != 0) {
    res = kDataProcessorInvalidParamSetError;
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
  DataProcessorCustomParam analyze_params = cls_param;
  pthread_mutex_unlock(&data_processor_mutex);

  if (in_size / sizeof(float) < analyze_params.maxPredictions) {
    LOG_WARN(
        "Number of classses in the model output tensor is lower than the "
        "expected maxPreditions");
  }

  switch (metadata_format) {
    case EdgeAppLibSendDataBase64: {
      flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();

      CreateClassificationFlatbuffer(in_data, in_size / sizeof(float), &builder,
                                     analyze_params);
      /* LCOV_EXCL_START: null pointer check*/
      uint8_t *buf_ptr = builder.GetBufferPointer();
      if (buf_ptr == nullptr) {
        LOG_ERR("Error while getting flatbuffers pointer");
        return kDataProcessorOther;
      }
      /* LCOV_EXCL_STOP */
      /* LCOV_EXCL_START: null pointer check */
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
      return kDataProcessorOk;
    }
    case EdgeAppLibSendDataJson: {
      JSON_Value *tensor_output =
          CreateClsOutputJson(in_data, in_size / sizeof(float), analyze_params);
      *out_data = json_serialize_to_string(tensor_output);
      *out_size = json_serialization_size(tensor_output);
      json_value_free(tensor_output);
      return kDataProcessorOk;
    }
    default:
      LOG_ERR("Specified format of metadata is undefined. ");
      return kDataProcessorInvalidParam;
  }
}

EdgeAppLibSendDataType DataProcessorGetDataType() { return metadata_format; }
