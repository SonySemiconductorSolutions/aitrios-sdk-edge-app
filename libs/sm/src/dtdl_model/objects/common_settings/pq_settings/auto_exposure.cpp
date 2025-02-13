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

#include "dtdl_model/objects/common_settings/pq_settings/auto_exposure.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *MAX_EXPOSURE_TIME = "max_exposure_time";
static const char *MIN_EXPOSURE_TIME = "min_exposure_time";
static const char *MAX_GAIN = "max_gain";
static const char *CONVERGENCE_SPEED = "convergence_speed";

AutoExposure::AutoExposure() {
  static Validation s_validations[] = {
      {.property = MAX_EXPOSURE_TIME, .validation = kGe, .value = 0},
      {.property = MIN_EXPOSURE_TIME, .validation = kGe, .value = 0},
      {.property = CONVERGENCE_SPEED, .validation = kGe, .value = 0},
      {.property = MAX_EXPOSURE_TIME, .validation = kType, .value = JSONNumber},
      {.property = MIN_EXPOSURE_TIME, .validation = kType, .value = JSONNumber},
      {.property = MAX_GAIN, .validation = kType, .value = JSONNumber},
      {.property = CONVERGENCE_SPEED, .validation = kType, .value = JSONNumber},
  }; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void AutoExposure::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY,
                          &auto_exposure, sizeof(auto_exposure));

  json_object_set_number(json_obj, MAX_EXPOSURE_TIME,
                         auto_exposure.max_exposure_time);
  json_object_set_number(json_obj, MIN_EXPOSURE_TIME,
                         auto_exposure.min_exposure_time);
  json_object_set_number(json_obj, MAX_GAIN, auto_exposure.max_gain);
  json_object_set_number(json_obj, CONVERGENCE_SPEED,
                         auto_exposure.convergence_speed);
}

int AutoExposure::Verify(JSON_Object *obj) {
  if (JsonObject::Verify(obj) != 0) return -1;

  EdgeAppLibSensorCameraAutoExposureProperty aux_auto_exposure = auto_exposure;
  if (json_object_has_value(obj, MIN_EXPOSURE_TIME))
    aux_auto_exposure.min_exposure_time =
        json_object_get_number(obj, MIN_EXPOSURE_TIME);
  if (json_object_has_value(obj, MAX_EXPOSURE_TIME))
    aux_auto_exposure.max_exposure_time =
        json_object_get_number(obj, MAX_EXPOSURE_TIME);

  if (aux_auto_exposure.min_exposure_time >
      aux_auto_exposure.max_exposure_time) {
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Parameter min_exposure_time can not be greater than "
        "max_exposure_time");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    return -1;
  }
  return 0;
}

int AutoExposure::Apply(JSON_Object *obj) {
  EdgeAppLibSensorCameraAutoExposureProperty aux_auto_exposure = auto_exposure;

  if (json_object_has_value(obj, MAX_EXPOSURE_TIME))
    aux_auto_exposure.max_exposure_time =
        json_object_get_number(obj, MAX_EXPOSURE_TIME);
  if (json_object_has_value(obj, MIN_EXPOSURE_TIME))
    aux_auto_exposure.min_exposure_time =
        json_object_get_number(obj, MIN_EXPOSURE_TIME);
  if (json_object_has_value(obj, MAX_GAIN))
    aux_auto_exposure.max_gain = json_object_get_number(obj, MAX_GAIN);
  if (json_object_has_value(obj, CONVERGENCE_SPEED))
    aux_auto_exposure.convergence_speed =
        json_object_get_number(obj, CONVERGENCE_SPEED);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY,
      &aux_auto_exposure, sizeof(aux_auto_exposure));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Auto Exposure property failed to be set. Please use valid values "
        "for max_exposure_time, min_exposure_time, max_gain and "
        "convergence_speed.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}
void AutoExposure::StoreValue(uint32_t max_exp, uint32_t min_exp,
                              float max_gain, uint32_t max_conv_sp) {
  /* LCOV_EXCL_BR_START: 2^4 different options */
  if (max_exp == auto_exposure.max_exposure_time &&
      min_exp == auto_exposure.min_exposure_time &&
      IsAlmostEqual(max_gain, auto_exposure.max_gain) &&
      max_conv_sp == auto_exposure.convergence_speed)
    return;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating AutoExposure");
  json_object_set_number(json_obj, MAX_EXPOSURE_TIME, max_exp);
  json_object_set_number(json_obj, MIN_EXPOSURE_TIME, min_exp);
  json_object_set_number(json_obj, MAX_GAIN, max_gain);
  json_object_set_number(json_obj, CONVERGENCE_SPEED, max_conv_sp);
  auto_exposure = EdgeAppLibSensorCameraAutoExposureProperty{
      .max_exposure_time = max_exp,
      .min_exposure_time = min_exp,
      .max_gain = max_gain,
      .convergence_speed = max_conv_sp};
}
