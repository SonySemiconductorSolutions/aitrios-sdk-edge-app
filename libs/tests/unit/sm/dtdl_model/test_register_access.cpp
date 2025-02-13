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

#define TEST_INPUT                                                     \
  "{\"bit_length\": 0, \"id\": 0, \"address\": \"AB54A98CEB1F0AD2\", " \
  "\"data\": \"123\"}"

#define BIT_LENGTH "bit_length"
#define ID "id"
#define ADDRESS "address"
#define DATA "data"

using namespace EdgeAppLib;

TEST(RegisterAccess, CheckNotification) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  RegisterAccess obj;

  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  ASSERT_FALSE(context->IsPendingNotification());
  obj.Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());
}

TEST(RegisterAccess, InitializeValues) {
  RegisterAccess obj;
  JSON_Object *json = obj.GetJsonObject();
  ASSERT_EQ(json_object_has_value(json, BIT_LENGTH), 0);
  ASSERT_EQ(json_object_has_value(json, ID), 0);
  ASSERT_EQ(json_object_has_value(json, ADDRESS), 0);
  ASSERT_EQ(json_object_has_value(json, DATA), 0);
  obj.InitializeValues();
  ASSERT_EQ(json_object_has_value(json, BIT_LENGTH), 1);
  ASSERT_EQ(json_object_has_value(json, ID), 1);
  ASSERT_EQ(json_object_has_value(json, ADDRESS), 1);
  ASSERT_EQ(json_object_has_value(json, DATA), 1);
  obj.Delete();
}

TEST(RegisterAccess, Invalid) {
  JSON_Value *value = json_parse_string(TEST_INPUT);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  RegisterAccess obj;

  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "bit_length", -1);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "bit_length", 0);
  json_object_set_number(obj1, "id", -1);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "id", 0);
  json_object_set_number(obj1, "data", 0);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_string(obj1, "data", "13");
  json_object_set_number(obj1, "address", 1);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_string(obj1, "address", "AB54A98CEB1F0AD2");
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
}
