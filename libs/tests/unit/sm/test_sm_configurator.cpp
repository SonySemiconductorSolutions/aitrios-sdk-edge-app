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
#include "fixtures/common_fixture.hpp"
#include "sm_configurator.hpp"
#include "sm_context.hpp"
#include "states/idle.hpp"

using ::testing::Return;

TEST_F(CommonTest, TestInvalidTransition) {
  context->SetCurrentState(new Idle);
  StateMachineConfigurator configurator(context);
  ASSERT_EQ(configurator.UpdateProcessState(STATE_COOLINGDOWN), -1);
  ASSERT_EQ(configurator.UpdateProcessState(STATE_RUNNING), 0);
}
