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

#ifndef STATES_RUNNING_THREAD_HPP
#define STATES_RUNNING_THREAD_HPP

#include "macros.h"
#include "states/state.hpp"
#include "states/state_utils.hpp"
#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

class RunningThread {
#ifdef UNIT_TEST
  FRIEND_TEST(RunningTest, ThreadLoopIterate);
  FRIEND_TEST(RunningTest, ThreadLoopIterateError);
  FRIEND_TEST(RunningTest, LoopIterateProcessEventShouldExit);
  FRIEND_TEST(RunningTest, LoopIterateIdleNextState);
  FRIEND_TEST(RunningTest, LoopIterateRunningNextState);
  FRIEND_TEST(RunningTest, LoopIterateContinue);
#endif
 public:
  enum class Command { UNINITIALIZED, RUNNING, EXIT };

  UT_ATTRIBUTE void ThreadStart();
  UT_ATTRIBUTE void ThreadStop();

  //  private:
  IterateStatus ThreadLoopIterate();
  static void *ThreadEntrypoint(void *arg);

  pthread_t command_thread;
  pthread_mutex_t command_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t command_cond = PTHREAD_COND_INITIALIZER;
  Command command = Command::UNINITIALIZED;
};

#endif /* STATES_RUNNING_THREAD_HPP */
