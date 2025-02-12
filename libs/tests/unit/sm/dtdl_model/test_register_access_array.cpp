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

#define TEST_INPUT_A                                                   \
  "{\"bit_length\": 0, \"id\": 0, \"address\": \"AB54A98CEB1F0AD2\", " \
  "\"data\": \"123\"}"
#define TEST_INPUT_B                                                   \
  "{\"bit_length\": 1, \"id\": 0, \"address\": \"AB54A98EEE391EEA\", " \
  "\"data\": \"12345\"}"
#define TEST_INPUT_C                                                   \
  "{\"bit_length\": 2, \"id\": 0, \"address\": \"9A3298AFB5AC71C8\", " \
  "\"data\": \"123456789\"}"
#define TEST_INPUT_D                                                   \
  "{\"bit_length\": 3, \"id\": 0, \"address\": \"8AC7230489E80001\", " \
  "\"data\": \"12345678901234567890\"}"

#define BIT_LENGTH "bit_length"
#define ID "id"
#define ADDRESS "address"
#define DATA "data"

using namespace EdgeAppLib;
TEST(RegisterAccessArray, CheckNotification) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  DtdlModel *dtdl = context->GetDtdlModel();
  RegisterAccessArray *array =
      dtdl->GetCommonSettings()->GetPqSettings()->GetRegisterAccessArray();

  EdgeAppLibSensorRegisterAccess8Property register_access_8_property;
  EdgeAppLibSensorRegisterAccess16Property register_access_16_property;
  EdgeAppLibSensorRegisterAccess32Property register_access_32_property;
  EdgeAppLibSensorRegisterAccess64Property register_access_64_property;

  JSON_Value *value_a = json_parse_string(TEST_INPUT_A);
  JSON_Object *object_a = json_object(value_a);
  JSON_Value *array_value_a = json_value_init_array();
  JSON_Array *array_a = json_value_get_array(array_value_a);
  json_array_append_value(array_a, value_a);

  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_a);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_a);
  ASSERT_FALSE(context->IsPendingNotification());

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY,
                          &register_access_8_property,
                          sizeof(register_access_8_property));
  ASSERT_EQ(register_access_8_property.id, 0);
  ASSERT_EQ(register_access_8_property.address, 12345678901234567890);
  ASSERT_EQ(register_access_8_property.data, 123);

  JSON_Value *value_b = json_parse_string(TEST_INPUT_B);
  JSON_Object *object_b = json_object(value_b);
  JSON_Value *array_value_b = json_value_init_array();
  JSON_Array *array_b = json_value_get_array(array_value_b);
  json_array_append_value(array_b, value_b);

  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_b);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_b);
  ASSERT_FALSE(context->IsPendingNotification());

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY,
                          &register_access_8_property,
                          sizeof(register_access_8_property));
  ASSERT_EQ(register_access_8_property.id, 0);
  ASSERT_EQ(register_access_8_property.address, 12345678901234567890);
  ASSERT_EQ(register_access_8_property.data, 123);

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY,
      &register_access_16_property, sizeof(register_access_16_property));
  ASSERT_EQ(register_access_16_property.id, 0);
  ASSERT_EQ(register_access_16_property.address, 12345678909876543210);
  ASSERT_EQ(register_access_16_property.data, 12345);

  JSON_Value *value_c = json_parse_string(TEST_INPUT_C);
  JSON_Object *object_c = json_object(value_c);
  JSON_Value *array_value_bc = json_value_init_array();
  JSON_Array *array_bc = json_value_get_array(array_value_bc);
  json_array_append_value(array_bc, json_value_deep_copy(value_b));
  json_array_append_value(array_bc, value_c);

  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_bc);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_bc);
  ASSERT_FALSE(context->IsPendingNotification());

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY,
                          &register_access_8_property,
                          sizeof(register_access_8_property));
  ASSERT_EQ(register_access_8_property.id, 0);
  ASSERT_EQ(register_access_8_property.address, 12345678901234567890);
  ASSERT_EQ(register_access_8_property.data, 123);

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY,
      &register_access_16_property, sizeof(register_access_16_property));
  ASSERT_EQ(register_access_16_property.id, 0);
  ASSERT_EQ(register_access_16_property.address, 12345678909876543210);
  ASSERT_EQ(register_access_16_property.data, 12345);

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY,
      &register_access_32_property, sizeof(register_access_32_property));
  ASSERT_EQ(register_access_32_property.id, 0);
  ASSERT_EQ(register_access_32_property.address, 11111111111111111112);
  ASSERT_EQ(register_access_32_property.data, 123456789);

  JSON_Value *value_d = json_parse_string(TEST_INPUT_D);
  JSON_Object *object_d = json_object(value_d);
  JSON_Value *array_value_d = json_value_init_array();
  JSON_Array *array_d = json_value_get_array(array_value_d);
  json_array_append_value(array_d, value_d);

  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_d);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  array->Apply(array_d);
  ASSERT_FALSE(context->IsPendingNotification());

  SensorStreamGetProperty(stream, AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY,
                          &register_access_8_property,
                          sizeof(register_access_8_property));
  ASSERT_EQ(register_access_8_property.id, 0);
  ASSERT_EQ(register_access_8_property.address, 12345678901234567890);
  ASSERT_EQ(register_access_8_property.data, 123);

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY,
      &register_access_16_property, sizeof(register_access_16_property));
  ASSERT_EQ(register_access_16_property.id, 0);
  ASSERT_EQ(register_access_16_property.address, 12345678909876543210);
  ASSERT_EQ(register_access_16_property.data, 12345);

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY,
      &register_access_32_property, sizeof(register_access_32_property));
  ASSERT_EQ(register_access_32_property.id, 0);
  ASSERT_EQ(register_access_32_property.address, 11111111111111111112);
  ASSERT_EQ(register_access_32_property.data, 123456789);

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY,
      &register_access_64_property, sizeof(register_access_64_property));
  ASSERT_EQ(register_access_64_property.id, 0);
  ASSERT_EQ(register_access_64_property.address, 10000000000000000001);
  ASSERT_EQ(register_access_64_property.data, 12345678901234567890);
}

TEST(RegisterAccessArray, Invalid) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  DtdlModel *dtdl = context->GetDtdlModel();
  RegisterAccessArray *array =
      dtdl->GetCommonSettings()->GetPqSettings()->GetRegisterAccessArray();

  JSON_Value *value_a = json_parse_string(TEST_INPUT_A);
  JSON_Object *object_a = json_object(value_a);
  json_object_set_number(object_a, BIT_LENGTH, 5);
  JSON_Value *array_value_a = json_value_init_array();
  JSON_Array *array_a = json_value_get_array(array_value_a);
  json_array_append_value(array_a, value_a);
  ASSERT_EQ(array->Apply(array_a), -1);
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
}

TEST(RegisterAccessArray, OverMaxLength) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  DtdlModel *dtdl = context->GetDtdlModel();
  RegisterAccessArray *array =
      dtdl->GetCommonSettings()->GetPqSettings()->GetRegisterAccessArray();

  JSON_Value *value_a = json_parse_string(TEST_INPUT_A);
  JSON_Object *object_a = json_object(value_a);
  JSON_Value *array_value_a = json_value_init_array();
  JSON_Array *array_a = json_value_get_array(array_value_a);
  for (size_t i = 0; i < 5; i++) {
    json_array_append_value(array_a, json_value_deep_copy(value_a));
  }
  ASSERT_EQ(array->Verify(array_a), -1);
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
  ASSERT_EQ(array->Apply(array_a), 0);
}

TEST(RegisterAccessArray, Uncompleted) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  DtdlModel *dtdl = context->GetDtdlModel();
  RegisterAccessArray *array =
      dtdl->GetCommonSettings()->GetPqSettings()->GetRegisterAccessArray();

  JSON_Value *value_a = json_parse_string(TEST_INPUT_A);
  JSON_Object *object_a = json_object(value_a);
  json_object_remove(object_a, BIT_LENGTH);
  JSON_Value *array_value_a = json_value_init_array();
  JSON_Array *array_a = json_value_get_array(array_value_a);
  json_array_append_value(array_a, json_value_deep_copy(value_a));

  ASSERT_EQ(array->Verify(array_a), -1);
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
  ASSERT_EQ(array->Apply(array_a), 0);

  JSON_Value *value_b = json_parse_string(TEST_INPUT_B);
  JSON_Object *object_b = json_object(value_b);
  json_object_remove(object_b, DATA);
  JSON_Value *array_value_b = json_value_init_array();
  JSON_Array *array_b = json_value_get_array(array_value_b);
  json_array_append_value(array_b, json_value_deep_copy(value_b));

  ASSERT_EQ(array->Verify(array_b), -1);
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
  ASSERT_EQ(array->Apply(array_b), 0);

  JSON_Value *value_c = json_parse_string(TEST_INPUT_C);
  JSON_Object *object_c = json_object(value_c);
  json_object_remove(object_c, ADDRESS);
  JSON_Value *array_value_c = json_value_init_array();
  JSON_Array *array_c = json_value_get_array(array_value_c);
  json_array_append_value(array_c, json_value_deep_copy(value_c));

  ASSERT_EQ(array->Verify(array_c), -1);
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
  ASSERT_EQ(array->Apply(array_c), 0);

  JSON_Value *value_d = json_parse_string("{}");
  JSON_Object *object_d = json_object(value_d);
  JSON_Value *array_value_d = json_value_init_array();
  JSON_Array *array_d = json_value_get_array(array_value_d);
  json_array_append_value(array_d, json_value_deep_copy(value_d));

  ASSERT_EQ(array->Verify(array_d), -1);
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
  ASSERT_EQ(array->Apply(array_d), 0);
}
