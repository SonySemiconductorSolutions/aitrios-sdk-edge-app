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
#include "states/destroying.hpp"
#include "states/state_factory.hpp"

class DestroyTest : public CommonTest {
 public:
  void SetUp() override {
    state = StateFactory::Create(STATE_DESTROYING);
    CommonTest::SetUp();
  }
};

class MockDestroying : public Destroying {
 public:
  MOCK_METHOD(void, StateHandleError, (const char *event, int res), (override));
};

TEST_F(DestroyTest, IterateCurrentStateIsDestroy) {
  IterateStatus result = state->Iterate();
  context = StateMachineContext::GetInstance(nullptr);
  ASSERT_EQ(context->GetCurrentState()->GetEnum(), STATE_DESTROYING);
  ASSERT_EQ(result, IterateStatus::Ok);
  ASSERT_EQ(wasOnDestroyCalled(), 1);
  ASSERT_EQ(wasEdgeAppLibDataExportUnInitializeCalled(), 1);
}

TEST_F(DestroyTest, OnDestroyWithNullStream) {
  context = StateMachineContext::GetInstance(nullptr);
  context->SetSensorCore(1);
  context->SetSensorStream(0);
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Ok);
}

TEST_F(DestroyTest, OnDestroyWithNullCore) {
  context = StateMachineContext::GetInstance(nullptr);
  context->SetSensorCore(0);
  context->SetSensorStream(1);
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Ok);
}

TEST_F(DestroyTest, ErrorHandlingOnDestroy) {
  setOnDestroyError();
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ("onDestroy call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
}

TEST_F(DestroyTest, ErrorHandlingEdgeAppLibSensorCoreCloseStream) {
  setEdgeAppLibSensorCoreCloseStreamFail();
  context->SetSensorCore(1);
  context->SetSensorStream(1);
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ(SENSOR_CORE_CLOSE_STREAM " call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  resetEdgeAppLibSensorCoreCloseStreamSuccess();
}

/* Deployed EdgeApp, but not opened SensorCore and SensorStream */
TEST_F(DestroyTest, NotOpenedEdgeAppLibSensorCoreCloseStream) {
  setEdgeAppLibSensorCoreCloseStreamFail();
  IterateStatus result = state->Iterate();
  /* CoreCloseStream won't be called */
  ASSERT_EQ(result, IterateStatus::Ok);
  resetEdgeAppLibSensorCoreCloseStreamSuccess();
}

TEST_F(DestroyTest, ErrorHandlingEdgeAppLibSensorCoreExit) {
  setEdgeAppLibSensorCoreExitFail();
  context->SetSensorCore(1);
  context->SetSensorStream(1);
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ(SENSOR_CORE_EXIT " call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  resetEdgeAppLibSensorCoreExitSuccess();
}

/* Deployed EdgeApp, but not core was not created */
TEST_F(DestroyTest, NotCreatedEdgeAppLibSensorCoreExit) {
  setEdgeAppLibSensorCoreExitFail();
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Ok);
  resetEdgeAppLibSensorCoreExitSuccess();
}

TEST_F(DestroyTest, ErrorHandlingEdgeAppLibDataExportInitialize) {
  setEdgeAppLibDataExportUnInitializeError();
  IterateStatus result = state->Iterate();
  ASSERT_EQ(result, IterateStatus::Error);
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ("EdgeAppLibDataExportUnInitialize call gave error res=1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  resetEdgeAppLibDataExportUnInitialize();
}
