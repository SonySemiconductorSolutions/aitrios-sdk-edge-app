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

#include <gtest/gtest.h>

#include "dtdl_model/properties.h"
#include "states/state_utils.hpp"

TEST(StateUtils, IsFeasibleTransitionFromCreate) {
  int is_feasible;

  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_CREATING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_IDLE);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_RUNNING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_DESTROYING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_EXITING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_COOLINGDOWN);
  EXPECT_EQ(is_feasible, 0);
}

TEST(StateUtils, IsFeasibleTransitionFromIdle) {
  int is_feasible;

  is_feasible = IsFeasibleTransition(STATE_IDLE, STATE_CREATING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_IDLE, STATE_IDLE);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_IDLE, STATE_RUNNING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_IDLE, STATE_DESTROYING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_IDLE, STATE_EXITING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_COOLINGDOWN);
  EXPECT_EQ(is_feasible, 0);
}

TEST(StateUtils, IsFeasibleTransitionFromRun) {
  int is_feasible;

  is_feasible = IsFeasibleTransition(STATE_RUNNING, STATE_CREATING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_RUNNING, STATE_IDLE);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_RUNNING, STATE_RUNNING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_RUNNING, STATE_DESTROYING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_RUNNING, STATE_EXITING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_RUNNING, STATE_COOLINGDOWN);
  EXPECT_EQ(is_feasible, 1);
}

TEST(StateUtils, IsFeasibleTransitionFromSleep) {
  int is_feasible;

  is_feasible = IsFeasibleTransition(STATE_COOLINGDOWN, STATE_CREATING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_COOLINGDOWN, STATE_IDLE);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_COOLINGDOWN, STATE_RUNNING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_COOLINGDOWN, STATE_DESTROYING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_COOLINGDOWN, STATE_EXITING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_COOLINGDOWN, STATE_COOLINGDOWN);
  EXPECT_EQ(is_feasible, 1);
}

TEST(StateUtils, IsFeasibleTransitionFromDestroy) {
  int is_feasible;

  is_feasible = IsFeasibleTransition(STATE_DESTROYING, STATE_CREATING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_DESTROYING, STATE_IDLE);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_DESTROYING, STATE_RUNNING);
  EXPECT_EQ(is_feasible, 0);
  is_feasible = IsFeasibleTransition(STATE_DESTROYING, STATE_DESTROYING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_DESTROYING, STATE_EXITING);
  EXPECT_EQ(is_feasible, 1);
  is_feasible = IsFeasibleTransition(STATE_CREATING, STATE_COOLINGDOWN);
  EXPECT_EQ(is_feasible, 0);
}
