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

#ifndef AITRIOS_SM_CORE_HPP
#define AITRIOS_SM_CORE_HPP

#include "macros.h"
#include "states/state.hpp"

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

class StateMachine {
#ifdef UNIT_TEST
  FRIEND_TEST(StateMachine, LoopIteration);
  FRIEND_TEST(StateMachine, Run);
  FRIEND_TEST(StateMachine, LoopIterationCreateFailure);
  FRIEND_TEST(StateMachine, LoopIterationStartFailure);
  FRIEND_TEST(StateMachine, LoopIterationStartStopFailure);
  FRIEND_TEST(StateMachine, LoopIterationStopFailure);
#endif
 public:
  StateMachine();
  ~StateMachine();

  int Run();

 protected:
  UT_ATTRIBUTE IterateStatus LoopIterate();

  StateMachineContext *context;
};

#endif /* AITRIOS_SM_CORE_HPP */
