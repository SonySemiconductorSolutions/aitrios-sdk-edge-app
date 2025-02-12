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

#include "data_export/mock_data_export.hpp"
#include "event_functions/mock_sm.hpp"
#include "evp/mock_evp.hpp"
#include "fixtures/common_fixture.hpp"
#include "sm_context.hpp"
#include "states/coolingdown.hpp"
#include "states/creating.hpp"
#include "states/state_factory.hpp"

TEST_F(CommonTest, StateFactoryCreate) {
  State *state = StateFactory::Create(STATE_CREATING);
  ASSERT_EQ(state->GetEnum(), STATE_CREATING);
  delete state;
  state = StateFactory::Create(STATE_IDLE);
  ASSERT_EQ(state->GetEnum(), STATE_IDLE);
  delete state;
  state = StateFactory::Create(STATE_COOLINGDOWN);
  ASSERT_EQ(state->GetEnum(), STATE_COOLINGDOWN);
  delete state;
  state = StateFactory::Create(STATE_RUNNING);
  ASSERT_EQ(state->GetEnum(), STATE_RUNNING);
  delete state;
  state = StateFactory::Create(STATE_DESTROYING);
  ASSERT_EQ(state->GetEnum(), STATE_DESTROYING);
  delete state;
  state = StateFactory::Create(STATE_COUNT);
  ASSERT_EQ(state, nullptr);
}
