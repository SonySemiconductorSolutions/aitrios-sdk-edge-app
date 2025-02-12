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
#include "parson.h"
#include "sensor.h"
#include "sensor/mock_sensor.hpp"
#include "sm_context.hpp"

#define TEST_INPUT_PATTERN \
  "{\"width\": %d, \"height\": %d, \"scaling_policy\": %d}"
#define TEST_INPUT "{\"width\": 1, \"height\": 3, \"scaling_policy\": 2}"
#define TEST_INPUT_INCOMPLETE "{\"width\": 1, \"height\": 3}"

#define WIDTH "width"
#define HEIGHT "height"
#define SCALING_POLICY "scaling_policy"
#define FLIP_HORIZONTAL "flip_horizontal"
#define FLIP_VERTICAL "flip_vertical"

using namespace EdgeAppLib;

class CameraImageParam
    : public ::testing::TestWithParam<std::tuple<int, int, int>> {
 public:
  void SetUp() override { context = StateMachineContext::GetInstance(nullptr); }
  void TearDown() override {
    SensorCoreExit(0);
    context->Delete();
  }
  StateMachineContext *context = nullptr;
};

TEST(CameraImageSize, Parse) {
  CameraImageSize obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST(CameraImageSize, Incomplete) {
  CameraImageSize obj;

  JSON_Value *value = json_parse_string(TEST_INPUT_INCOMPLETE);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST(CameraImageSize, VerifyFailNotInRange) {
  CameraImageSize obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, WIDTH, -1);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "width not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, WIDTH, 1);
  json_object_set_number(obj1, HEIGHT, -1);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "height not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, HEIGHT, 1);
  json_object_set_number(obj1, SCALING_POLICY, 0);

  ASSERT_EQ(obj.Verify(obj1), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "scaling_policy not >= 1.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, SCALING_POLICY, 3);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "scaling_policy not <= 2.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();

  json_value_free(value);
}

TEST(CameraImageSize, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  CameraImageSize *obj = context->GetDtdlModel()
                             ->GetCommonSettings()
                             ->GetPqSettings()
                             ->GetCameraImageSize();
  CameraImageSize *obj2 = context->GetDtdlModel()
                              ->GetCommonSettings()
                              ->GetPqSettings()
                              ->GetCameraImageSize();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraImageSizeProperty camera_size;
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY,
                          &camera_size, sizeof(camera_size));

  ASSERT_EQ(camera_size.width, 1);
  ASSERT_EQ(camera_size.height, 3);
  ASSERT_EQ(camera_size.scaling_policy, 2);

  json_object_set_number(obj1, "height", 24);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj->Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  EdgeAppLibSensorStream stream2 = context->GetSensorStream();
  EdgeAppLibSensorCameraImageSizeProperty camera_size2;
  SensorStreamGetProperty(stream2,
                          AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY,
                          &camera_size2, sizeof(camera_size2));
  ASSERT_EQ(camera_size2.width, 1);
  ASSERT_EQ(camera_size2.height, 3);
  ASSERT_EQ(camera_size2.scaling_policy, 2);

  json_value_free(value1);
}

TEST(CameraImageSize, InitializeValues) {
  CameraImageSize obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_TRUE(json_object_has_value(json, WIDTH) == 0);
  ASSERT_TRUE(json_object_has_value(json, HEIGHT) == 0);
  ASSERT_TRUE(json_object_has_value(json, SCALING_POLICY) == 0);
  obj.InitializeValues();
  ASSERT_TRUE(json_object_has_value(json, WIDTH) == 1);
  ASSERT_TRUE(json_object_has_value(json, HEIGHT) == 1);
  ASSERT_TRUE(json_object_has_value(json, SCALING_POLICY) == 1);
  obj.Delete();
}

#define TEST_INPUT_FLIP "{\"flip_horizontal\": 1, \"flip_vertical\": 0}"

TEST(CameraImageFlip, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT_FLIP);
  JSON_Object *obj1 = json_object(value1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  CameraImageFlip *obj = context->GetDtdlModel()
                             ->GetCommonSettings()
                             ->GetPqSettings()
                             ->GetCameraImageFlip();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraImageFlipProperty camera_flip;
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY,
                          &camera_flip, sizeof(camera_flip));

  ASSERT_EQ(camera_flip.flip_horizontal, 1);
  ASSERT_EQ(camera_flip.flip_vertical, 0);

  json_object_set_number(obj1, "flip_horizontal", 0);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj->Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  EdgeAppLibSensorStream stream2 = context->GetSensorStream();
  EdgeAppLibSensorCameraImageFlipProperty camera_flip2;
  SensorStreamGetProperty(stream2,
                          AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY,
                          &camera_flip2, sizeof(camera_flip2));
  ASSERT_EQ(camera_flip2.flip_horizontal, 1);
  ASSERT_EQ(camera_flip2.flip_vertical, 0);

  json_value_free(value1);
}

TEST(CameraImageFlip, InitializeValues) {
  CameraImageFlip obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_TRUE(json_object_has_value(json, FLIP_HORIZONTAL) == 0);
  ASSERT_TRUE(json_object_has_value(json, FLIP_VERTICAL) == 0);
  obj.InitializeValues();
  ASSERT_TRUE(json_object_has_value(json, FLIP_HORIZONTAL) == 1);
  ASSERT_TRUE(json_object_has_value(json, FLIP_VERTICAL) == 1);
  obj.Delete();
}

TEST(CameraImageFlip, VerifyFailNotInRange) {
  CameraImageFlip obj;

  JSON_Value *value = json_parse_string(TEST_INPUT_FLIP);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "flip_vertical", 2);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "flip_vertical not <= 1.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "flip_vertical", -12);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "flip_vertical not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "flip_vertical", 1);
  json_object_set_number(obj1, "flip_horizontal", 17);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "flip_horizontal not <= 1.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "flip_horizontal", -10);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "flip_horizontal not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}
