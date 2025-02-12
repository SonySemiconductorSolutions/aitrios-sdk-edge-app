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

#include <assert.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dtdl_model/dtdl_model.hpp"
#include "dtdl_model/properties.h"
#include "log.h"
#include "parson.h"
#include "sensor.h"
#include "states/idle.hpp"

#ifndef DTDL_OBJ_PATH
#define DTDL_OBJ_PATH "../dtdl_model/sample_implemented.json"
#endif

#define MAX_JSON_SIZE (16 * 1024)

class MockCommonSettings : public CommonSettings {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  MOCK_METHOD(int, Apply, (JSON_Object * obj), (override));
  // int Apply(JSON_Object *obj) override { return -1; }
};

class MockCustomSettings : public CustomSettings {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  MOCK_METHOD(int, Apply, (JSON_Object * obj), (override));
  // int Apply(JSON_Object *obj) override { return -1; }
};

class MockDtdlModel : public DtdlModel {
 public:
  MockDtdlModel() {
    static Property s_properties[] = {
        {.property = "common_settings", .obj = GetCommonSettings()},
        {.property = "custom_settings", .obj = GetCustomSettings()}};
    SetProperties(s_properties, sizeof(s_properties) / sizeof(Property));
  }

  CommonSettings *GetCommonSettings() override { return &mock_common_settings; }
  CustomSettings *GetCustomSettings() override { return &mock_custom_settings; }

 private:
  MockCommonSettings mock_common_settings;
  MockCustomSettings mock_custom_settings;
};

class DTDLTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StateFactory::Create(STATE_IDLE);
    FILE *file = fopen(DTDL_OBJ_PATH, "r");
    EXPECT_TRUE(file != nullptr);
    char *json_str = (char *)malloc(MAX_JSON_SIZE);
    uint32_t size = fread(json_str, 1, MAX_JSON_SIZE, file);
    fclose(file);
    json_value = json_parse_string(json_str);
    free(json_str);
    json_obj = json_object(json_value);
    context = StateMachineContext::GetInstance(nullptr);
  }

  void TearDown() override {
    json_value_free(json_value);
    context->Delete();
  }

  JSON_Value *json_value;
  JSON_Object *json_obj;
  StateMachineContext *context = nullptr;
};

TEST_F(DTDLTest, InitializeValues) {
  DtdlModel dtdl_model;
  JSON_Object *json_pre = dtdl_model.GetJsonObject();
  char *pre_init_str =
      json_serialize_to_string(json_object_get_wrapping_value(json_pre));
  dtdl_model.InitializeValues();
  JSON_Object *json_post = dtdl_model.GetJsonObject();
  char *post_init_str =
      json_serialize_to_string(json_object_get_wrapping_value(json_post));

  EXPECT_FALSE(strcmp(pre_init_str, post_init_str) == 0);

  free(pre_init_str);
  free(post_init_str);
}

TEST_F(DTDLTest, EmptyJson) {
  DtdlModel obj;
  JSON_Value *value = json_parse_string("{}");
  obj.Apply(json_object(value));
  json_value_free(value);
}

TEST_F(DTDLTest, CommonSettingsAndCustomSettings) {
  // request object does not have res_info
  json_object_dotremove(json_obj, "res_info");

  DtdlModel *dtdl_model = context->GetDtdlModel();
  char *json_str = json_serialize_to_string(json_value);
  EXPECT_TRUE(dtdl_model->Update(json_str) == 0);
  free(json_str);

  char *golden_str =
      json_serialize_to_string_pretty(json_object_get_wrapping_value(
          json_object_get_object(json_obj, "common_settings")));
  char *dtdl_str = dtdl_model->Serialize();

  JSON_Value *value2 = json_parse_string(dtdl_str);
  JSON_Value *value3 = json_object_get_wrapping_value(
      json_object_get_object(json_value_get_object(value2), "common_settings"));
  char *res_info = json_serialize_to_string_pretty(value3);

  EXPECT_STREQ(golden_str, res_info);
  json_value_free(value2);
  free(golden_str);
  free(dtdl_str);
  free(res_info);
}

TEST_F(DTDLTest, CommonSettingsAndCustomSettingsFailed) {
  json_object_dotremove(json_obj, "res_info");
  json_object_dotset_number(
      json_obj, "common_settings.pq_settings.auto_exposure.max_exposure_time",
      -1);

  DtdlModel *dtdl_model = context->GetDtdlModel();
  char *json_str = json_serialize_to_string(json_value);
  EXPECT_TRUE(dtdl_model->Update(json_str) == -1);
  free(json_str);
}

TEST_F(DTDLTest, ReqInfo) {
  DtdlModel dtdl_model;
  for (int i = 0; i < 10; ++i) {
    char uuid[128];
    sprintf(uuid, "%d", i);
    json_object_dotset_string(json_obj, "req_info.req_id", uuid);
    char *json_str = json_serialize_to_string(json_value);
    EXPECT_TRUE(dtdl_model.Update(json_str) == 0);
    free(json_str);

    json_object_dotset_number(json_obj, "res_info.code", 0);
    json_object_dotset_string(json_obj, "res_info.res_id", uuid);
    json_object_dotset_string(json_obj, "res_info.detail_msg", "");

    char *parson_str =
        json_serialize_to_string_pretty(json_object_get_wrapping_value(
            json_object_get_object(json_obj, "res_info")));
    char *dtdl_str = dtdl_model.Serialize();
    JSON_Value *value2 = json_parse_string(dtdl_str);
    JSON_Value *value3 = json_object_get_wrapping_value(
        json_object_get_object(json_value_get_object(value2), "res_info"));
    char *res_info = json_serialize_to_string_pretty(value3);
    json_value_free(value2);

    EXPECT_STREQ(parson_str, res_info);
    free(parson_str);
    free(dtdl_str);
    free(res_info);
  }
}

TEST_F(DTDLTest, ApplyCommonSettingsFailed) {
  MockDtdlModel obj;

  MockCommonSettings *common = (MockCommonSettings *)obj.GetCommonSettings();
  EXPECT_CALL(*common,
              Apply(json_object_get_object(json_obj, "common_settings")))
      .Times(1)
      .WillOnce(::testing::Return(-1));

  MockCustomSettings *custom = (MockCustomSettings *)obj.GetCustomSettings();
  EXPECT_CALL(*common,
              Apply(json_object_get_object(json_obj, "custom_settings")))
      .Times(0);

  int ret = obj.Apply(json_obj);
  ASSERT_EQ(ret, -1);
}

TEST_F(DTDLTest, ApplyCustomSettingsFailed) {
  MockDtdlModel obj;

  MockCommonSettings *common = (MockCommonSettings *)obj.GetCommonSettings();
  EXPECT_CALL(*common,
              Apply(json_object_get_object(json_obj, "common_settings")))
      .Times(1)
      .WillOnce(::testing::Return(0));

  MockCustomSettings *custom = (MockCustomSettings *)obj.GetCustomSettings();
  EXPECT_CALL(*custom,
              Apply(json_object_get_object(json_obj, "custom_settings")))
      .Times(1)
      .WillOnce(::testing::Return(-1));

  int ret = obj.Apply(json_obj);
  ASSERT_EQ(ret, -1);
}
