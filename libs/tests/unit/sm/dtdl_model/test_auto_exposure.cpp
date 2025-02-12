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

#define MAX_EXPOSURE_TIME "max_exposure_time"
#define MIN_EXPOSURE_TIME "min_exposure_time"
#define MAX_GAIN "max_gain"
#define CONVERGENCE_SPEED "convergence_speed"

#define TEST_INPUT_PATTERN      \
  "{\"max_exposure_time\": %d," \
  "\"min_exposure_time\": %d, " \
  "\"max_gain\": 0.353791,"     \
  "\"convergence_speed\": 5}"

#define TEST_INPUT             \
  "{\"max_exposure_time\": 8," \
  "\"min_exposure_time\": 1, " \
  "\"max_gain\": 0.353791,"    \
  "\"convergence_speed\": 5}"

using namespace EdgeAppLib;

class AutoExposureParam
    : public ::testing::TestWithParam<std::tuple<int, int, float, int>> {
 public:
  void SetUp() override { context = StateMachineContext::GetInstance(nullptr); }
  void TearDown() override { context->Delete(); }

  StateMachineContext *context = nullptr;
};

TEST(AutoExposure, Parse) {
  AutoExposure obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST(AutoExposure, VerifyFailMinMaxExposure) {
  AutoExposure obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "max_exposure_time", 7);
  json_object_set_number(obj1, "min_exposure_time", 10);
  ASSERT_EQ(obj.Verify(obj1), -1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  ASSERT_STREQ(
      context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
      "Parameter min_exposure_time can not be greater than max_exposure_time");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST_P(AutoExposureParam, Invalid) {
  auto [max_exposure_time, min_exposure_time, max_gain, convergence_speed] =
      GetParam();
  char input[100];
  sprintf(input, TEST_INPUT_PATTERN, max_exposure_time, min_exposure_time);

  JSON_Value *value = json_parse_string(input);

  AutoExposure obj;
  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_value_free(value);
}
INSTANTIATE_TEST_SUITE_P(NegativeCombinations, AutoExposureParam,
                         ::testing::Values(std::make_tuple(1, -1, 0.5, 5),
                                           std::make_tuple(-1, -10, 0.5, 5)));

TEST_F(AutoExposureParam, VerifyFailMaxGain) {
  AutoExposure obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "max_gain", -1);

  ASSERT_EQ(obj.Verify(json_object(value)), 0);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(), "");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(), CODE_OK);

  json_value_free(value);
}

TEST_F(AutoExposureParam, VerifyFailConvSpeed) {
  AutoExposure obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "convergence_speed", -1);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "convergence_speed not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_value_free(value);
}

TEST(AutoExposure, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  AutoExposure *obj = context->GetDtdlModel()
                          ->GetCommonSettings()
                          ->GetPqSettings()
                          ->GetAutoExposure();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraAutoExposureProperty auto_exposure;

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY,
                          &auto_exposure, sizeof(auto_exposure));

  ASSERT_EQ(auto_exposure.max_exposure_time, 8);
  ASSERT_EQ(auto_exposure.min_exposure_time, 1);
  ASSERT_LT(abs(auto_exposure.max_gain - (float)0.353791), TOLERANCE);
  ASSERT_EQ(auto_exposure.convergence_speed, 5);

  json_object_set_number(obj1, "max_exposure_time", 15);
  json_object_set_number(obj1, "min_exposure_time", 7);
  json_object_set_number(obj1, "max_gain", 0.5);
  json_object_set_number(obj1, "convergence_speed", 1);
  obj->Apply(obj1);

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY,
                          &auto_exposure, sizeof(auto_exposure));

  ASSERT_EQ(auto_exposure.max_exposure_time, 15);
  ASSERT_EQ(auto_exposure.min_exposure_time, 7);
  ASSERT_EQ(auto_exposure.max_gain, 0.5);
  ASSERT_EQ(auto_exposure.convergence_speed, 1);

  json_object_set_number(obj1, "max_exposure_time", 14);
  json_object_set_number(obj1, "min_exposure_time", 8);
  json_object_set_number(obj1, "max_gain", 0.1);
  json_object_set_number(obj1, "convergence_speed", 2);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj->Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY,
                          &auto_exposure, sizeof(auto_exposure));
  ASSERT_EQ(auto_exposure.max_exposure_time, 15);
  ASSERT_EQ(auto_exposure.min_exposure_time, 7);
  ASSERT_EQ(auto_exposure.max_gain, 0.5);
  ASSERT_EQ(auto_exposure.convergence_speed, 1);
}

TEST(AutoExposure, InitializeValues) {
  AutoExposure obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_TRUE(json_object_has_value(json, MAX_EXPOSURE_TIME) == 0);
  ASSERT_TRUE(json_object_has_value(json, MIN_EXPOSURE_TIME) == 0);
  ASSERT_TRUE(json_object_has_value(json, MAX_GAIN) == 0);
  ASSERT_TRUE(json_object_has_value(json, CONVERGENCE_SPEED) == 0);
  obj.InitializeValues();
  ASSERT_TRUE(json_object_has_value(json, MAX_EXPOSURE_TIME) == 1);
  ASSERT_TRUE(json_object_has_value(json, MIN_EXPOSURE_TIME) == 1);
  ASSERT_TRUE(json_object_has_value(json, MAX_GAIN) == 1);
  ASSERT_TRUE(json_object_has_value(json, CONVERGENCE_SPEED) == 1);
  obj.Delete();
}
