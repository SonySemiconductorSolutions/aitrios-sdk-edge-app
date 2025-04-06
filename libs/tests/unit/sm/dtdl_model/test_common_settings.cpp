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
#include "log.h"
#include "log_internal.h"
#include "macros.h"
#include "parson.h"
#include "sm_context.hpp"
#include "states/state.hpp"

// empty objects for each key apart from process_state
#define SUBJSON                                                              \
  "\"inference_settings\": {}, \"pq_settings\": {}, \"port_settings\": {}, " \
  "\"codec_settings\": {}"

#define LOGLEVEL " \"log_level\": 1, \"number_of_inference_per_message\": 1, "

#define TEST_INPUT_ERROR "{\"process_state\": 13," LOGLEVEL SUBJSON "}"
#define TEST_INPUT_RUNNING "{\"process_state\": 2," LOGLEVEL SUBJSON "}"
#define TEST_INPUT_IDLE "{\"process_state\": 1," LOGLEVEL SUBJSON "}"

// mocks

class MockPortSettings : public PortSettings {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  MOCK_METHOD(int, Apply, (JSON_Object * obj), (override));
};

class MockPqSettings : public PqSettings {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  MOCK_METHOD(int, Apply, (JSON_Object * obj), (override));
};

class MockInferenceSettings : public InferenceSettings {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockCodecSettings : public CodecSettings {
 public:
  MOCK_METHOD(int, Verify, (JSON_Object * obj), (override));
  MOCK_METHOD(int, Apply, (JSON_Object * obj), (override));
};

class MockCommonSettings : public CommonSettings {
 public:
  MockCommonSettings() {
    static Property s_properties[] = {
        {.property = "pq_settings", .obj = GetPqSettings()},
        {.property = "port_settings", .obj = GetPortSettings()},
        {.property = "inference_settings", .obj = GetInferenceSettings()},
        {.property = "codec_settings", .obj = GetCodecSettings()}};
    SetProperties(s_properties, sizeof(s_properties) / sizeof(Property));
  }

  PortSettings *GetPortSettings() override { return &port_settings; }
  PqSettings *GetPqSettings() override { return &pq_settings; }
  InferenceSettings *GetInferenceSettings() override {
    return &inference_settings;
  }
  CodecSettings *GetCodecSettings() override { return &codec_settings; }

 private:
  MockPqSettings pq_settings;
  MockPortSettings port_settings;
  MockInferenceSettings inference_settings;
  MockCodecSettings codec_settings;
};

class MockMethodsCommonSettings : public MockCommonSettings {
 public:
  MOCK_METHOD(int, SetInferencePerMessage, (uint32_t value), (override));
};

// fixtures

class CommonSettingsRunningToIdle : public ::testing::Test {
 protected:
  void SetUp() override {
    context =
        StateMachineContext::GetInstance(StateFactory::Create(STATE_RUNNING));
    json_obj = json_value_get_object(json_parse_string(TEST_INPUT_IDLE));
  }

  void TearDown() override {
    json_value_free(json_object_get_wrapping_value(json_obj));
    context->Delete();
  }

  StateMachineContext *context;
  JSON_Object *json_obj;
};

class CommonSettingsRunningToRunning : public ::testing::Test {
 protected:
  void SetUp() override {
    context =
        StateMachineContext::GetInstance(StateFactory::Create(STATE_RUNNING));
    json_obj = json_value_get_object(json_parse_string(TEST_INPUT_RUNNING));
  }

  void TearDown() override {
    json_value_free(json_object_get_wrapping_value(json_obj));
    context->Delete();
  }

  StateMachineContext *context;
  JSON_Object *json_obj;
};

class CommonSettingsIdleToRunning : public ::testing::Test {
 protected:
  void SetUp() override {
    context =
        StateMachineContext::GetInstance(StateFactory::Create(STATE_IDLE));
    json_obj = json_value_get_object(json_parse_string(TEST_INPUT_RUNNING));
  }

  void TearDown() override {
    json_value_free(json_object_get_wrapping_value(json_obj));
    context->Delete();
  }

  StateMachineContext *context;
  JSON_Object *json_obj;
};

class CommonSettingsRunning : public ::testing::Test {
 protected:
  void SetUp() override {
    context =
        StateMachineContext::GetInstance(StateFactory::Create(STATE_RUNNING));
  }
  void TearDown() override { context->Delete(); }
  StateMachineContext *context;
};

class CommonSettingsIdle : public ::testing::Test {
 protected:
  void SetUp() override {
    context =
        StateMachineContext::GetInstance(StateFactory::Create(STATE_IDLE));
  }
  void TearDown() override { context->Delete(); }
  StateMachineContext *context;
};

// tests

TEST_F(CommonSettingsRunningToIdle, Parse) {
  CommonSettings common_settings;
  ASSERT_EQ(common_settings.Verify(json_obj), 0);
  common_settings.Apply(json_obj);
  ASSERT_EQ(common_settings.GetProcessState(), 1);
}

TEST_F(CommonSettingsIdle, JsonEmpty) {
  CommonSettings common_settings;
  JSON_Value *value = json_parse_string("{}");
  common_settings.Apply(json_object(value));
  json_value_free(value);
}

TEST_F(CommonSettingsIdle, DefaultValue) {
  CommonSettings common_settings;
  uint32_t numOfInf = common_settings.getNumOfInfPerMsg();
  ASSERT_EQ(numOfInf, 1);
}

TEST_F(CommonSettingsRunningToIdle, StateUpdateRunning) {
  MockCommonSettings common_settings;
  ASSERT_EQ(context->GetNextState(), STATE_RUNNING);
  common_settings.Apply(json_obj);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
}

TEST_F(CommonSettingsIdleToRunning, StateUpdateToRunning) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  MockCommonSettings common_settings;
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
  common_settings.Apply(json_obj);
  ASSERT_EQ(context->GetNextState(), STATE_RUNNING);
}

TEST_F(CommonSettingsIdleToRunning, StateUpdateNotification) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  MockCommonSettings common_settings;
  common_settings.Apply(json_obj);
  context->ClearNotification();
  common_settings.Apply(json_obj);
  ASSERT_FALSE(context->IsPendingNotification());
  for (int i = STATE_IDLE; i <= STATE_RUNNING; ++i) {
    json_object_set_number(json_obj, "process_state", i);
    common_settings.Apply(json_obj);
    ASSERT_TRUE(context->IsPendingNotification());
    context->ClearNotification();
  }
}

TEST_F(CommonSettingsRunning, InvalidProcessState) {
  JSON_Value *json_value = json_parse_string(TEST_INPUT_ERROR);
  JSON_Object *json_obj = json_object(json_value);
  CommonSettings common_settings;
  ASSERT_EQ(common_settings.Verify(json_obj), -1);
}

TEST_F(CommonSettingsRunningToRunning, RevokeSettingsInRunning) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  MockCommonSettings common_settings;

  MockPqSettings *pq = (MockPqSettings *)common_settings.GetPqSettings();
  EXPECT_CALL(*pq, Apply(json_object_get_object(json_obj, "pq_settings")))
      .Times(0);
  MockPortSettings *ps = (MockPortSettings *)common_settings.GetPortSettings();
  EXPECT_CALL(*ps, Apply(json_object_get_object(json_obj, "port_settings")))
      .Times(0);
  MockCodecSettings *cs =
      (MockCodecSettings *)common_settings.GetCodecSettings();
  EXPECT_CALL(*ps, Apply(json_object_get_object(json_obj, "codec_settings")))
      .Times(0);
  common_settings.Apply(json_obj);
  DtdlModel *dtdl = context->GetDtdlModel();
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), (uint32_t)CODE_FAILED_PRECONDITION);
  ASSERT_STREQ(
      dtdl->GetResInfo()->GetDetailMsg(),
      "Ignoring Port Settings and Pq Settings since state is Running.");
}

TEST_F(CommonSettingsRunningToRunning, RevokeSettingsInRunningWithoutChange) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  MockCommonSettings common_settings;

  MockPqSettings *pq = (MockPqSettings *)common_settings.GetPqSettings();
  EXPECT_CALL(*pq, Apply(json_object_get_object(json_obj, "pq_settings")))
      .Times(0);
  MockPortSettings *ps = (MockPortSettings *)common_settings.GetPortSettings();
  EXPECT_CALL(*ps, Apply(json_object_get_object(json_obj, "port_settings")))
      .Times(0);
  MockCodecSettings *cs =
      (MockCodecSettings *)common_settings.GetCodecSettings();
  EXPECT_CALL(*cs, Apply(cs->GetJsonObject())).Times(0);
  json_object_set_value(json_obj, "pq_settings",
                        json_object_get_wrapping_value(pq->GetJsonObject()));
  json_object_set_value(json_obj, "port_settings",
                        json_object_get_wrapping_value(ps->GetJsonObject()));
  json_object_set_value(json_obj, "codec_settings",
                        json_object_get_wrapping_value(cs->GetJsonObject()));
  json_object_set_number(
      json_obj, "number_of_inference_per_message",
      (int)json_object_get_number(common_settings.GetJsonObject(),
                                  "number_of_inference_per_message"));
  common_settings.Apply(json_obj);
  DtdlModel *dtdl = context->GetDtdlModel();
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), (uint32_t)CODE_OK);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(), "");
}

TEST_F(CommonSettingsIdleToRunning, ApplySettingsInIdle) {
  StateMachineContext *context =
      StateMachineContext::GetInstance(StateFactory::Create(STATE_IDLE));
  MockCommonSettings common_settings;

  MockPqSettings *pq = (MockPqSettings *)common_settings.GetPqSettings();
  EXPECT_CALL(*pq, Apply(json_object_get_object(json_obj, "pq_settings")))
      .Times(1);
  MockPortSettings *ps = (MockPortSettings *)common_settings.GetPortSettings();
  EXPECT_CALL(*ps, Apply(json_object_get_object(json_obj, "port_settings")))
      .Times(1);
  common_settings.Apply(json_obj);
}

TEST_F(CommonSettingsIdleToRunning, ApplyLogLevel) {
  assert(kCriticalLevel == 0);
  assert(kTraceLevel == 5);
  MockCommonSettings common_settings;
  for (int i = kTraceLevel; i <= kCriticalLevel; ++i) {
    json_object_set_number(json_obj, "log_level", i);
    common_settings.Apply(json_obj);
    ASSERT_EQ(common_settings.GetLoggingLevel(common_settings.json_obj), i);
    ASSERT_EQ(GetLogLevel(), i);
  }
}

TEST_F(CommonSettingsIdleToRunning, CodecSettingsCalled) {
  MockCommonSettings common_settings;
  MockCodecSettings *codec =
      (MockCodecSettings *)common_settings.GetCodecSettings();
  EXPECT_CALL(*codec,
              Verify(json_object_get_object(json_obj, "codec_settings")))
      .Times(1);
  EXPECT_CALL(*codec, Apply(json_object_get_object(json_obj, "codec_settings")))
      .Times(1);
  common_settings.Verify(json_obj);
  common_settings.Apply(json_obj);
}

TEST_F(CommonSettingsRunningToRunning, CodecSettingsNotCalled) {
  MockCommonSettings common_settings;
  MockCodecSettings *codec =
      (MockCodecSettings *)common_settings.GetCodecSettings();
  EXPECT_CALL(*codec,
              Verify(json_object_get_object(json_obj, "codec_settings")))
      .Times(1);
  EXPECT_CALL(*codec, Apply(json_object_get_object(json_obj, "codec_settings")))
      .Times(0);
  common_settings.Verify(json_obj);
  common_settings.Apply(json_obj);
}

TEST_F(CommonSettingsIdleToRunning, InferencePerMessageCalled) {
  MockMethodsCommonSettings common_settings;
  int inference_per_msg = 3;
  json_object_set_number(json_obj, "number_of_inference_per_message",
                         inference_per_msg);
  EXPECT_CALL(common_settings, SetInferencePerMessage(inference_per_msg));
  common_settings.Apply(json_obj);
}

TEST_F(CommonSettingsRunningToRunning, InferencePerMessageNotCalled) {
  MockMethodsCommonSettings common_settings;
  int inference_per_msg = 3;
  json_object_set_number(json_obj, "number_of_inference_per_message",
                         inference_per_msg);
  EXPECT_CALL(common_settings, SetInferencePerMessage(inference_per_msg))
      .Times(0);
  common_settings.Apply(json_obj);
}

TEST_F(CommonSettingsIdleToRunning, InferencePerMessage) {
  MockCommonSettings common_settings;
  int inference_per_msg = 3;
  json_object_set_number(json_obj, "number_of_inference_per_message",
                         inference_per_msg);
  common_settings.Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  common_settings.Delete();
}
