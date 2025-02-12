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

#ifndef STATES_IDLE_HPP
#define STATES_IDLE_HPP

#include "states/state.hpp"
#include "states/state_utils.hpp"

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

class Idle : public State {
#ifdef UNIT_TEST
  FRIEND_TEST(IdleTest, OnResumeCalled);
  FRIEND_TEST(RunningTest, OnResumeCalled);
  FRIEND_TEST(IdleTest, IterateCurrentStateIsIdle);
  FRIEND_TEST(IdleTest, TestExitAtEvpShouldExit);
#endif
 public:
  Idle();
  IterateStatus Iterate();
  STATE GetEnum() { return STATE_IDLE; }
};

#endif /* STATES_IDLE_HPP */
