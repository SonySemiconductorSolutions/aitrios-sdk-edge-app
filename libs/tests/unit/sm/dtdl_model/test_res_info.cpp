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

#include "dtdl_model/objects/res_info.hpp"
#include "parson.h"
#include "sm_context.hpp"

#define TEST_INPUT \
  "{\"res_id\": \"aabbb\", \"code\": 0, \"detail_msg\": \"my message\"}"
#define NEW_DETAIL_MSG "my new message"

class ResInfoParam : public ::testing::Test {
 public:
  void SetUp() override {
    json_value = json_parse_string(TEST_INPUT);
    json_obj = json_object(json_value);
    context = StateMachineContext::GetInstance(nullptr);
  }

  void TearDown() override {
    json_value_free(json_value);
    context->Delete();
    res_info.Delete();
  }
  ResInfo res_info;
  StateMachineContext *context;
  JSON_Value *json_value;
  JSON_Object *json_obj;
};

TEST_F(ResInfoParam, Parse) {
  ASSERT_EQ(res_info.Verify(json_obj), 0);
  res_info.Apply(json_obj);
  ASSERT_STREQ(res_info.GetResId(), "");
  ASSERT_EQ(res_info.GetCode(), 0);
  ASSERT_STREQ(res_info.GetDetailMsg(), "");
}

TEST_F(ResInfoParam, SetGetDetailMessage) {
  res_info.SetDetailMsg(NEW_DETAIL_MSG);
  ASSERT_STREQ(res_info.GetDetailMsg(), NEW_DETAIL_MSG);
  ASSERT_TRUE(context->IsPendingNotification());
}

TEST_F(ResInfoParam, SetGetCode) {
  res_info.SetCode(1);
  ASSERT_EQ(res_info.GetCode(), 1);
  ASSERT_TRUE(context->IsPendingNotification());
}

TEST_F(ResInfoParam, ResInfoParam_SetGetResId_Test) {
  res_info.SetResId("abc");
  ASSERT_STREQ(res_info.GetResId(), "abc");
  ASSERT_TRUE(context->IsPendingNotification());
}
