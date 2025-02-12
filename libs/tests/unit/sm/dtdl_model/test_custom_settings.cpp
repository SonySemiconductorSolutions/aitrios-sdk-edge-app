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
#include "event_functions/mock_sm.hpp"
#include "log.h"
#include "parson.h"
#include "sm_context.hpp"
#include "states/idle.hpp"
#include "states/running.hpp"

#define TEST_INPUT "{\"mynn\": {}}"
#define UUID "9438c35a-d7de-11ee-a506-0242ac120002"

class CustomSettingsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    context = StateMachineContext::GetInstance(nullptr);
    json_value = json_parse_string(TEST_INPUT);
    json_obj = json_object(json_value);
  }

  void TearDown() override {
    json_value_free(json_value);
    context->Delete();
  }

  State *state;
  StateMachineContext *context;
  JSON_Value *json_value;
  JSON_Object *json_obj;
};

TEST_F(CustomSettingsTest, Parse) {
  JSON_Value *value2 = json_parse_string("{\"req_id\": \"" UUID "\"}");
  JSON_Object *obj2 = json_object(value2);
  ReqInfo *req_info = context->GetDtdlModel()->GetReqInfo();
  req_info->Apply(obj2);
  json_value_free(value2);

  context->SetCurrentState(StateFactory::Create(STATE_IDLE));
  CustomSettings *custom_settings =
      context->GetDtdlModel()->GetCustomSettings();
  ASSERT_EQ(custom_settings->Verify(json_obj), 0);
  custom_settings->Apply(json_obj);
  ASSERT_TRUE(wasOnConfigureCalled());

  json_object_dotset_number(json_obj, "res_info.code", 0);
  json_object_dotset_string(json_obj, "res_info.res_id", UUID);
  json_object_dotset_string(json_obj, "res_info.detail_msg", "");
  char *str =
      json_serialize_to_string(json_object_get_wrapping_value(json_obj));
  ASSERT_STREQ((char *)OnConfigureInput(), str);

  // verify not called if input the same
  json_object_remove(json_obj, "res_info");
  resetOnConfigure();
  custom_settings->Apply(json_obj);
  ASSERT_FALSE(wasOnConfigureCalled());

  json_free_serialized_string(str);
  resetOnConfigure();
}

TEST_F(CustomSettingsTest, CheckOnConfigureCalledRunningState) {
  CustomSettings *custom_settings =
      context->GetDtdlModel()->GetCustomSettings();
  context->SetCurrentState(StateFactory::Create(STATE_RUNNING));
  JSON_Value *pre_set_value = json_object_get_wrapping_value(json_obj);
  custom_settings->Apply(json_obj);
  ASSERT_TRUE(wasOnConfigureCalled());
  JSON_Value *post_set_value = json_object_get_wrapping_value(json_obj);

  // check that json is not being modified
  ASSERT_TRUE(json_value_equals(pre_set_value, post_set_value));
  DtdlModel *dtdl = context->GetDtdlModel();
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_OK);
  resetOnConfigure();
}

TEST_F(CustomSettingsTest, CheckOnConfigureError) {
  CustomSettings *custom_settings =
      context->GetDtdlModel()->GetCustomSettings();
  context->SetCurrentState(StateFactory::Create(STATE_RUNNING));
  JSON_Value *pre_set_value = json_object_get_wrapping_value(json_obj);
  setOnConfigureError();
  custom_settings->Apply(json_obj);
  ASSERT_TRUE(wasOnConfigureCalled());
  JSON_Value *post_set_value = json_object_get_wrapping_value(json_obj);

  DtdlModel *dtdl = context->GetDtdlModel();
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_FAILED_PRECONDITION);
  ASSERT_STREQ("onConfigure call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);

  resetOnConfigure();
}
