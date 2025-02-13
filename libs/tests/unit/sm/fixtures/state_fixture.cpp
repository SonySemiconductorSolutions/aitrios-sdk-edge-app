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

#include "fixtures/state_fixture.hpp"

#include <gtest/gtest.h>

#include "data_export/mock_data_export.hpp"
#include "event_functions/mock_sm.hpp"
#include "evp/mock_evp.hpp"
#include "sm_context.hpp"

void StateTest::SetUp() {
  CommonTest::SetUp();
  resetSetConfigurationCallbackCalled();
  resetProcessEventCalled();
  setProcessEventResult(EVP_OK);
  resetOnCreate();
  resetOnStart();
  resetOnConfigure();
  resetOnIterate();
  resetOnStop();
  resetOnDestroy();
  resetEdgeAppLibDataExportInitialize();
  resetEdgeAppLibDataExportUnInitialize();
}
