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
#include "fixtures/state_fixture.cpp"
#include "mock_sensor.hpp"
#include "parson.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "states/state_factory.hpp"

#define AMS_CONFIG_0 \
  "[{\"name\":\"ai_model\",\"target\":\"cpu\",\"url_path\":\"path_string\", \
  \"hash\":\"1234\"}]"
#define AMS_CONFIG_1 \
  "[{\"name\":\"ai_model\",\"target\":\"cpu\",\"url_path\":\"path_str\", \
  \"hash\":\"1234\"},{},{},{},{},{},{},{},{},{},{},{}]"
#define AMS_CONFIG_2 \
  "[{\"name\":\"ai_model\",\"target\":\"cpu\",\"url_path\":\"path_str\", \
  \"hash\":\"1234\"},{}]"

TEST(AiModels, Verify) {
  AiModels ams;

  JSON_Value *value = json_parse_string(AMS_CONFIG_0);
  ASSERT_EQ(ams.Verify(json_array(value)), 0);
  json_value_free(value);

  value = json_parse_string(AMS_CONFIG_1);
  ASSERT_EQ(ams.Verify(json_array(value)), -1);
  json_value_free(value);
}

TEST(AiModels, Apply) {
  AiModels ams;

  JSON_Value *value = json_parse_string(AMS_CONFIG_0);
  ASSERT_EQ(ams.Apply(json_array(value)), 0);
  json_value_free(value);

  value = json_parse_string(AMS_CONFIG_2);
  ASSERT_EQ(ams.Apply(json_array(value)), -1);
  json_value_free(value);
}
