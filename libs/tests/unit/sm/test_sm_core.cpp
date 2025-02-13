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
#include "sm_context.hpp"
#include "sm_core.hpp"
#include "states/creating.hpp"

using ::testing::Return;

class MockStateMachine : public StateMachine {
  FRIEND_TEST(StateMachine, StateMachinRun);

 public:
  MOCK_METHOD0(LoopIterate, IterateStatus());
};

TEST(StateMachine, Run) {
  MockStateMachine sm;
  EXPECT_CALL(sm, LoopIterate()).WillRepeatedly(Return(IterateStatus::Break));
  EXPECT_EQ(sm.Run(), 0);
}

TEST(StateMachine, RunFailed) {
  MockStateMachine sm;
  EXPECT_CALL(sm, LoopIterate())
      .WillOnce(Return(IterateStatus::Ok))
      .WillOnce(Return(IterateStatus::Error));
  EXPECT_EQ(sm.Run(), 0);
}

TEST(StateMachine, LoopIteration) {
  StateMachine sm;
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_CREATING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_CREATING);
  // // create -> idle
  sm.LoopIterate();
  // force same state
  sm.LoopIterate();
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);
  sm.context->SetNextState(STATE_DESTROYING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_DESTROYING);
  // // run -> destroy
  EXPECT_EQ(sm.LoopIterate(), IterateStatus::Ok);
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_DESTROYING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_DESTROYING);
  // // destroy -> stop state machine
  EXPECT_EQ(sm.LoopIterate(), IterateStatus::Break);
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_DESTROYING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_EXITING);
}

TEST(StateMachine, LoopIterationCreateFailure) {
  StateMachine sm;
  setOnCreateError();
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_CREATING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_CREATING);
  // create -> Idle
  sm.LoopIterate();
  // Error at Idle keep the state Idle
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);
}

TEST(StateMachine, LoopIterationStartFailure) {
  StateMachine sm;
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_CREATING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_CREATING);
  // create -> idle
  sm.LoopIterate();
  // force same state
  sm.LoopIterate();
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);

  setOnStartError();

  sm.context->SetNextState(STATE_RUNNING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_RUNNING);
  // idle -> run -> idle
  EXPECT_EQ(sm.LoopIterate(), IterateStatus::Ok);

  resetOnStart();

  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);
}

TEST(StateMachine, LoopIterationStartStopFailure) {
  StateMachine sm;
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_CREATING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_CREATING);
  // create -> idle
  sm.LoopIterate();
  // force same state
  sm.LoopIterate();
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);

  setOnStartError();
  setOnStopError();

  sm.context->SetNextState(STATE_RUNNING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_RUNNING);
  // idle -> run -> idle
  EXPECT_EQ(sm.LoopIterate(), IterateStatus::Ok);

  resetOnStart();
  resetOnStop();

  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);
}

TEST(StateMachine, LoopIterationStopFailure) {
  StateMachine sm;
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_CREATING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_CREATING);
  // create -> idle
  sm.LoopIterate();
  // force same state
  sm.LoopIterate();
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);
  sm.context->SetNextState(STATE_RUNNING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_RUNNING);
  // idle -> run
  EXPECT_EQ(sm.LoopIterate(), IterateStatus::Ok);
  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_RUNNING);
  EXPECT_EQ(sm.context->GetNextState(), STATE_RUNNING);

  setOnStopError();

  sm.context->SetNextState(STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);
  // run -> idle
  EXPECT_EQ(sm.LoopIterate(), IterateStatus::Ok);

  resetOnStop();

  EXPECT_EQ(sm.context->GetCurrentState()->GetEnum(), STATE_IDLE);
  EXPECT_EQ(sm.context->GetNextState(), STATE_IDLE);
}
