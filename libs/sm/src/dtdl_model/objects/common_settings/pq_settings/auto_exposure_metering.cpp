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

#include "dtdl_model/objects/common_settings/pq_settings/auto_exposure_metering.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *MODE = "metering_mode";
static const char *TOP = "top";
static const char *LEFT = "left";
static const char *BOTTOM = "bottom";
static const char *RIGHT = "right";

AutoExposureMetering::AutoExposureMetering() {
  static Validation s_validation[] = {
      {MODE, kGe,
       AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_MODE_FULL_SCREEN},
      {MODE, kLe,
       AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_MODE_USER_WINDOW},
      {MODE, kType, JSONNumber},
      {TOP, kGe, 0},
      {TOP, kType, JSONNumber},
      {LEFT, kGe, 0},
      {LEFT, kType, JSONNumber},
      {BOTTOM, kGe, 0},
      {BOTTOM, kType, JSONNumber},
      {RIGHT, kGe, 0},
      {RIGHT, kType, JSONNumber},
  }; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validation, sizeof(s_validation) / sizeof(Validation));
}

void AutoExposureMetering::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY,
      &auto_exposure_metering, sizeof(auto_exposure_metering));

  json_object_set_number(json_obj, MODE, auto_exposure_metering.mode);
  json_object_set_number(json_obj, TOP, auto_exposure_metering.top);
  json_object_set_number(json_obj, LEFT, auto_exposure_metering.left);
  json_object_set_number(json_obj, BOTTOM, auto_exposure_metering.bottom);
  json_object_set_number(json_obj, RIGHT, auto_exposure_metering.right);
}

int AutoExposureMetering::Verify(JSON_Object *obj) {
  if (JsonObject::Verify(obj) != 0) return -1;

  EdgeAppLibSensorCameraAutoExposureMeteringProperty
      aux_auto_exposure_metering = auto_exposure_metering;

  if (json_object_has_value(obj, MODE))
    aux_auto_exposure_metering.mode =
        (EdgeAppLibSensorCameraAutoExposureMeteringMode)json_object_get_number(
            obj, MODE);
  if (json_object_has_value(obj, TOP))
    aux_auto_exposure_metering.top = json_object_get_number(obj, TOP);
  if (json_object_has_value(obj, LEFT))
    aux_auto_exposure_metering.left = json_object_get_number(obj, LEFT);
  if (json_object_has_value(obj, BOTTOM))
    aux_auto_exposure_metering.bottom = json_object_get_number(obj, BOTTOM);
  if (json_object_has_value(obj, RIGHT))
    aux_auto_exposure_metering.right = json_object_get_number(obj, RIGHT);

  if (aux_auto_exposure_metering.mode ==
      AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_MODE_FULL_SCREEN)
    return 0;

  if (!(aux_auto_exposure_metering.top < aux_auto_exposure_metering.bottom)) {
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg("top not top < bottom");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    return -1;
  }

  if (!(aux_auto_exposure_metering.left < aux_auto_exposure_metering.right)) {
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg("left not left < right");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    return -1;
  }

  return 0;
}

int AutoExposureMetering::Apply(JSON_Object *obj) {
  EdgeAppLibSensorCameraAutoExposureMeteringProperty
      aux_auto_exposure_metering = auto_exposure_metering;

  if (json_object_has_value(obj, MODE))
    aux_auto_exposure_metering.mode =
        (EdgeAppLibSensorCameraAutoExposureMeteringMode)json_object_get_number(
            obj, MODE);
  if (json_object_has_value(obj, TOP))
    aux_auto_exposure_metering.top = json_object_get_number(obj, TOP);
  if (json_object_has_value(obj, LEFT))
    aux_auto_exposure_metering.left = json_object_get_number(obj, LEFT);
  if (json_object_has_value(obj, BOTTOM))
    aux_auto_exposure_metering.bottom = json_object_get_number(obj, BOTTOM);
  if (json_object_has_value(obj, RIGHT))
    aux_auto_exposure_metering.right = json_object_get_number(obj, RIGHT);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY,
      &aux_auto_exposure_metering, sizeof(aux_auto_exposure_metering));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Auto Exposure Metering property failed to be set. Please use valid "
        "values for mode, top, left, bottom and right.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void AutoExposureMetering::StoreValue(double mode, uint32_t top, uint32_t left,
                                      uint32_t bottom, uint32_t right) {
  /* LCOV_EXCL_BR_START: check if initialized */
  if (mode == auto_exposure_metering.mode &&
      top == auto_exposure_metering.top &&
      left == auto_exposure_metering.left &&
      bottom == auto_exposure_metering.bottom &&
      right == auto_exposure_metering.right)
    return;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating AutoExposureMetering");
  json_object_set_number(json_obj, MODE, mode);
  json_object_set_number(json_obj, TOP, top);
  json_object_set_number(json_obj, LEFT, left);
  json_object_set_number(json_obj, BOTTOM, bottom);
  json_object_set_number(json_obj, RIGHT, right);

  auto_exposure_metering = EdgeAppLibSensorCameraAutoExposureMeteringProperty{
      .mode = (EdgeAppLibSensorCameraAutoExposureMeteringMode)mode,
      .top = top,
      .left = left,
      .bottom = bottom,
      .right = right};
}
