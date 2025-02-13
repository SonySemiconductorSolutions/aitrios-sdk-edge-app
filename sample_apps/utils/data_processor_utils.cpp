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

DataProcessorResultCode ExtractHeaderIdAndVersion(
    JSON_Object *json, const char *custom_id_version) {
  int ret = json_object_has_value(json, "header");
  if (!ret) {
    LOG_ERR("json file does not have header");
    return kDataProcessorOk;
  }

  ret = json_object_has_value_of_type(json, "header", JSONObject);
  if (!ret) {
    return kDataProcessorOk;
  }

  JSON_Object *header = json_object_get_object(json, "header");
  const char *p = NULL;

  ret = json_object_has_value(header, "id");
  if (!ret) {
    LOG_ERR("json file does not have header:id");
    return kDataProcessorInvalidParam;
  }
  p = json_object_get_string(header, "id");
  if (p == NULL) {
    LOG_ERR("header_id is NULL");
    return kDataProcessorInvalidParam;
  }
  ret = strncmp(custom_id_version, p, 2);
  LOG_DBG("header_id = %s", p);
  if (ret != 0) {
    return kDataProcessorInvalidParam;
  }

  ret = json_object_has_value(header, "version");
  if (!ret) {
    LOG_ERR("json file does not have header:version");
    return kDataProcessorInvalidParam;
  }
  p = json_object_get_string(header, "version");
  if (p == NULL) {
    LOG_ERR("header_version is NULL");
    return kDataProcessorInvalidParam;
  }
  ret = strncmp(&custom_id_version[3], p, 8);
  if (ret == 0) {
    LOG_DBG("header_version p = %s custom_id_version=%s", p,
            &custom_id_version[3]);
  } else {
    LOG_ERR("header_version = %s", p);
    return kDataProcessorInvalidParam;
  }
  return kDataProcessorOk;
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
