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
#include <stdlib.h>

#include "apitest_util.h"
#include "data_processor_utils.hpp"
#include "device.h"
#include "log.h"
#include "memory_usage.h"
#include "parson.h"
#include "sensor.h"
#include "sm_types.h"
#include "sm_utils.hpp"
#include "time.h"
extern "C" {
#include "base64.h"
}
extern EdgeAppLibSensorStream s_stream;

#include "dcpu_param_parser.h"

using EdgeAppLib::SensorStreamGetProperty;

#define MODEL_NAME "apitest"

DataProcessorResultCode DataProcessorInitialize() {
  LOG_INFO(
      "Successful call, although empty implementation of "
      "DataProcessorInitialize. App will continue to work normally");

  // Test memory usage API
  MemoryMetrics metrics;
  get_memory_metrics(&metrics);
  LOG_INFO(
      "Memory Usage API Test - Used: %zu bytes, Free: %zu bytes, "
      "Fragmentation: %.2f%%",
      metrics.used_bytes, metrics.free_bytes,
      metrics.fragmentation_rate >= 0 ? metrics.fragmentation_rate * 100.0f
                                      : -1.0f);

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

DataProcessorResultCode DataProcessorConfigureApiTest(JSON_Object *object,
                                                      char **out_config_json) {
  JSON_Object *object_model = json_object_get_object(object, "apitest");
  JSON_Object *object_params = nullptr;
  if (object_model == nullptr) {
    const char *error_msg = "Not exist apitest in JSON object.";
    LOG_INFO("%s", error_msg);
    return kDataProcessorOk;
  }

  const int32_t scenario_id =
      json_object_get_number(object_model, "scenario_id");
  if (!scenario_id) {
    const char *error_msg =
        "Not exist or 0 apitest scenario_id in JSON object.";
    LOG_INFO("%s", error_msg);
    return kDataProcessorOk;
  }

  SetCurrentApiTestScenarioId(scenario_id);

  LOG_INFO("Successfully set scenario_id %d", scenario_id);
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

  // Parse custom_settings in configuration json and get post process
  // parameter
  if (ParsePostProcessParameter(value, (const char *)value) < 0) {
    const char *error_msg = "ParsePostProcessParameter nothing";
    LOG_INFO("%s", error_msg);
  }

  JSON_Object *object = json_object(value);

  // extract parameters
  JSON_Object *object_model =
      json_object_dotget_object(object, "ai_models." MODEL_NAME);
  JSON_Object *object_params = nullptr;
  if (object_model == nullptr || (object_params = json_object_dotget_object(
                                      object_model, "parameters")) == nullptr) {
    const char *error_msg = "Not exist AI model parameters in JSON object.";
    LOG_INFO("%s", error_msg);
  } else {
    if (SetEdgeAppLibNetwork(s_stream, object_model) != 0) {
      *out_config_json = json_serialize_to_string(value);
      json_value_free(value);
      return kDataProcessorInvalidParamSetError;
    }
  }

  DataProcessorResultCode res_apitest =
      DataProcessorConfigureApiTest(object, out_config_json);
  if (res_apitest != kDataProcessorOk) {
    const char *error_msg = "Error parsing custom settings apitest JSON";
    LOG_ERR("%s", error_msg);
    *out_config_json =
        GetConfigureErrorJson(ResponseCodeInvalidArgument, error_msg, "");
    json_value_free(value);
    return res_apitest;
  }

  json_value_free(value);
  return res_apitest;
}

DataProcessorResultCode DataProcessorJsonFormat(void *in_data, uint32_t in_size,
                                                uint64_t timestamp,
                                                char **out_data,
                                                uint32_t *out_size) {
  LOG_TRACE("DataProcessorFormat");

  // Test memory usage API during processing
  MemoryMetrics metrics;
  get_memory_metrics(&metrics);
  LOG_INFO(
      "Memory during processing - Used: %zu bytes, Free: %zu bytes, "
      "Fragmentation: %.2f%%",
      metrics.used_bytes, metrics.free_bytes,
      metrics.fragmentation_rate >= 0 ? metrics.fragmentation_rate * 100.0f
                                      : -1.0f);

  if (in_data == nullptr) {
    const char *error_msg = "Invalid in_data param";
    LOG_ERR("%s", error_msg);
    *out_data =
        GetConfigureErrorJson(ResponseCodeInvalidArgument, error_msg, "");
    *out_size = strlen(*out_data);
    return kDataProcessorInvalidParam;
  }
  JSON_Value *json_value = json_value_init_object();
  JSON_Object *json_object = json_value_get_object(json_value);
  struct EdgeAppLibSensorAiModelBundleIdProperty moduleId_value = {};
  char device_id[WASM_BINDING_DEVICEID_MAX_SIZE] = {0};
  int32_t ret = -1;
  if ((ret = SensorStreamGetProperty(
           s_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
           &moduleId_value, sizeof(moduleId_value))) != 0) {
    const char *error_msg = "Error GET AI model id.";
    LOG_ERR("%s : ret=%d", error_msg, ret);
    *out_data = GetConfigureErrorJson(
        ResponseCodeUnavaiable, error_msg,
        json_object_dotget_string(json_object, "res_info.res_id"));
    *out_size = strlen(*out_data);
    json_value_free(json_value);
    return kDataProcessorOther;
  }
  if ((ret = EsfSystemGetDeviceID(device_id)) != kEsfDeviceIdResultOk) {
    const char *error_msg = "Error GET device id.";
    LOG_ERR("%s : ret=%d", error_msg, ret);
    *out_data = GetConfigureErrorJson(
        ResponseCodeUnavaiable, error_msg,
        json_object_dotget_string(json_object, "res_info.res_id"));
    // This part should be sent via Evp_sendstate
    *out_size = strlen(*out_data);
    snprintf(device_id, sizeof(device_id), "000000000000000");
  }
  time_t seconds = (time_t)(timestamp / 1000000000ULL);
  struct tm tm;
  gmtime_r(&seconds, &tm);
  uint64_t milliseconds = (timestamp % 1000000000ULL) / 1000000ULL;
  char output_timestamp[32];
  int num =
      strftime(output_timestamp, sizeof(output_timestamp), "%Y%m%d%H%M%S", &tm);
  if (num > 0) {
    snprintf(output_timestamp + num, sizeof(output_timestamp) - num,
             "%03" PRIu64, milliseconds);
  }
  int tensor_out_size = b64e_size((unsigned int)in_size) + 1;
  unsigned char *tensor_out =
      (unsigned char *)malloc((sizeof(char) * tensor_out_size));
  if (tensor_out == nullptr) {
    const char *error_msg = "Error while allocating memory for tensor_out.";
    LOG_ERR("%s : ret=%d", error_msg, ret);
    *out_data = GetConfigureErrorJson(
        ResponseCodeUnavaiable, error_msg,
        json_object_dotget_string(json_object, "res_info.res_id"));
    *out_size = strlen(*out_data);
    json_value_free(json_value);
    return kDataProcessorMemoryError;
  }
  tensor_out_size = b64_encode((unsigned char *)in_data, in_size, tensor_out);
  json_object_set_string(json_object, "ModelID",
                         moduleId_value.ai_model_bundle_id);
  json_object_set_string(json_object, "DeviceID", device_id);
  json_object_set_boolean(json_object, "Image", 0);
  JSON_Value *json_array_value = json_value_init_array();
  JSON_Array *json_array = json_value_get_array(json_array_value);
  JSON_Value *tensor_value = json_value_init_object();
  JSON_Object *tensor_object = json_value_get_object(tensor_value);
  json_object_set_string(tensor_object, "T", (const char *)output_timestamp);
  json_object_set_string(tensor_object, "O", (const char *)tensor_out);
  json_array_append_value(json_array, tensor_value);
  json_object_set_value(json_object, "Inferences", json_array_value);
  char *send_buffer = json_serialize_to_string(json_value);
  *out_data = send_buffer;
  *out_size = strlen(send_buffer);
  free(tensor_out);
  json_value_free(json_value);
  return kDataProcessorOk;
}
