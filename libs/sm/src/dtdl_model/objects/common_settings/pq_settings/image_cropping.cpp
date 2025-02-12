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

#include "dtdl_model/objects/common_settings/pq_settings/image_cropping.hpp"

#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *LEFT = "left";
static const char *TOP = "top";
static const char *WIDTH = "width";
static const char *HEIGHT = "height";

ImageCropping::ImageCropping() {
  static Validation s_validations[8] = {
      {LEFT, kGe, 0},
      {TOP, kGe, 0},
      {WIDTH, kGe, 0},
      {HEIGHT, kGe, 0},
      {LEFT, kType, JSONNumber},
      {TOP, kType, JSONNumber},
      {WIDTH, kType, JSONNumber},
      {HEIGHT, kType, JSONNumber},
  }; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void ImageCropping::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                          &image_crop_property, sizeof(image_crop_property));

  json_object_set_number(json_obj, LEFT, image_crop_property.left);
  json_object_set_number(json_obj, TOP, image_crop_property.top);
  json_object_set_number(json_obj, WIDTH, image_crop_property.width);
  json_object_set_number(json_obj, HEIGHT, image_crop_property.height);
}

int ImageCropping::Apply(JSON_Object *obj) {
  EdgeAppLibSensorImageCropProperty aux_image_crop_property =
      image_crop_property;

  if (json_object_has_value(obj, LEFT))
    aux_image_crop_property.left = (uint32_t)json_object_get_number(obj, LEFT);
  if (json_object_has_value(obj, TOP))
    aux_image_crop_property.top = (uint32_t)json_object_get_number(obj, TOP);
  if (json_object_has_value(obj, WIDTH))
    aux_image_crop_property.width =
        (uint32_t)json_object_get_number(obj, WIDTH);
  if (json_object_has_value(obj, HEIGHT))
    aux_image_crop_property.height =
        (uint32_t)json_object_get_number(obj, HEIGHT);

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &aux_image_crop_property,
      sizeof(aux_image_crop_property));

  if (result != 0) {
    EdgeAppLibSensorErrorCause cause = SmUtilsPrintSensorError();
    CODE code = CodeFromSensorErrorCause(cause);
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Image Crop property failed to be set. Please use valid values for "
        "left, top, width and height.");
    dtdl->GetResInfo()->SetCode(code);
  }
  return result;
}

void ImageCropping::StoreValue(uint32_t left, uint32_t top, uint32_t width,
                               uint32_t height) {
  /* LCOV_EXCL_BR_START: check if initialized */
  if (left == image_crop_property.left && top == image_crop_property.top &&
      width == image_crop_property.width &&
      height == image_crop_property.height)
    return;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  LOG_INFO("Updating ImageCropping");
  json_object_set_number(json_obj, LEFT, left);
  json_object_set_number(json_obj, TOP, top);
  json_object_set_number(json_obj, WIDTH, width);
  json_object_set_number(json_obj, HEIGHT, height);

  image_crop_property = {
      .left = left, .top = top, .width = width, .height = height};
}
