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

#include "dtdl_model/objects/common_settings/pq_settings/frame_rate.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *NUM = "num";
static const char *DEN = "denom";

FrameRate::FrameRate() {
  static Validation s_validations[4] = {
      {NUM, kGe, 0},
      {DEN, kGe, 0},
      {NUM, kType, JSONNumber},
      {DEN, kType, JSONNumber}}; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void FrameRate::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY,
                          &framerate, sizeof(framerate));

  json_object_set_number(json_obj, NUM, framerate.num);
  json_object_set_number(json_obj, DEN, framerate.denom);
}

int FrameRate::Verify(JSON_Object *obj) {
  if (JsonObject::Verify(obj) != 0) return -1;

  if (json_object_has_value(obj, NUM))
    if (IsInteger(json_object_get_number(obj, NUM)) == 0) {
      DtdlModel *dtdl =
          StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
      dtdl->GetResInfo()->SetDetailMsg("Num property has to be an integer");
      dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
      return -1;
    }

  if (json_object_has_value(obj, DEN))
    if (IsInteger(json_object_get_number(obj, DEN)) == 0) {
      DtdlModel *dtdl =
          StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
      dtdl->GetResInfo()->SetDetailMsg("Denom property has to be an integer");
      dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
      return -1;
    }

  return 0;
}

int FrameRate::Apply(JSON_Object *obj) {
  EdgeAppLibSensorCameraFrameRateProperty aux_framerate = framerate;

  if (json_object_has_value(obj, NUM))
    aux_framerate.num = (uint32_t)json_object_get_number(obj, NUM);
  if (json_object_has_value(obj, DEN))
    aux_framerate.denom = (uint32_t)json_object_get_number(obj, DEN);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY, &aux_framerate,
      sizeof(aux_framerate));

  /* LCOV_EXCL_START: error check division by 0 */
  if (result != 0) {
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "FrameRate property failed to be set. Please use valid values for num "
        "and denom.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    /* LCOV_EXCL_STOP */
  }
  return result;
}

void FrameRate::StoreValue(uint32_t num, uint32_t denom) {
  /* LCOV_EXCL_BR_START: check if initialized */
  if (framerate.num == num && framerate.denom == denom) return;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating FrameRate");
  json_object_set_number(json_obj, NUM, num);
  json_object_set_number(json_obj, DEN, denom);

  framerate = {.num = num, .denom = denom};
}
