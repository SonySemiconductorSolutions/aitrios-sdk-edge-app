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

#include "dtdl_model/objects/common_settings/codec_settings.hpp"

#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_context.hpp"

#define FORMAT "format"

CodecSettings::CodecSettings() {
  static Validation s_validations[] = {
      {.property = FORMAT, .validation = kType, .value = JSONNumber},
  };
  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

int CodecSettings::Apply(JSON_Object *obj) {
  if (!json_object_has_value(obj, FORMAT)) return 0;

  uint32_t format = (uint32_t)json_object_get_number(obj, FORMAT);
  if ((uint32_t)json_object_get_number(json_obj, FORMAT) == format) return 0;

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating CodecSettings");
  json_object_set_number(json_obj, FORMAT, format);
  return 0;
}
