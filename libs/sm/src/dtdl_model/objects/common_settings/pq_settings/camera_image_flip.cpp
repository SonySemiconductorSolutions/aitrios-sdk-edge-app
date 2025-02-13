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

#include "dtdl_model/objects/common_settings/pq_settings/camera_image_flip.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *FLIP_HORIZONTAL = "flip_horizontal";
static const char *FLIP_VERTICAL = "flip_vertical";

CameraImageFlip::CameraImageFlip() {
  static Validation s_validations[6] = {
      {FLIP_HORIZONTAL, kGe, 0},
      {FLIP_VERTICAL, kGe, 0},
      {FLIP_HORIZONTAL, kLe, 1},
      {FLIP_VERTICAL, kLe, 1},
      {FLIP_VERTICAL, kType, JSONNumber},
      {FLIP_HORIZONTAL, kType,
       JSONNumber}}; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void CameraImageFlip::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY,
                          &camera_flip, sizeof(camera_flip));

  json_object_set_number(json_obj, FLIP_HORIZONTAL,
                         camera_flip.flip_horizontal);
  json_object_set_number(json_obj, FLIP_VERTICAL, camera_flip.flip_vertical);
}

int CameraImageFlip::Apply(JSON_Object *obj) {
  EdgeAppLibSensorCameraImageFlipProperty aux_camera_flip = camera_flip;

  if (json_object_has_value(obj, FLIP_HORIZONTAL))
    aux_camera_flip.flip_horizontal =
        (bool)json_object_get_number(obj, FLIP_HORIZONTAL);
  if (json_object_has_value(obj, FLIP_VERTICAL))
    aux_camera_flip.flip_vertical =
        (bool)json_object_get_number(obj, FLIP_VERTICAL);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY, &aux_camera_flip,
      sizeof(aux_camera_flip));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Camera Image Flip property failed to be set. Please use valid values "
        "for flip_horizontal and flip_vertical.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void CameraImageFlip::StoreValue(bool flip_horizontal, bool flip_vertical) {
  /* LCOV_EXCL_BR_START: check if initialized */
  if (flip_horizontal == camera_flip.flip_horizontal &&
      flip_vertical == camera_flip.flip_vertical)
    return;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_DBG("Updating CameraImageFlip");
  json_object_set_number(json_obj, FLIP_HORIZONTAL, flip_horizontal);
  json_object_set_number(json_obj, FLIP_VERTICAL, flip_vertical);

  camera_flip = {.flip_horizontal = flip_horizontal,
                 .flip_vertical = flip_vertical};
}
