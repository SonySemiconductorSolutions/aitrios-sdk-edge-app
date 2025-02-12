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

#include "event_functions/mock_sm.hpp"
#include "evp/mock_evp.hpp"
#include "fixtures/state_fixture.cpp"
#include "sm_context.hpp"
#include "states/idle.hpp"
#include "states/state_factory.hpp"

class IdleTest : public StateTest {
 public:
  void SetUp() override {
    state = StateFactory::Create(STATE_IDLE);
    StateTest::SetUp();
  }
};

class MockIdle : public Idle {
 public:
  MOCK_METHOD(void, StateHandleError, (const char *event, int res), (override));
};

TEST_F(IdleTest, IterateCurrentStateIsIdle) {
  IterateStatus result = state->Iterate();
  ASSERT_EQ(context->GetCurrentState()->GetEnum(), STATE_IDLE);
  ASSERT_EQ(result, IterateStatus::Ok);
}

TEST_F(IdleTest, TestExitAtEvpShouldExit) {
  setProcessEventResult(EVP_SHOULDEXIT);
  IterateStatus result = state->Iterate();
  ASSERT_EQ(wasProcessEventCalled(), 1);
  ASSERT_EQ(context->GetCurrentState()->GetEnum(), STATE_IDLE);
  ASSERT_EQ(result, IterateStatus::Ok);
}
