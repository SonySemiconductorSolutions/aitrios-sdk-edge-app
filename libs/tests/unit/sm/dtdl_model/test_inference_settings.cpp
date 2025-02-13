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
#include "sm_context.hpp"

#define TEST_INPUT "{\"number_of_iterations\": 5}"
#define TEST_INPUT_INCOMPLETE "{}"

TEST(InferenceSettings, Parse) {
  InferenceSettings obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST(InferenceSettings, Invalid) {
  InferenceSettings obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "number_of_iterations", -10);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "number_of_iterations not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  obj.Delete();
  json_value_free(value);
}

TEST(InferenceSettings, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->ClearNotification();

  InferenceSettings obj;
  ASSERT_FALSE(context->IsPendingNotification());
  obj.Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj.Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  JSON_Object *json_obj = obj.GetJsonObject();

  ASSERT_EQ(json_object_get_number(json_obj, "number_of_iterations"), 5);
  json_value_free(value1);
}
