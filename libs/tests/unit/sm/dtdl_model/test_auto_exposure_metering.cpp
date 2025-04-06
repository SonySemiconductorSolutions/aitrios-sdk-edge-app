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

#include <cstdlib>

#include "dtdl_model/objects/common_settings.hpp"
#include "dtdl_model/properties.h"
#include "parson.h"
#include "sensor.h"
#include "sensor/mock_sensor.hpp"
#include "sm_context.hpp"

#define MODE "metering_mode"
#define TOP "top"
#define LEFT "left"
#define BOTTOM "bottom"
#define RIGHT "right"

#define TEST_INPUT                                                   \
  "{\"" MODE "\": 1, \"" TOP "\": 160, \"" LEFT "\": 120, \"" BOTTOM \
  "\": 480, \"" RIGHT "\": 360}"

using namespace EdgeAppLib;

class AutoExposureMeteringParam
    : public ::testing::TestWithParam<std::tuple<double, int, int, int, int>> {
 public:
  void SetUp() override { context = StateMachineContext::GetInstance(nullptr); }
  void TearDown() override { context->Delete(); }

  StateMachineContext *context = nullptr;
};

TEST(AutoExposureMetering, Parse) {
  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);

  ASSERT_EQ(obj.Verify(json_object(value)), 0);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifySuccessMode) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, MODE, 0);
  ASSERT_EQ(obj.Verify(obj1), 0);

  json_object_set_number(obj1, MODE, 1);
  ASSERT_EQ(obj.Verify(obj1), 0);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifyFailMode) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, MODE, -1);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "metering_mode not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, MODE, 2);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "metering_mode not <= 1.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifySuccessTop) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, MODE, 1);
  json_object_set_number(obj1, TOP, 0);
  ASSERT_EQ(obj.Verify(obj1), 0);

  json_object_set_number(obj1, TOP, 160);
  json_object_set_number(obj1, BOTTOM, 161);
  ASSERT_EQ(obj.Verify(obj1), 0);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifyFailTop) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, MODE, 1);
  json_object_set_number(obj1, TOP, -1);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "top not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, TOP, 160);
  json_object_set_number(obj1, BOTTOM, 100);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "top not top < bottom");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, TOP, 320);
  json_object_set_number(obj1, BOTTOM, 320);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "top not top < bottom");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifySuccessLeft) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, LEFT, 0);
  ASSERT_EQ(obj.Verify(obj1), 0);

  json_object_set_number(obj1, LEFT, 160);
  json_object_set_number(obj1, RIGHT, 161);
  ASSERT_EQ(obj.Verify(obj1), 0);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifyFailLeft) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, LEFT, -1);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "left not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, LEFT, 120);
  json_object_set_number(obj1, RIGHT, 100);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "left not left < right");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, LEFT, 120);
  json_object_set_number(obj1, RIGHT, 120);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "left not left < right");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifyFailRight) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, MODE, 1);
  json_object_set_number(obj1, RIGHT, -1);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "right not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, VerifyFailBottom) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, MODE, 1);
  json_object_set_number(obj1, BOTTOM, -1);
  ASSERT_EQ(obj.Verify(obj1), -1);
  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "bottom not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposureMetering, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposureMetering *obj = context->GetDtdlModel()
                                  ->GetCommonSettings()
                                  ->GetPqSettings()
                                  ->GetAutoExposureMetering();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraAutoExposureMeteringProperty auto_exposure_metering;

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY,
      &auto_exposure_metering, sizeof(auto_exposure_metering));

  ASSERT_EQ(auto_exposure_metering.mode, 1);
  ASSERT_EQ(auto_exposure_metering.top, 160);
  ASSERT_EQ(auto_exposure_metering.left, 120);
  ASSERT_EQ(auto_exposure_metering.bottom, 480);
  ASSERT_EQ(auto_exposure_metering.right, 360);

  json_object_set_number(obj1, MODE, 0);
  json_object_set_number(obj1, TOP, 320);
  json_object_set_number(obj1, LEFT, 240);
  json_object_set_number(obj1, BOTTOM, 960);
  json_object_set_number(obj1, RIGHT, 720);
  obj->Apply(obj1);

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY,
      &auto_exposure_metering, sizeof(auto_exposure_metering));

  ASSERT_EQ(auto_exposure_metering.mode, 0);
  ASSERT_EQ(auto_exposure_metering.top, 320);
  ASSERT_EQ(auto_exposure_metering.left, 240);
  ASSERT_EQ(auto_exposure_metering.bottom, 960);
  ASSERT_EQ(auto_exposure_metering.right, 720);

  json_object_set_number(obj1, MODE, 2);
  json_object_set_number(obj1, TOP, 320);
  json_object_set_number(obj1, LEFT, 240);
  json_object_set_number(obj1, BOTTOM, 160);
  json_object_set_number(obj1, RIGHT, 120);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj->Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY,
      &auto_exposure_metering, sizeof(auto_exposure_metering));
  ASSERT_EQ(auto_exposure_metering.mode, 0);
  ASSERT_EQ(auto_exposure_metering.top, 320);
  ASSERT_EQ(auto_exposure_metering.left, 240);
  ASSERT_EQ(auto_exposure_metering.bottom, 960);
  ASSERT_EQ(auto_exposure_metering.right, 720);
}
