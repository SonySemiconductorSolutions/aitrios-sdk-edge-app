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

#include "dtdl_model/objects/req_info.hpp"
#include "parson.h"

#define TEST_INPUT "{\"req_id\": \"aaabbb\"}"
#define TEST_INPUT_ERROR "{\"req_id2\": 13}"

class RestInfoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    json_value = json_parse_string(TEST_INPUT);
    json_obj = json_object(json_value);
  }

  void TearDown() override { json_value_free(json_value); }

  JSON_Value *json_value;
  JSON_Object *json_obj;
};

TEST_F(RestInfoTest, Parse) {
  ReqInfo req_info;
  ASSERT_EQ(req_info.Verify(json_obj), 0);
  req_info.Apply(json_obj);
  ASSERT_TRUE(strcmp(req_info.GetReqId(), "aaabbb") == 0);
}

TEST(RestInfoTestError, ParseError) {
  JSON_Value *json_value = json_parse_string(TEST_INPUT_ERROR);
  JSON_Object *json_obj = json_object(json_value);
  ReqInfo req_info;
  ASSERT_EQ(req_info.Verify(json_obj), -1);
  json_value_free(json_value);
}
