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

#include "dtdl_model/objects/common_settings/port_setting.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_context.hpp"

#define METHOD "method"
#define STORAGE_NAME "storage_name"
#define ENDPOINT "endpoint"
#define PATH "path"
#define ENABLED "enabled"

PortSetting::PortSetting(PortSettingOption ps_opt) {
  static Validation s_validations[] = {
      {.property = METHOD, .validation = kType, .value = JSONNumber},
      {.property = STORAGE_NAME, .validation = kType, .value = JSONString},
      {.property = ENDPOINT, .validation = kType, .value = JSONString},
      {.property = PATH, .validation = kType, .value = JSONString},
      {.property = ENABLED, .validation = kType, .value = JSONBoolean},
  };
  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
  json_object_set_number(json_obj, METHOD, 0);
  json_object_set_string(json_obj, STORAGE_NAME, "");
  json_object_set_string(json_obj, ENDPOINT, "");
  json_object_set_string(json_obj, PATH, "");
  json_object_set_boolean(json_obj, ENABLED, false);
}

int PortSetting::Apply(JSON_Object *obj) {
  if (json_object_has_value(obj, METHOD))
    json_object_set_number(json_obj, METHOD,
                           json_object_get_number(obj, METHOD));
  if (json_object_has_value(obj, STORAGE_NAME))
    json_object_set_string(json_obj, STORAGE_NAME,
                           json_object_get_string(obj, STORAGE_NAME));
  if (json_object_has_value(obj, ENDPOINT))
    json_object_set_string(json_obj, ENDPOINT,
                           json_object_get_string(obj, ENDPOINT));
  if (json_object_has_value(obj, PATH))
    json_object_set_string(json_obj, PATH, json_object_get_string(obj, PATH));
  if (json_object_has_value(obj, ENABLED))
    json_object_set_boolean(json_obj, ENABLED,
                            json_object_get_boolean(obj, ENABLED));
  return 0;
}

uint32_t PortSetting::GetMethod() const {
  return (uint32_t)json_object_get_number(json_obj, METHOD);
}

const char *PortSetting::GetStorageName() const {
  return json_object_get_string(json_obj, STORAGE_NAME);
}

const char *PortSetting::GetEndpoint() const {
  return json_object_get_string(json_obj, ENDPOINT);
}

const char *PortSetting::GetPath() const {
  return json_object_get_string(json_obj, PATH);
}

bool PortSetting::GetEnabled() const {
  return (bool)json_object_get_boolean(json_obj, ENABLED);
}
