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
#include "sm_context.hpp"

#define CONFIG(method, storage_name, endpoint, path, enabled)   \
  "{\"method\": " #method ", \"storage_name\": \"" storage_name \
  "\", \"endpoint\": \"" endpoint                               \
  "\", "                                                        \
  "\"path\": \"" path "\", \"enabled\": " #enabled "}"

#define CONFIG_1 CONFIG(2, "mystoragename", "myendpoint", "mypath", true)
#define CONFIG_2 CONFIG(2, "mystoragename2", "myendpoint2", "mypath2", false)

TEST(PortSetting, Verify) {
  PortSetting ps(PS_INFERENCE);

  JSON_Value *value = json_parse_string(CONFIG_1);
  ASSERT_EQ(ps.Verify(json_object(value)), 0);
  ps.Apply(json_object(value));
  ps.Delete();
  json_value_free(value);
}

TEST(PortSetting, VerifyFailed) {
  PortSetting ps(PS_INFERENCE);
  JSON_Value *value;

  value = json_parse_string("{\"method\": false}");
  ASSERT_EQ(ps.Verify(json_object(value)), -1);
  json_value_free(value);
  value = json_parse_string("{\"method\": 3}");
  ASSERT_EQ(ps.Verify(json_object(value)), 0);
  json_value_free(value);

  value = json_parse_string("{\"storage_name\": false}");
  ASSERT_EQ(ps.Verify(json_object(value)), -1);
  json_value_free(value);
  value = json_parse_string("{\"storage_name\": \"mystring\"}");
  ASSERT_EQ(ps.Verify(json_object(value)), 0);
  json_value_free(value);

  value = json_parse_string("{\"endpoint\": false}");
  ASSERT_EQ(ps.Verify(json_object(value)), -1);
  json_value_free(value);
  value = json_parse_string("{\"endpoint\": \"mystring\"}");
  ASSERT_EQ(ps.Verify(json_object(value)), 0);
  json_value_free(value);

  value = json_parse_string("{\"path\": false}");
  ASSERT_EQ(ps.Verify(json_object(value)), -1);
  json_value_free(value);
  value = json_parse_string("{\"path\": \"mystring\"}");
  ASSERT_EQ(ps.Verify(json_object(value)), 0);
  json_value_free(value);

  value = json_parse_string("{\"enabled\": 3}");
  ASSERT_EQ(ps.Verify(json_object(value)), -1);
  json_value_free(value);
  value = json_parse_string("{\"enabled\": false}");
  ASSERT_EQ(ps.Verify(json_object(value)), 0);
  json_value_free(value);

  ps.Delete();
}

TEST(PortSetting, Apply) {
  PortSetting ps(PS_INFERENCE);
  JSON_Value *value = json_parse_string(CONFIG_1);
  JSON_Object *object = json_object(value);

  ps.Apply(json_object(value));

  ASSERT_EQ(ps.GetMethod(), (uint32_t)json_object_get_number(object, "method"));
  ASSERT_STREQ(ps.GetStorageName(),
               json_object_get_string(object, "storage_name"));
  ASSERT_STREQ(ps.GetEndpoint(), json_object_get_string(object, "endpoint"));
  ASSERT_STREQ(ps.GetPath(), json_object_get_string(object, "path"));
  ASSERT_EQ(ps.GetEnabled(), json_object_get_boolean(object, "enabled"));

  json_value_free(value);
  ps.Delete();
}

TEST(PortSetting, EmptyJson) {
  PortSetting obj(PS_INFERENCE);
  JSON_Value *value = json_parse_string("{}");
  obj.Apply(json_object(value));
  json_value_free(value);
}
