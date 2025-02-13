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

#include "states/idle.hpp"

#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/idle.hpp"
#include "states/state_defs.h"
#include "states/state_utils.hpp"

Idle::Idle() {
  LOG_DBG("Initializing the state");
  context = StateMachineContext::GetInstance(this);
}

IterateStatus Idle::Iterate() {
  EVP_RESULT result =
      EVP_processEvent(context->evp_client, EVP_PROCESSEVENT_TIMEOUT_MS);
  if (result == EVP_SHOULDEXIT) {
    LOG_DBG("Exiting the main loop due to EVP_SHOULDEXIT");
    context->SetNextState(STATE_DESTROYING);
    return IterateStatus::Ok;
  }
  return IterateStatus::Ok;
}
