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

#include "dtdl_model/objects/common_settings/pq_settings/manual_exposure.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *EXPOSURE_TIME = "exposure_time";
static const char *GAIN = "gain";

ManualExposure::ManualExposure() {
  static Validation s_validations[] = {
      {EXPOSURE_TIME, kGe, 0},
      {EXPOSURE_TIME, kType, JSONNumber},
      {GAIN, kType, JSONNumber},
  }; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void ManualExposure::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY,
                          &manual_exposure, sizeof(manual_exposure));

  json_object_set_number(json_obj, EXPOSURE_TIME,
                         manual_exposure.exposure_time);
  json_object_set_number(json_obj, GAIN, manual_exposure.gain);
}

int ManualExposure::Apply(JSON_Object *obj) {
  EdgeAppLibSensorCameraManualExposureProperty aux_manual_exposure =
      manual_exposure;

  if (json_object_has_value(obj, EXPOSURE_TIME))
    aux_manual_exposure.exposure_time =
        (uint32_t)json_object_get_number(obj, EXPOSURE_TIME);
  if (json_object_has_value(obj, GAIN))
    aux_manual_exposure.gain = (float)json_object_get_number(obj, GAIN);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY,
      &aux_manual_exposure, sizeof(aux_manual_exposure));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Manual Exposure property failed to be set. Please use valid values "
        "for exposure_time and gain.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void ManualExposure::StoreValue(uint32_t exp, float gain) {
  /* LCOV_EXCL_BR_START: check if initialized */
  if (manual_exposure.exposure_time == exp &&
      IsAlmostEqual(manual_exposure.gain, gain))
    return;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating ManualExposure");
  json_object_set_number(json_obj, EXPOSURE_TIME, exp);
  json_object_set_number(json_obj, GAIN, gain);

  manual_exposure = {.exposure_time = exp, .gain = gain};
}
