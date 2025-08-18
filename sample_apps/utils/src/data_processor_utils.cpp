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

#include "data_processor_utils.hpp"

#include <string.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

#include "data_processor_api.hpp"
#include "log.h"
#include "parson.h"

int GetValueNumber(JSON_Object *json, const char *param, double *result) {
  if (json == NULL || param == NULL || result == NULL) {
    LOG_ERR("Invalid input arguments");
    return -1;
  }
  if (json_object_has_value(json, param)) {
    *result = json_object_get_number(json, param);
    return 0;
  }
  LOG_WARN("JSON file does not have parameter '%s' using default value", param);
  return 1;
}

int GetValueBoolean(JSON_Object *json, const char *param, bool *result) {
  if (json == NULL || param == NULL || result == NULL) {
    LOG_ERR("Invalid input arguments");
    return -1;
  }
  if (json_object_has_value(json, param)) {
    *result = json_object_get_boolean(json, param);
    return 0;
  }
  LOG_WARN("JSON file does not have parameter '%s' using default value", param);
  return 1;
}

int GetValueString(JSON_Object *json, const char *param, char *result,
                   size_t result_size) {
  if (json == NULL || param == NULL || result == NULL) {
    LOG_ERR("Invalid input arguments");
    return -1;
  }
  if (json_object_has_value(json, param)) {
    const char *value = json_object_get_string(json, param);
    if (value) {
      strncpy(result, value, result_size);
      result[std::min(strlen(value), result_size - 1)] = '\0';
      return 0;
    }
  }
  LOG_WARN("JSON file does not have parameter '%s' using default value", param);
  return 1;
}

char *GetConfigureErrorJson(ResponseCode code, const char *message,
                            const char *res_id) {
  char *config_error = nullptr;
  asprintf(
      &config_error,
      "{\"res_info\": {\"res_id\":\"%s\",\"code\": %d,\"detail_msg\":\"%s\"}}",
      res_id, code, message);
  return config_error;
}
