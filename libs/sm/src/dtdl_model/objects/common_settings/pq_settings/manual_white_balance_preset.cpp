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

#include "dtdl_model/objects/common_settings/pq_settings/manual_white_balance_preset.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

typedef enum { TEMP_3200K = 0, TEMP_4300K, TEMP_5600K, TEMP_6500K } TEMP;

static uint32_t temperature_enum_to_value(TEMP temperature) {
  switch (temperature) { /* LCOV_EXCL_BR_LINE: default case */
    case TEMP_3200K:
      return 3200;
    case TEMP_4300K:
      return 4300;
    case TEMP_5600K:
      return 5600;
    case TEMP_6500K:
      return 6500;
    default: /* LCOV_EXCL_START: default case */
      LOG_WARN("Using 3200 as default");
      return 3200;
      /* LCOV_EXCL_STOP */
  }
}

static TEMP temperature_value_to_enum(uint32_t temperature) {
  switch (temperature) { /* LCOV_EXCL_BR_LINE: default case */
    case 3200:
      return TEMP_3200K;
    case 4300:
      return TEMP_4300K;
    case 5600:
      return TEMP_5600K;
    case 6500:
      return TEMP_6500K;
    default: /* LCOV_EXCL_START: default case */
      LOG_WARN("Using 3200 as default");
      return TEMP_3200K;
      /* LCOV_EXCL_STOP */
  }
}

static const char *COLOR_TEMPERATURE = "color_temperature";

ManualWhiteBalancePreset::ManualWhiteBalancePreset() {
  static Validation s_validations[3] = {
      {COLOR_TEMPERATURE, kGe, 0},
      {COLOR_TEMPERATURE, kLe, 3},
      {COLOR_TEMPERATURE, kType,
       JSONNumber}}; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void ManualWhiteBalancePreset::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &manual_wb_preset, sizeof(manual_wb_preset));

  json_object_set_number(
      json_obj, COLOR_TEMPERATURE,
      temperature_value_to_enum(manual_wb_preset.color_temperature));
}

int ManualWhiteBalancePreset::Apply(JSON_Object *obj) {
  EdgeAppLibSensorManualWhiteBalancePresetProperty aux_manual_wb_preset =
      manual_wb_preset;

  if (json_object_has_value(obj, COLOR_TEMPERATURE))
    aux_manual_wb_preset.color_temperature = temperature_enum_to_value(
        (TEMP)(int32_t)json_object_get_number(obj, COLOR_TEMPERATURE));

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &aux_manual_wb_preset, sizeof(aux_manual_wb_preset));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Manual White Balance property failed to be set. Please use valid "
        "values for color_temperature.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void ManualWhiteBalancePreset::StoreValue(uint32_t temperature) {
  temperature = temperature_value_to_enum(temperature);
  if (temperature == manual_wb_preset.color_temperature) return;

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating COLOR_TEMPERATURE");
  json_object_set_number(json_obj, COLOR_TEMPERATURE, temperature);

  manual_wb_preset = {.color_temperature = temperature};
}
