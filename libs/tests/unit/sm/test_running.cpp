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

#include "dtdl_model/properties.h"
#include "event_functions/mock_sm.hpp"
#include "evp/mock_evp.hpp"
#include "fixtures/state_fixture.cpp"
#include "log.h"
#include "sm_context.hpp"
#include "states/idle.hpp"
#include "states/running.hpp"
#include "states/running_thread.hpp"
#include "states/state_factory.hpp"

#define REPEAT_TEST 10

using ::testing::Range;
using ::testing::Return;

class RunningTest : public CommonTest,
                    public ::testing::WithParamInterface<int> {
 public:
  void SetUp() override {
    state = StateFactory::Create(STATE_RUNNING);
    CommonTest::SetUp();
  }
};

class MockRunningThread : public RunningThread {
 public:
  void ThreadStart() { is_thread_start_called = true; };
  void ThreadStop() { is_thread_stop_called = true; }

  bool is_thread_start_called = false;
  bool is_thread_stop_called = false;
};

class MockRunningThreadOrder : public RunningThread {
 public:
  void ThreadStart() { on_start_before_thread_start = wasOnStartCalled(); }
  void ThreadStop() { on_stop_after_thread_stop = !wasOnStopCalled(); }

  bool on_start_before_thread_start = false;
  bool on_stop_after_thread_stop = false;
};

TEST_F(CommonTest, IterateCallsThreadStart) {
  MockRunningThread *mock_running_thread = new MockRunningThread;
  Running *running = new Running((RunningThread *)mock_running_thread);
  context->SetCurrentState(running);
  context->SetCurrentState(nullptr);
  ASSERT_TRUE(mock_running_thread->is_thread_start_called);
  ASSERT_TRUE(mock_running_thread->is_thread_stop_called);
}

TEST_F(RunningTest, LoopIterateProcessEventShouldExit) {
  setProcessEventResult(EVP_SHOULDEXIT);
  IterateStatus res = state->Iterate();
  ASSERT_EQ(res, IterateStatus::Ok);
  context = StateMachineContext::GetInstance(nullptr);
  ASSERT_EQ(context->GetNextState(), STATE_DESTROYING);
}

TEST_F(RunningTest, LoopIterateIdleNextState) {
  context = StateMachineContext::GetInstance(nullptr);
  context->SetNextState(STATE_IDLE);
  IterateStatus res = state->Iterate();
  ASSERT_EQ(res, IterateStatus::Ok);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
}

TEST_F(RunningTest, LoopIterateRunningNextState) {
  context = StateMachineContext::GetInstance(nullptr);
  context->SetNextState(STATE_DESTROYING);
  IterateStatus res = state->Iterate();
  ASSERT_EQ(res, IterateStatus::Ok);
  ASSERT_EQ(context->GetNextState(), STATE_DESTROYING);
}

TEST_F(RunningTest, LoopIterateContinue) {
  context = StateMachineContext::GetInstance(nullptr);
  context->SetNextState(STATE_RUNNING);
  IterateStatus res = state->Iterate();
  ASSERT_EQ(res, IterateStatus::Ok);
  ASSERT_EQ(context->GetNextState(), STATE_RUNNING);
}

TEST_F(CommonTest, OnResumeCalled) {
  ASSERT_EQ(wasOnStartCalled(), 0);
  context->SetCurrentState(StateFactory::Create(STATE_RUNNING));
  ASSERT_EQ(wasOnStartCalled(), 1);
}

TEST_F(CommonTest, ErrorHandlingOnStart) {
  MockRunningThread *mock_running_thread = new MockRunningThread;
  Running *running = new Running((RunningThread *)mock_running_thread);
  context->SetCurrentState(running);
  setOnStartError();
  ASSERT_NE(context->GetNextState(), STATE_IDLE);
  State *state = StateFactory::Create(STATE_RUNNING);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ("onStart call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  delete state;
}

TEST_F(RunningTest, OnStopCalled) {
  ASSERT_EQ(wasOnStopCalled(), 0);
  context->SetCurrentState(nullptr);
  ASSERT_EQ(wasOnStopCalled(), 1);
}

TEST_F(CommonTest, OnStartThreadStartThreadStopOnStop) {
  MockRunningThreadOrder *mock_running_thread = new MockRunningThreadOrder;
  Running *running = new Running((RunningThread *)mock_running_thread);
  context->SetCurrentState(running);
  context->SetCurrentState(nullptr);
  ASSERT_TRUE(mock_running_thread->on_start_before_thread_start);
  ASSERT_TRUE(mock_running_thread->on_stop_after_thread_stop);
}

TEST_F(CommonTest, ErrorHandlingOnStop) {
  MockRunningThread *mock_running_thread = new MockRunningThread;
  Running *running = new Running((RunningThread *)mock_running_thread);
  context->SetCurrentState(running);
  setOnStopError();
  context->SetCurrentState(nullptr);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
  State *state = StateFactory::Create(STATE_RUNNING);
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
  ASSERT_EQ(CODE_FAILED_PRECONDITION,
            context->GetDtdlModel()->GetResInfo()->GetCode());
  ASSERT_STREQ("onStop call gave error res=-1",
               context->GetDtdlModel()->GetResInfo()->GetDetailMsg());
  delete state;
}
