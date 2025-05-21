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
#include <edgeapp/sm_types.h>
#include <stdlib.h>

#include "data_processor_utils.hpp"
#include "parson.h"
#include "sm_utils.hpp"
extern EdgeAppLibSensorStream s_stream;

using EdgeAppLib::SensorStreamGetProperty;

#define MODEL_NAME "passthrough"

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
  LOG_INFO("config_json:%s\n", config_json);
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
  if (object_model == nullptr) {
    const char *error_msg =
        "Error accessing AI model parameters in JSON object.";
    LOG_ERR("%s", error_msg);
    *out_config_json = GetConfigureErrorJson(
        ResponseCodeInvalidArgument, error_msg,
        json_object_dotget_string(object, "res_info.res_id"));
    json_value_free(value);
    return kDataProcessorInvalidParam;
  }
  DataProcessorResultCode res = kDataProcessorOk;
  if (SetEdgeAppLibNetwork(s_stream, object_model) != 0) {
    res = kDataProcessorInvalidParamSetError;
    *out_config_json = json_serialize_to_string(value);
  }

  json_value_free(value);
  return res;
}
