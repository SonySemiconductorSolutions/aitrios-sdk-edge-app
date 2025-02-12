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
#include "parson.h"
#include "sensor.h"
#include "sm_context.hpp"

#define TEST_INPUT_JPG "{\"format\": 1}"
#define TEST_INPUT_BMP "{\"format\": 0}"

TEST(CodecSettings, ParseBmp) {
  CodecSettings obj;

  JSON_Value *value = json_parse_string(TEST_INPUT_BMP);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Apply(json_object(value));
  obj.Delete();
  json_value_free(value);
}

TEST(CodecSettings, ParseJpg) {
  CodecSettings obj;

  JSON_Value *value = json_parse_string(TEST_INPUT_JPG);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Apply(json_object(value));
  obj.Delete();
  json_value_free(value);
}

TEST(CodecSettings, EmptyJson) {
  CodecSettings obj;
  JSON_Value *value = json_parse_string("{}");
  obj.Apply(json_object(value));
  json_value_free(value);
}
