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

#include "sm_context.hpp"
#include "states/state.hpp"

class TestState : public State {
 public:
  TestState(StateMachineContext *sm_context);
  IterateStatus Iterate();
  STATE GetEnum() { return state; }
  void SetEnum(STATE state) { state; }

  STATE state = STATE_IDLE;
};

TestState::TestState(StateMachineContext *sm_context) { context = sm_context; }

IterateStatus TestState::Iterate() { return IterateStatus::Ok; }

class StateMachineContextTest : public ::testing::Test {
 public:
  void SetUp() {
    sm_context = new StateMachineContext();
    test_state = new TestState(sm_context);
  };
  void TearDown() { sm_context->Delete(); };

  StateMachineContext *sm_context;
  TestState *test_state;
};

TEST_F(StateMachineContextTest, SetCurrentStateIdleToIdle) {
  EXPECT_EQ(sm_context->GetCurrentState(), nullptr);
  sm_context->SetCurrentState(test_state);
  EXPECT_NE(sm_context->GetCurrentState(), nullptr);
  EXPECT_EQ(sm_context->GetNextState(), test_state->GetEnum());
  EXPECT_EQ(sm_context->GetDtdlModel()->GetCommonSettings()->GetProcessState(),
            test_state->GetEnum());
}

TEST_F(StateMachineContextTest, SetCurrentStateIdleToRunning) {
  EXPECT_EQ(sm_context->GetCurrentState(), nullptr);
  test_state->SetEnum(STATE_RUNNING);
  sm_context->SetCurrentState(test_state);
  EXPECT_NE(sm_context->GetCurrentState(), nullptr);
  EXPECT_EQ(sm_context->GetNextState(), test_state->GetEnum());
  EXPECT_EQ(sm_context->GetDtdlModel()->GetCommonSettings()->GetProcessState(),
            test_state->GetEnum());
}

TEST_F(StateMachineContextTest, SetCurrentStateIdleToDestroy) {
  EXPECT_EQ(sm_context->GetCurrentState(), nullptr);
  sm_context->SetNextState(STATE_DESTROYING);
  sm_context->SetCurrentState(test_state);
  EXPECT_NE(sm_context->GetCurrentState(), nullptr);
  EXPECT_EQ(sm_context->GetDtdlModel()->GetCommonSettings()->GetProcessState(),
            STATE_IDLE);
}

TEST_F(StateMachineContextTest, SetCurrentStateNull) {
  EXPECT_EQ(sm_context->GetCurrentState(), nullptr);
  sm_context->SetCurrentState(nullptr);
  EXPECT_EQ(sm_context->GetNextState(), STATE_EXITING);
  EXPECT_EQ(sm_context->GetDtdlModel()->GetCommonSettings()->GetProcessState(),
            STATE_IDLE);
}
