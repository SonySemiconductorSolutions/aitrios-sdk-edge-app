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

#include "callbacks/configuration.hpp"

#include "log.h"
#include "sm_api.hpp"
#include "sm_context.hpp"

void configuration_cb(const char *topic, const void *config, size_t configlen,
                      void *userData) {
  JSON_Value *config_value = json_parse_string((const char *)config);
  if (config_value == NULL) {
    LOG_ERR("json_parse_string null");
    return;
  }
  JSON_Object *config_obj = json_value_get_object(config_value);
  if (config_obj == NULL) {
    LOG_ERR("json_value_get_object null");
    json_value_free(config_value);
    return;
  }
  const char *req_id = json_object_dotget_string(config_obj, "req_info.req_id");
  if (req_id == NULL) {
    LOG_ERR("req_id is null");
    json_value_free(config_value);
    return;
  }
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  const char *prev_req_id = context->GetDtdlModel()->GetReqInfo()->GetReqId();
  if (prev_req_id != NULL && strcmp(req_id, prev_req_id) == 0) {
    LOG_WARN("The req ID is the same as the one for the previous config");
    json_value_free(config_value);
    return;
  }
  json_value_free(config_value);
  context->SetPendingConfiguration((void *)config, configlen);
  context->SetNextState(STATE_APPLYING);
}
