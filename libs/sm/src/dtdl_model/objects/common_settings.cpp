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

#include "dtdl_model/objects/common_settings.hpp"

#include "common_settings.hpp"
#include "dtdl_model/properties.h"
#include "log.h"
#include "log_internal.h"
#include "sm_context.hpp"
#include "state.hpp"

static const char *COMMON_SETTINGS = "common_settings";
static const char *LOG_LEVEL = "log_level";
static const char *PROCESS_STATE = "process_state";
static const char *INFERENCE_SETTINGS = "inference_settings";
static const char *PQ_SETTINGS = "pq_settings";
static const char *PORT_SETTINGS = "port_settings";
static const char *CODEC_SETTINGS = "codec_settings";
static const char *NUMBER_OF_INFERENCE_PER_MESSAGE =
    "number_of_inference_per_message";

// define in dtdl
STATE EnumToState(int process_state) {
  switch (process_state) { /* LCOV_EXCL_BR_LINE: default case */
    case 1:
      return STATE_IDLE;
    case 2:
      return STATE_RUNNING;
    case 3:
      return STATE_DESTROYING;
  }
  /* LCOV_EXCL_START: error check */
  LOG_ERR("Unknown state %d", process_state);
  return STATE_EXITING;
  /* LCOV_EXCL_STOP */
}

CommonSettings::CommonSettings() {
  static Validation s_validations[] = {
      {.property = PROCESS_STATE, .validation = kGe, .value = STATE_IDLE},
      {.property = PROCESS_STATE,
       .validation = kLe,
       .value = STATE_RUNNING}}; /* LCOV_EXCL_BR_LINE: array check */
  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
  static Property s_properties[] = {
      {.property = PQ_SETTINGS, .obj = &pq_settings},
      {.property = PORT_SETTINGS, .obj = &port_settings},
      {.property = INFERENCE_SETTINGS, .obj = &inference_settings},
      {.property = CODEC_SETTINGS,
       .obj = &codec_settings}}; /* LCOV_EXCL_BR_LINE: array check */
  SetProperties(s_properties, sizeof(s_properties) / sizeof(Property));

  json_object_set_number(json_obj, PROCESS_STATE, STATE_IDLE);
  SetLogLevel(kWarnLevel);
  json_object_set_number(json_obj, LOG_LEVEL, GetLogLevel());
  json_object_set_value(
      json_obj, INFERENCE_SETTINGS,
      json_object_get_wrapping_value(inference_settings.GetJsonObject()));
  json_object_set_value(
      json_obj, PQ_SETTINGS,
      json_object_get_wrapping_value(pq_settings.GetJsonObject()));
  json_object_set_value(
      json_obj, PORT_SETTINGS,
      json_object_get_wrapping_value(port_settings.GetJsonObject()));
  json_object_set_value(
      json_obj, CODEC_SETTINGS,
      json_object_get_wrapping_value(codec_settings.GetJsonObject()));
}

JSON_Object *CommonSettings::GetSettingsJson(const char *setting) {
  if (strcmp(setting, PQ_SETTINGS) == 0) {
    return GetPqSettings()->GetJsonObject();
  } else if (strcmp(setting, PORT_SETTINGS) == 0) {
    return GetPortSettings()->GetJsonObject();
  } else if (strcmp(setting, CODEC_SETTINGS) == 0) {
    return GetCodecSettings()->GetJsonObject();
  } else {
    return NULL;  // Return NULL if the setting is not found
  }
}

int CommonSettings::Apply(JSON_Object *obj) {
  if (json_object_has_value(obj, PROCESS_STATE))
    SetProcessState(GetProcessState(obj));
  if (json_object_has_value(obj, LOG_LEVEL))
    SetLoggingLevel(GetLoggingLevel(obj));

  int res = 0;
  res = GetInferencePerMessage(json_obj) == GetInferencePerMessage(obj);
  for (const char *setting : {PQ_SETTINGS, PORT_SETTINGS, CODEC_SETTINGS,
                              NUMBER_OF_INFERENCE_PER_MESSAGE}) {
    if (res != 1) break;
    JSON_Value *new_setting_val =
        json_object_get_wrapping_value(json_object_get_object(obj, setting));
    JSON_Value *current_setting_val =
        json_object_get_wrapping_value(GetSettingsJson(setting));
    res = json_value_equals(new_setting_val, current_setting_val);
  }

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  if (context->GetCurrentState()->GetEnum() == STATE_RUNNING) {
    if (res != 1) {
      DtdlModel *dtdl = context->GetDtdlModel();
      dtdl->GetResInfo()->SetDetailMsg(
          "Ignoring Port Settings and Pq Settings since state is Running.");
      dtdl->GetResInfo()->SetCode(CODE_FAILED_PRECONDITION);
    }
    json_object_remove(obj, PQ_SETTINGS);
    json_object_remove(obj, PORT_SETTINGS);
    json_object_remove(obj, CODEC_SETTINGS);
    json_object_remove(obj, NUMBER_OF_INFERENCE_PER_MESSAGE);
  } else if (json_object_has_value(obj, NUMBER_OF_INFERENCE_PER_MESSAGE))
    SetInferencePerMessage(GetInferencePerMessage(obj));
  return JsonObject::Apply(obj);
}

uint32_t CommonSettings::GetProcessState(JSON_Object *obj) const {
  return (uint32_t)json_object_get_number(obj, PROCESS_STATE);
}

uint32_t CommonSettings::GetProcessState() const {
  return GetProcessState(json_obj);
}

int CommonSettings::SetProcessState(uint32_t value) {
  LOG_TRACE("In SetProcessState: %d", value);
  if (StateMachineContext::GetInstance(
          nullptr) /* LCOV_EXCL_START: error check */
          ->aitrios_sm_configurator->UpdateProcessState(EnumToState(value))) {
    return -1;
    /* LCOV_EXCL_STOP */
  }
  // avoids sending invalid state to the cloud
  if (EnumToState(value) == STATE_RUNNING || EnumToState(value) == STATE_IDLE) {
    return json_object_set_number(json_obj, PROCESS_STATE, value) ==
           JSONSuccess;
  }
  return 0;
}

uint32_t CommonSettings::GetLoggingLevel(JSON_Object *obj) const {
  return (uint32_t)json_object_get_number(obj, LOG_LEVEL);
}

int CommonSettings::SetLoggingLevel(uint32_t value) {
  if (GetLoggingLevel(json_obj) == value) return 1;
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();
  SetLogLevel((LogLevel)value);
  return json_object_set_number(json_obj, LOG_LEVEL, value) == JSONSuccess;
}

uint32_t CommonSettings::GetInferencePerMessage(JSON_Object *obj) const {
  return (uint32_t)json_object_get_number(obj, NUMBER_OF_INFERENCE_PER_MESSAGE);
}

int CommonSettings::SetInferencePerMessage(uint32_t value) {
  if (GetInferencePerMessage(json_obj) == value) return 1;
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  return json_object_set_number(json_obj, NUMBER_OF_INFERENCE_PER_MESSAGE,
                                value) == JSONSuccess;
}

PortSettings *CommonSettings::GetPortSettings() { return &port_settings; }

PqSettings *CommonSettings::GetPqSettings() { return &pq_settings; }

InferenceSettings *CommonSettings::GetInferenceSettings() {
  return &inference_settings;
}

CodecSettings *CommonSettings::GetCodecSettings() { return &codec_settings; }

uint32_t CommonSettings::getNumOfInfPerMsg() const {
  return GetInferencePerMessage(json_obj);
}
