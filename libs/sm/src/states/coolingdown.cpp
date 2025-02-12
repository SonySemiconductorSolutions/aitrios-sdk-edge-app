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

#include "states/coolingdown.hpp"

#include "data_export_private.h"
#include "log.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/state_defs.h"

using EdgeAppLib::DataExportHasPendingOperations;

CoolingDown::CoolingDown() {
  LOG_DBG("Initializing the state");
  context = StateMachineContext::GetInstance(this);
}

IterateStatus CoolingDown::Iterate() {
  if (context->GetNextState() == STATE_DESTROYING) {
    LOG_INFO("Stopping execution");
    return IterateStatus::Break;
  }
  while (DataExportHasPendingOperations()) { /* LCOV_EXCL_START: no
                                                       pending operations to
                                                       avoid infinite loop */
    EVP_processEvent(context->evp_client, EVP_PROCESSEVENT_TIMEOUT_MS);
    /* LCOV_EXCL_STOP */
  }
  context->EnableNotification();
  context->SetNextState(STATE_IDLE);
  LOG_DBG("Successfully ended the Iterate().");
  return IterateStatus::Ok;
}
