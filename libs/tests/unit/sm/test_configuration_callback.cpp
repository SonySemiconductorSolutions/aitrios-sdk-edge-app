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

#include "callbacks/configuration.hpp"
#include "dtdl_model/properties.h"
#include "event_functions/mock_sm.hpp"
#include "fixtures/state_fixture.cpp"
#include "sm_context.hpp"
#include "states/creating.hpp"
#include "states/idle.hpp"
#include "states/running.hpp"

class ConfigurationCallbackTest : public CommonTest {};

TEST_F(ConfigurationCallbackTest, onConfigureCalled) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  ASSERT_NE(context->GetNextState(), STATE_APPLYING);
  const char *in_config = "{\"req_info\":{\"req_id\": \"ic_sample\"},\"a\": 3}";
  configuration_cb(nullptr, in_config, strlen(in_config), nullptr);
  ASSERT_EQ(context->GetNextState(), STATE_APPLYING);
  char *out_config = nullptr;
  size_t size = context->GetPendingConfiguration((void **)&out_config);
  ASSERT_STREQ(in_config, out_config);
  context->Delete();
}

TEST_F(ConfigurationCallbackTest, onConfigureCalledFailure) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  ASSERT_NE(context->GetNextState(), STATE_APPLYING);

  const char *in_config_1 = "";
  configuration_cb(nullptr, in_config_1, strlen(in_config_1), nullptr);
  ASSERT_NE(context->GetNextState(), STATE_APPLYING);

  const char *in_config_2 = "[\"a\", \"b\"]";
  configuration_cb(nullptr, in_config_2, strlen(in_config_2), nullptr);
  ASSERT_NE(context->GetNextState(), STATE_APPLYING);

  const char *in_config_3 = "{\"a\": 3}";
  configuration_cb(nullptr, in_config_3, strlen(in_config_3), nullptr);
  ASSERT_NE(context->GetNextState(), STATE_APPLYING);

  const char *in_config_4 =
      "{\"req_info\":{\"req_id\": \"ic_sample\"},\"a\": 3}";
  context->GetDtdlModel()->Update((void *)in_config_4);
  configuration_cb(nullptr, in_config_4, strlen(in_config_4), nullptr);
  ASSERT_NE(context->GetNextState(), STATE_APPLYING);

  context->Delete();
}
