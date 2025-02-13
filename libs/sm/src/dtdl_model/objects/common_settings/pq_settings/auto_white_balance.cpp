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

#include "dtdl_model/objects/common_settings/pq_settings/auto_white_balance.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *CONVERGENCE_SPEED = "convergence_speed";

AutoWhiteBalance::AutoWhiteBalance() {
  static Validation s_validations[2] = {
      {CONVERGENCE_SPEED, kGe, 0},
      {CONVERGENCE_SPEED, kType,
       JSONNumber}}; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void AutoWhiteBalance::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY,
                          &wb_speed, sizeof(wb_speed));

  json_object_set_number(json_obj, CONVERGENCE_SPEED,
                         wb_speed.convergence_speed);
}

int AutoWhiteBalance::Apply(JSON_Object *obj) {
  EdgeAppLibSensorAutoWhiteBalanceProperty aux_wb_speed = wb_speed;

  if (json_object_has_value(obj, CONVERGENCE_SPEED))
    aux_wb_speed.convergence_speed =
        json_object_get_number(obj, CONVERGENCE_SPEED);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY, &aux_wb_speed,
      sizeof(aux_wb_speed));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Auto White Balance property failed to be set. Please use valid values "
        "for convergence_speed.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void AutoWhiteBalance::StoreValue(uint32_t speed) {
  if (speed == wb_speed.convergence_speed) return;

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating AutoWhiteBalance");
  json_object_set_number(json_obj, CONVERGENCE_SPEED, speed);
  wb_speed =
      EdgeAppLibSensorAutoWhiteBalanceProperty{.convergence_speed = speed};
}
