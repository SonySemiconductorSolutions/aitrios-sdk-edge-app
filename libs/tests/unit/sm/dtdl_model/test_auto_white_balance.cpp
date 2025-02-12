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

#define CONVERGENCE_SPEED "convergence_speed"
#define TEST_INPUT "{\"convergence_speed\": 4400}"

using namespace EdgeAppLib;

TEST(AutoWhiteBalance, Parse) {
  AutoWhiteBalance obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST(AutoWhiteBalance, VerifyFailNotInRange) {
  AutoWhiteBalance obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "convergence_speed", -17);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "convergence_speed not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(AutoWhiteBalance, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  AutoWhiteBalance *obj = context->GetDtdlModel()
                              ->GetCommonSettings()
                              ->GetPqSettings()
                              ->GetAutoWhiteBalance();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  EdgeAppLibSensorAutoWhiteBalanceProperty wb_speed;
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY,
                          &wb_speed, sizeof(wb_speed));
  ASSERT_EQ(wb_speed.convergence_speed, 4400);

  json_object_set_number(obj1, "convergence_speed", 5000);
  obj->Apply(obj1);
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY,
                          &wb_speed, sizeof(wb_speed));
  ASSERT_EQ(wb_speed.convergence_speed, 5000);

  json_object_set_number(obj1, "convergence_speed", 6000);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj->Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY,
                          &wb_speed, sizeof(wb_speed));
  ASSERT_EQ(wb_speed.convergence_speed, 5000);
}

TEST(AutoWhiteBalance, InitializeValues) {
  AutoWhiteBalance obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_TRUE(json_object_has_value(json, CONVERGENCE_SPEED) == 0);
  obj.InitializeValues();
  ASSERT_TRUE(json_object_has_value(json, CONVERGENCE_SPEED) == 1);
  obj.Delete();
}
