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

#ifndef STATES_RUNNING_HPP
#define STATES_RUNNING_HPP

#include "states/running_thread.hpp"
#include "states/state.hpp"
#include "states/state_utils.hpp"

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

class Running : public State {
#ifdef UNIT_TEST
  FRIEND_TEST(RunningTest, ThreadLoopIterate);
  FRIEND_TEST(RunningTest, ThreadLoopIterateError);
  FRIEND_TEST(RunningTest, LoopIterateProcessEventShouldExit);
  FRIEND_TEST(RunningTest, LoopIterateIdleNextState);
  FRIEND_TEST(RunningTest, LoopIterateRunningNextState);
  FRIEND_TEST(RunningTest, LoopIterateContinue);
#endif
 public:
  Running(RunningThread *running_thread);
  ~Running();
  IterateStatus Iterate();
  STATE GetEnum() { return STATE_RUNNING; }

 private:
  RunningThread *running_thread;
  bool is_failed_on_start = false;
};

#endif /* STATES_RUNNING_HPP */
