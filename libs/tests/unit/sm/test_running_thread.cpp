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
#include "states/state_factory.hpp"

#define REPEAT_TEST 10

using ::testing::Range;
using ::testing::Return;

class RunningTest : public CommonTest,
                    public ::testing::WithParamInterface<int> {};

TEST(RunningTest, StoppingNonInitialized) {
  RunningThread running_thread;
  running_thread.ThreadStop();
}

#define TEST_INPUT_NUMBER_OF_ITERATIONS_1 "{\"number_of_iterations\": 1}"

TEST(RunningTest, Thread) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT_NUMBER_OF_ITERATIONS_1);
  JSON_Object *obj1 = json_object(value1);
  StateMachineContext *context =
      StateMachineContext::GetInstance(StateFactory::Create(STATE_RUNNING));
  InferenceSettings *obj =
      context->GetDtdlModel()->GetCommonSettings()->GetInferenceSettings();
  obj->Apply(obj1);

  context->SetCurrentState(nullptr);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(), "");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(), CODE_OK);

  context->Delete();
  json_value_free(value1);
}

TEST(RunningTest, ThreadOnIterateError) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT_NUMBER_OF_ITERATIONS_1);
  JSON_Object *obj1 = json_object(value1);
  setOnIterateError();
  StateMachineContext *context =
      StateMachineContext::GetInstance(StateFactory::Create(STATE_RUNNING));
  InferenceSettings *obj =
      context->GetDtdlModel()->GetCommonSettings()->GetInferenceSettings();
  obj->Apply(obj1);
  context->SetCurrentState(new Idle);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "onIterate call gave error res=-1");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_FAILED_PRECONDITION);
  context->Delete();
  json_value_free(value1);
  resetOnIterate();
}

#define TEST_INPUT_LIMITED "{\"number_of_iterations\": 4}"

TEST(RunningTest, LimitedIterationsThread) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT_LIMITED);
  JSON_Object *obj1 = json_object(value1);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  InferenceSettings *obj =
      context->GetDtdlModel()->GetCommonSettings()->GetInferenceSettings();
  obj->Apply(obj1);
  CodecSettings *codec =
      context->GetDtdlModel()->GetCommonSettings()->GetCodecSettings();
  ASSERT_NE(codec, nullptr);

  RunningThread running_thread;
  running_thread.ThreadEntrypoint(&running_thread);
  ASSERT_EQ(running_thread.command, RunningThread::Command::RUNNING);
  ASSERT_EQ(context->GetNextState(), STATE_COOLINGDOWN);
  json_value_free(value1);
  context->Delete();
}

TEST_P(RunningTest, StartStop) {
  RunningThread running_thread;
  ASSERT_EQ(running_thread.command, RunningThread::Command::UNINITIALIZED);
  ASSERT_EQ(wasOnIterateCalled(), 0);
  running_thread.ThreadStart();
  ASSERT_EQ(running_thread.command, RunningThread::Command::RUNNING);
  running_thread.ThreadStop();
  ASSERT_EQ(running_thread.command, RunningThread::Command::EXIT);
  ASSERT_EQ(wasOnIterateCalled(), 1);
}
INSTANTIATE_TEST_CASE_P(IterateCreatesThread, RunningTest,
                        Range(0, REPEAT_TEST));

TEST_P(RunningTest, StopUninitialized) {
  RunningThread running_thread;
  running_thread.ThreadStart();
  running_thread.ThreadStop();
  running_thread.ThreadStop();
}
