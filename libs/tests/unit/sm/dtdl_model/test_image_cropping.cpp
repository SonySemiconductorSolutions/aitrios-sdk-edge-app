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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dtdl_model/objects/common_settings.hpp"
#include "dtdl_model/properties.h"
#include "fixtures/common_fixture.hpp"
#include "parson.h"
#include "sensor.h"
#include "sensor/mock_sensor.hpp"
#include "sm_context.hpp"

#define TEST_INPUT_PATTERN \
  "{\"left\": %d, \"top\": %d, \"width\": %d, \"height\": %d}"

#define TEST_INPUT "{\"left\": 10, \"top\": 10, \"width\": 0, \"height\": 9}"
#define TEST_INPUT_INCOMPLETE "{\"left\": 10, \"top\": 10, \"height\": 9}"

#define LEFT "left"
#define TOP "top"
#define WIDTH "width"
#define HEIGHT "height"

using namespace EdgeAppLib;

TEST(ImageCropping, Invalid) {
  JSON_Value *value = json_parse_string(TEST_INPUT);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  ImageCropping obj;
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "left", -17);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "left not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "left", 1);
  json_object_set_number(obj1, "top", -17);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "top not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "top", 7);
  json_object_set_number(obj1, "width", -7);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "width not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "width", 7);
  json_object_set_number(obj1, "height", -17);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "height not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_value_free(value);
  obj.Delete();
}

TEST(ImageCropping, CheckNotification) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  ImageCropping obj;
  EdgeAppLibSensorImageCropProperty image_crop_property;

  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  ASSERT_FALSE(context->IsPendingNotification());
  obj.Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj.Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                          &image_crop_property, sizeof(image_crop_property));
  ASSERT_EQ(image_crop_property.left, 10);
  ASSERT_EQ(image_crop_property.top, 10);
  ASSERT_EQ(image_crop_property.width, 0);
  ASSERT_EQ(image_crop_property.height, 9);

  json_object_set_number(obj1, "left", 11);
  json_object_set_number(obj1, "top", 12);
  json_object_set_number(obj1, "width", 23);
  json_object_set_number(obj1, "height", 14);
  obj.Apply(obj1);
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                          &image_crop_property, sizeof(image_crop_property));
  ASSERT_EQ(image_crop_property.left, 11);
  ASSERT_EQ(image_crop_property.top, 12);
  ASSERT_EQ(image_crop_property.width, 23);
  ASSERT_EQ(image_crop_property.height, 14);

  json_object_set_number(obj1, "left", 21);
  json_object_set_number(obj1, "top", 22);
  json_object_set_number(obj1, "width", 33);
  json_object_set_number(obj1, "height", 24);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj.Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                          &image_crop_property, sizeof(image_crop_property));
  ASSERT_EQ(image_crop_property.left, 11);
  ASSERT_EQ(image_crop_property.top, 12);
  ASSERT_EQ(image_crop_property.width, 23);
  ASSERT_EQ(image_crop_property.height, 14);
}

TEST(ImageCropping, InitializeValues) {
  ImageCropping obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_EQ(json_object_has_value(json, LEFT), 0);
  ASSERT_EQ(json_object_has_value(json, TOP), 0);
  ASSERT_EQ(json_object_has_value(json, WIDTH), 0);
  ASSERT_EQ(json_object_has_value(json, HEIGHT), 0);
  obj.InitializeValues();
  ASSERT_EQ(json_object_has_value(json, LEFT), 1);
  ASSERT_EQ(json_object_has_value(json, TOP), 1);
  ASSERT_EQ(json_object_has_value(json, WIDTH), 1);
  ASSERT_EQ(json_object_has_value(json, HEIGHT), 1);
  obj.Delete();
}
