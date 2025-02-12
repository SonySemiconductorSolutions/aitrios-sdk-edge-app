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
#include "dtdl_model/properties.h"
#include "event_functions/mock_sm.hpp"
#include "evp/mock_evp.hpp"
#include "fixtures/state_fixture.cpp"
#include "sensor/mock_sensor.hpp"
#include "sm_context.hpp"
#include "states/creating.hpp"
#include "states/state_defs.h"
#include "states/state_factory.hpp"

class CreateTest : public CommonTest {
 public:
  void SetUp() override {
    state = StateFactory::Create(STATE_CREATING);
    CommonTest::SetUp();
  }
};

class MockCreating : public Creating {
 public:
  MOCK_METHOD(void, StateHandleError, (const char *event, int res), (override));
};

TEST_F(CreateTest, CreateCallsEVPSetConfigurationCallback) {
  state->Iterate();
  ASSERT_EQ(wasSetConfigurationCallbackCalled(), 1);
}

TEST_F(CreateTest, CreateNotCallonCreate) {
  state->Iterate();
  ASSERT_EQ(wasOnCreateCalled(), 0);  // onCreate
  ASSERT_EQ(wasEdgeAppLibSensorCoreOpenStreamCalled(), 0);
  ASSERT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 0);
  ASSERT_EQ(wasEdgeAppLibDataExportInitializeCalled(), 1);
  ASSERT_EQ(wasEdgeAppLibSensorStreamGetPropertyCalled(), 0);
}

TEST_F(CreateTest, CreateNotCallsOnStart) {
  state->Iterate();
  ASSERT_EQ(wasOnStartCalled(), 0);
  ASSERT_EQ(wasEdgeAppLibSensorCoreOpenStreamCalled(), 0);
  ASSERT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 0);
}

TEST_F(CreateTest, IterateCurrentStateIsCreate) {
  IterateStatus result = state->Iterate();
  ASSERT_EQ(context->GetCurrentState()->GetEnum(), STATE_CREATING);
  ASSERT_EQ(result, IterateStatus::Ok);
}

TEST_F(CreateTest, ErrorHandlingEdgeAppLibDataExportInitialize) {
  setEdgeAppLibDataExportInitializeError();
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(STATE_DESTROYING, context->GetNextState());
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ(AITRIOS_DATA_EXPORT_INITIALIZE " call gave error res=1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  resetEdgeAppLibDataExportInitialize();
}

TEST_F(CreateTest, NotificationEnabled) {
  ASSERT_FALSE(context->IsPendingNotification());
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Ok);
  ASSERT_TRUE(context->IsPendingNotification());
}
