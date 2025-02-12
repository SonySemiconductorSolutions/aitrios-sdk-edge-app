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

#define TEST_INPUT "{ \"color_temperature\": 1}"
#define COLOR_TEMPERATURE "color_temperature"

using namespace EdgeAppLib;

TEST(ManualWhiteBalancePreset, Parse) {
  ManualWhiteBalancePreset obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST(ManualWhiteBalancePreset, Invalid) {
  ManualWhiteBalancePreset obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);

  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "color_temperature", -3);
  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "color_temperature not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_object_set_number(obj1, "color_temperature", 5);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "color_temperature not <= 3.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(ManualWhiteBalancePreset, CheckNotification) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  ManualWhiteBalancePreset obj;
  EdgeAppLibSensorManualWhiteBalancePresetProperty manual_wb_preset;

  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  ASSERT_FALSE(context->IsPendingNotification());
  obj.Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj.Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &manual_wb_preset, sizeof(manual_wb_preset));
  ASSERT_EQ(manual_wb_preset.color_temperature, 4300);

  json_object_set_number(obj1, "color_temperature", 0);
  obj.Apply(obj1);
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &manual_wb_preset, sizeof(manual_wb_preset));
  ASSERT_EQ(manual_wb_preset.color_temperature, 3200);

  json_object_set_number(obj1, "color_temperature", 2);
  obj.Apply(obj1);
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &manual_wb_preset, sizeof(manual_wb_preset));
  ASSERT_EQ(manual_wb_preset.color_temperature, 5600);

  json_object_set_number(obj1, "color_temperature", 3);
  obj.Apply(obj1);
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &manual_wb_preset, sizeof(manual_wb_preset));
  ASSERT_EQ(manual_wb_preset.color_temperature, 6500);

  json_object_set_number(obj1, "color_temperature", 2);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj.Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &manual_wb_preset, sizeof(manual_wb_preset));
  ASSERT_EQ(manual_wb_preset.color_temperature, 6500);
}

TEST(ManualWhiteBalancePreset, InitializeValues) {
  ManualWhiteBalancePreset obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_TRUE(json_object_has_value(json, COLOR_TEMPERATURE) == 0);

  EdgeAppLibSensorManualWhiteBalancePresetProperty manual_wb_preset = {
      .color_temperature = 6500};
  SensorStreamSetProperty(
      StateMachineContext::GetInstance(nullptr)->GetSensorStream(),
      AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
      &manual_wb_preset, sizeof(manual_wb_preset));

  obj.InitializeValues();
  ASSERT_TRUE(json_object_get_number(json, COLOR_TEMPERATURE) == 3);
  obj.Delete();
}
