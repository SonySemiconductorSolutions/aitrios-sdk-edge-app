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

#include "dtdl_model/objects/req_info.hpp"

#include "log.h"

#define REQ_ID "req_id"

ReqInfo::ReqInfo() { json_object_set_string(json_obj, REQ_ID, ""); }

int ReqInfo::Verify(JSON_Object *obj) {
  if (!json_object_has_value_of_type(obj, REQ_ID, JSONString)) {
    LOG_ERR("%s missing", REQ_ID);
    return -1;
  }
  return 0;
}

int ReqInfo::Apply(JSON_Object *obj) {
  json_object_set_string(json_obj, REQ_ID, json_object_get_string(obj, REQ_ID));
  return 0;
}

const char *ReqInfo::GetReqId() const {
  return json_object_get_string(json_obj, REQ_ID);
}
