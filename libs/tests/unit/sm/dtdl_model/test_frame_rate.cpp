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
#include "sm_context.hpp"

#define TEST_INPUT_PATTERN "{\"num\": %d, \"denom\": %d}"
#define TEST_INPUT "{\"num\": 3, \"denom\": 1}"
#define TEST_INVALID_FLOAT_NUM "{\"num\": 0.5, \"denom\": 1}"
#define TEST_INVALID_FLOAT_DENOM "{\"num\": 5, \"denom\": 1.7}"
#define TEST_INVALID_FLOAT_ZERO "{\"num\": 0, \"denom\":0}"
#define TEST_INPUT_INCOMPLETE "{\"num\": 1}"
#define NUM "num"
#define DEN "denom"

using namespace EdgeAppLib;

class FrameRateParam : public ::testing::TestWithParam<std::tuple<int, int>> {
 public:
  void SetUp() override { context = StateMachineContext::GetInstance(nullptr); }
  void TearDown() override { context->Delete(); }

  StateMachineContext *context = nullptr;
};

TEST(FrameRate, Parse) {
  FrameRate obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST_F(FrameRateParam, InvalidFloatNum) {
  FrameRate obj;

  JSON_Value *value = json_parse_string(TEST_INVALID_FLOAT_NUM);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "Num property has to be an integer");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST_F(FrameRateParam, InvalidFloatDenom) {
  FrameRate obj;

  JSON_Value *value = json_parse_string(TEST_INVALID_FLOAT_DENOM);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "Denom property has to be an integer");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST_F(FrameRateParam, Invalid) {
  JSON_Value *value = json_parse_string(TEST_INPUT);

  FrameRate obj;
  JSON_Object *obj1 = json_object(value);

  json_object_set_number(obj1, NUM, -1);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "num not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, DEN, -1);
  json_object_set_number(obj1, NUM, 1);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "denom not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_value_free(value);
  obj.Delete();
}

TEST(FrameRate, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  FrameRate *obj = context->GetDtdlModel()
                       ->GetCommonSettings()
                       ->GetPqSettings()
                       ->GetFrameRate();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorCameraFrameRateProperty frameRate = {.num = 0, .denom = 0};
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY,
                          &frameRate, sizeof(frameRate));
  ASSERT_EQ(frameRate.num, 3);
  ASSERT_EQ(frameRate.denom, 1);
}

TEST(FrameRate, InitializeValues) {
  FrameRate obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_TRUE(json_object_has_value(json, NUM) == 0);
  ASSERT_TRUE(json_object_has_value(json, DEN) == 0);
  obj.InitializeValues();
  ASSERT_TRUE(json_object_has_value(json, NUM) == 1);
  ASSERT_TRUE(json_object_has_value(json, DEN) == 1);
  obj.Delete();
}
