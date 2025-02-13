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

#include "dtdl_model/objects/custom_settings.hpp"

#include "dtdl_model/properties.h"
#include "log.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/state.hpp"
#include "states/state_utils.hpp"

static const char *CUSTOM_SETTINGS = "custom_settings";

int CustomSettings::Verify(JSON_Object *obj) {
  // since parson parsed the string correctly, custom settings must be a valid
  // json
  return 0;
}

int CustomSettings::Apply(JSON_Object *obj) {
  int ret = 0;
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  State *state = context->GetCurrentState();

  json_object_remove(json_obj, "res_info");
  JSON_Value *value_tmp = json_object_get_wrapping_value(obj);
  if (json_value_equals(json_object_get_wrapping_value(json_obj), value_tmp) !=
      1) {
    value_tmp = json_value_deep_copy(value_tmp);
    json_obj = json_object(value_tmp);

    // value_tmp is set as a custom setting in the JSON object of DtdlModel
    // It is deallocated in DtdlModel's destructor via json_value_free, managed
    // by json_obj.
    json_object_set_value(context->GetDtdlModel()->GetJsonObject(),
                          CUSTOM_SETTINGS, value_tmp);
    ReqInfo *req_info =
        StateMachineContext::GetInstance(nullptr)->GetDtdlModel()->GetReqInfo();
    // fill res_info with default values
    json_object_dotset_number(json_obj, "res_info.code", 0);
    json_object_dotset_string(json_obj, "res_info.res_id",
                              req_info->GetReqId());
    json_object_dotset_string(json_obj, "res_info.detail_msg", "");

    char *custom_settings =
        json_serialize_to_string(json_object_get_wrapping_value(json_obj));
    int res = onConfigure((char *)CUSTOM_SETTINGS, custom_settings,
                          strlen(custom_settings));
    if (res != 0) {
      ret = res;
      EventHandleError(ON_CONFIGURE, res, context, STATE_IDLE);
    }
    context->EnableNotification();
  } else {
    LOG_INFO("Custom setting remains the same");
  }
  return ret;
}

void CustomSettings::Store(void *state, int statelen) {
  JSON_Value *value = json_parse_string((char *)state);
  if (value == nullptr) { /* LCOV_EXCL_START: error check */
    LOG_WARN("Custom settings from developer code cannot be parsed.");
    return;
    /* LCOV_EXCL_STOP */
  }
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  json_obj = json_object(value);
  json_object_set_value(context->GetDtdlModel()->GetJsonObject(),
                        CUSTOM_SETTINGS, value);

  context->EnableNotification();
  LOG_TRACE("Custom settings copied to DTDL");
}
