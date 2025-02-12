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

#include "dtdl_model/objects/res_info.hpp"

#include <stdlib.h>

#include "log.h"
#include "sm_context.hpp"

#define RES_ID "res_id"
#define CODE "code"
#define DETAIL_MSG "detail_msg"

ResInfo::ResInfo() { Reset(); }

void ResInfo::Reset() {
  json_object_set_number(json_obj, CODE, 0);
  json_object_set_string(json_obj, RES_ID, "");
  json_object_set_string(json_obj, DETAIL_MSG, "");
}

int ResInfo::Verify(JSON_Object *obj) { return 0; }

int ResInfo::Apply(JSON_Object *obj) { return 0; }

const char *ResInfo::GetResId() const {
  return json_object_get_string(json_obj, RES_ID);
}

int ResInfo::SetResId(const char *value) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();
  return json_object_set_string(json_obj, RES_ID, value) == JSONSuccess;
}

uint32_t ResInfo::GetCode() const {
  return (uint32_t)json_object_get_number(json_obj, CODE);
}

int ResInfo::SetCode(uint32_t value) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();
  return json_object_set_number(json_obj, CODE, (double)value) == JSONSuccess;
}

const char *ResInfo::GetDetailMsg() const {
  return json_object_get_string(json_obj, DETAIL_MSG);
}

int ResInfo::SetDetailMsg(const char *value) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();
  return json_object_set_string(json_obj, DETAIL_MSG, value) == JSONSuccess;
}
