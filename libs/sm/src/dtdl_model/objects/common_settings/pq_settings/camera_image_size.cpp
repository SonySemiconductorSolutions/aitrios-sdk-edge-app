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

#include "dtdl_model/objects/common_settings/pq_settings/camera_image_size.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *WIDTH = "width";
static const char *HEIGHT = "height";
static const char *SCALING_POLICY = "scaling_policy";

CameraImageSize::CameraImageSize() {
  static Validation s_validations[7] = {
      {WIDTH, kGe, 0},
      {HEIGHT, kGe, 0},
      {SCALING_POLICY, kGe, 1},
      {SCALING_POLICY, kLe, 2},
      {WIDTH, kType, JSONNumber},
      {HEIGHT, kType, JSONNumber},
      {SCALING_POLICY, kType, JSONNumber},
  }; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void CameraImageSize::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY,
                          &camera_size, sizeof(camera_size));

  json_object_set_number(json_obj, WIDTH, camera_size.width);
  json_object_set_number(json_obj, HEIGHT, camera_size.height);
  json_object_set_number(json_obj, SCALING_POLICY, camera_size.scaling_policy);
}

int CameraImageSize::Apply(JSON_Object *obj) {
  EdgeAppLibSensorCameraImageSizeProperty aux_camera_size = camera_size;

  if (json_object_has_value(obj, WIDTH))
    aux_camera_size.width = json_object_get_number(obj, WIDTH);
  if (json_object_has_value(obj, HEIGHT))
    aux_camera_size.height = json_object_get_number(obj, HEIGHT);
  if (json_object_has_value(obj, SCALING_POLICY))
    aux_camera_size.scaling_policy =
        (enum EdgeAppLibSensorCameraScalingPolicy)json_object_get_number(
            obj, SCALING_POLICY);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY, &aux_camera_size,
      sizeof(aux_camera_size));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Camera Image Size property failed to be set. Please use valid values "
        "for width, height and scaling_policy.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void CameraImageSize::StoreValue(uint32_t width, uint32_t height,
                                 int scaling_policy) {
  /* LCOV_EXCL_BR_START: check if initialized */
  if (camera_size.width == width && camera_size.height == height &&
      camera_size.scaling_policy == scaling_policy)
    return;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating CameraImageSize");
  json_object_set_number(json_obj, WIDTH, width);
  json_object_set_number(json_obj, HEIGHT, height);
  json_object_set_number(json_obj, SCALING_POLICY, scaling_policy);

  camera_size = {
      .width = width,
      .height = height,
      .scaling_policy = (EdgeAppLibSensorCameraScalingPolicy)scaling_policy};
}
