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

#include "dtdl_model/dtdl_model.hpp"

#include "log.h"

static const char *REQ_INFO = "req_info";
static const char *RES_INFO = "res_info";
static const char *COMMON_SETTINGS = "common_settings";
static const char *CUSTOM_SETTINGS = "custom_settings";

DtdlModel::DtdlModel() {
  LOG_TRACE("Initializing DTDL");

  static Property s_properties[] = {
      {.property = REQ_INFO, .obj = GetReqInfo()},
      {.property = RES_INFO, .obj = GetResInfo()},
      {.property = COMMON_SETTINGS, .obj = GetCommonSettings()},
      {.property = CUSTOM_SETTINGS, .obj = GetCustomSettings()}};
  SetProperties(s_properties, sizeof(s_properties) / sizeof(Property));

  json_object_set_value(
      json_obj, RES_INFO,
      json_object_get_wrapping_value(res_info.GetJsonObject()));
  json_object_set_value(
      json_obj, COMMON_SETTINGS,
      json_object_get_wrapping_value(common_settings.GetJsonObject()));
  json_object_set_value(
      json_obj, CUSTOM_SETTINGS,
      json_object_get_wrapping_value(custom_settings.GetJsonObject()));
}

DtdlModel::~DtdlModel() {
  json_value_free(json_object_get_wrapping_value(json_obj));
  req_info.Delete();
}

int DtdlModel::Update(void *json) {
  LOG_DBG("Parsing new DTDL object");
  JSON_Value *new_json_value = json_parse_string((const char *)json);
  if (new_json_value == nullptr) {
    LOG_ERR("json_parse_string null");
    return -1;
  }
  JSON_Object *new_json_obj = json_value_get_object(new_json_value);
  if (new_json_obj == nullptr) {
    LOG_ERR("json_value_get_object null");
    json_value_free(new_json_value);
    return -1;
  }
  res_info.Reset();
  int res = Verify(new_json_obj);
  if (res == 0) Apply(new_json_obj);
  json_value_free(new_json_value);
  return res;
}

int DtdlModel::Verify(JSON_Object *obj) {
  // req_info must be applied in verification to contain req_id
  req_info.Apply(json_object_get_object(obj, REQ_INFO));
  res_info.SetResId(req_info.GetReqId());

  return JsonObject::Verify(obj);
}

int DtdlModel::Apply(JSON_Object *obj) {
  int ret = 0;
  if (json_object_has_value(obj, COMMON_SETTINGS)) {
    ret = GetCommonSettings()->Apply(
        json_object_get_object(obj, COMMON_SETTINGS));
  }
  if (ret == 0 && json_object_has_value(obj, CUSTOM_SETTINGS))
    ret = GetCustomSettings()->Apply(
        json_object_get_object(obj, CUSTOM_SETTINGS));
  return ret;
}

char *DtdlModel::Serialize() {
  return json_serialize_to_string(json_object_get_wrapping_value(json_obj));
}

ReqInfo *DtdlModel::GetReqInfo() { return &req_info; }
void DtdlModel::InitializeValues() {
  PqSettings *pq_setting = common_settings.GetPqSettings();
  pq_setting->InitializeValues();
}

ResInfo *DtdlModel::GetResInfo() { return &res_info; }

CommonSettings *DtdlModel::GetCommonSettings() { return &common_settings; }

CustomSettings *DtdlModel::GetCustomSettings() { return &custom_settings; }
