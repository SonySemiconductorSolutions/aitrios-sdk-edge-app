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

#include "data_export/mock_data_export.hpp"
#include "event_functions/mock_sm.hpp"
#include "evp/mock_evp.hpp"
#include "fixtures/state_fixture.hpp"
#include "sm_context.hpp"
#include "states/coolingdown.hpp"
#include "states/state_factory.hpp"

#define TEST_INPUT                                                      \
  "{\"method\": 1, \"storage_name\": \"mystoragename\", \"endpoint\": " \
  "\"myendpoint\", \"path\": \"mypath\", "                              \
  "\"enabled\": true}"

class CoolingDownTest : public CommonTest {
 public:
  void SetUp() override {
    state = StateFactory::Create(STATE_COOLINGDOWN);
    CommonTest::SetUp();
  }
};

class MockCoolingDown : public CoolingDown {
 public:
  MOCK_METHOD(void, StateHandleError, (const char *event, int res), (override));
};

TEST_F(CoolingDownTest, IterateCurrentStateIsCoolingDown) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);
  context->GetDtdlModel()
      ->GetCommonSettings()
      ->GetPortSettings()
      ->GetMetadata()
      ->Apply(obj1);
  context->GetDtdlModel()
      ->GetCommonSettings()
      ->GetPortSettings()
      ->GetInputTensor()
      ->Apply(obj1);

  IterateStatus result = state->Iterate();
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
  ASSERT_EQ(context->GetCurrentState()->GetEnum(), STATE_COOLINGDOWN);
  ASSERT_EQ(result, IterateStatus::Ok);
  json_value_free(value1);
}

TEST_F(CoolingDownTest, CoolingDownRefuse) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);
  context->SetNextState(STATE_DESTROYING);

  IterateStatus result = state->Iterate();
  ASSERT_EQ(context->GetNextState(), STATE_DESTROYING);
  ASSERT_EQ(context->GetCurrentState()->GetEnum(), STATE_COOLINGDOWN);
  ASSERT_EQ(result, IterateStatus::Break);
  json_value_free(value1);
}
