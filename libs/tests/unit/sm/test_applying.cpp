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

class ApplyingTest : public CommonTest {
 public:
  void SetUp() override {
    state = StateFactory::Create(STATE_APPLYING);
    CommonTest::SetUp();
  }
};

TEST_F(ApplyingTest, IterateError) {
  IterateStatus res = state->Iterate();
  ASSERT_EQ(res, IterateStatus::Error);
}

TEST_F(ApplyingTest, CallOnCreateOnce) {
  resetOnCreate();
  resetEdgeAppLibSensorCoreOpenStreamCalled();
  resetEdgeAppLibSensorCoreInitCalled();
  state->Iterate();
  state->Iterate();
  state->Iterate();
  state->Iterate();
  ASSERT_EQ(wasOnCreateCalled(), 1);
  ASSERT_EQ(wasEdgeAppLibSensorCoreOpenStreamCalled(), 1);
  ASSERT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 1);
}

TEST_F(ApplyingTest, MultipleCalling) {
  resetOnCreate();
  resetEdgeAppLibSensorCoreOpenStreamCalled();
  resetEdgeAppLibSensorCoreInitCalled();
  resetOnConfigure();
  char *config = nullptr;
  asprintf(&config, "{\"custom_settings\": {\"process_state\": %d}}",
           STATE_RUNNING);
  // idle as next state
  context->SetNextState(STATE_IDLE);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);

  // set pending configuration
  context->SetPendingConfiguration(config, strlen(config));
  // apply configuration
  state->Iterate();

  char *config2 = nullptr;
  asprintf(&config2, "{\"custom_settings\": {\"test\": %d}}", 5);
  // idle as next state
  context->SetNextState(STATE_IDLE);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
  // set pending configuration
  context->SetPendingConfiguration(config2, strlen(config2));
  state->Iterate();

  ASSERT_EQ(wasOnConfigureCalled(), 2); /* configuration was called 2 times */
  ASSERT_EQ(wasOnCreateCalled(), 1);    /* should be called only once */
  ASSERT_EQ(wasEdgeAppLibSensorCoreOpenStreamCalled(),
            1); /* should be called only once */
  ASSERT_EQ(wasEdgeAppLibSensorCoreInitCalled(),
            1); /* should be called only once */
  ASSERT_NE(wasEdgeAppLibSensorStreamGetPropertyCalled(),
            0); /* should be called only once */
  resetOnCreate();
  resetOnConfigure();
  resetEdgeAppLibSensorCoreOpenStreamCalled();
  resetEdgeAppLibSensorCoreInitCalled();
}

TEST_F(ApplyingTest, ErrorHandlingOnCreate) {
  setOnCreateError();
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(STATE_IDLE, context->GetNextState());
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ(ON_CREATE " call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
}

TEST_F(ApplyingTest, ErrorHandlingEdgeAppLibSensorCoreOpenStream) {
  setEdgeAppLibSensorCoreOpenStreamFail();
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(STATE_DESTROYING, context->GetNextState());
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ(SENSOR_CORE_OPEN_STREAM " call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  resetEdgeAppLibSensorCoreOpenStreamSuccess();
}

TEST_F(ApplyingTest, ErrorHandlingEdgeAppLibSensorCoreInit) {
  setEdgeAppLibSensorCoreInitFail();
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(STATE_DESTROYING, context->GetNextState());
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ(SENSOR_CORE_INIT " call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  resetEdgeAppLibSensorCoreInitSuccess();
}

TEST_F(ApplyingTest, Iterate) {
  char *config = nullptr;
  asprintf(&config, "{\"common_settings\": {\"process_state\": %d}}",
           STATE_RUNNING);
  // idle as next state
  context->SetNextState(STATE_IDLE);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);

  // set pending configuration
  context->SetPendingConfiguration(config, strlen(config));
  // apply configuration
  IterateStatus res = state->Iterate();
  ASSERT_EQ(res, IterateStatus::Ok);

  // pending configuration has been consumed
  ASSERT_EQ(context->GetPendingConfiguration((void **)&config), 0);
  ASSERT_EQ(config, nullptr);

  // running as next state
  ASSERT_EQ(context->GetNextState(), STATE_RUNNING);
}
