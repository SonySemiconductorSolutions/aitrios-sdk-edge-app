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

#include "states/running.hpp"

#include <assert.h>

#include "data_export_private.h"
#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "pthread.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/state_defs.h"

using EdgeAppLib::DataExportHasPendingOperations;

Running::Running(RunningThread *running_thread) {
  LOG_DBG("Initializing the state");
  context = StateMachineContext::GetInstance(this);
  int res = 0;
  this->running_thread = running_thread;
  if ((res = onStart()) != 0) {
    EventHandleError(ON_START, res, context, STATE_IDLE);
    return;
  }
  context->SendState();
  running_thread->ThreadStart();
  LOG_DBG("Initialized.");
}

Running::~Running() {
  LOG_DBG("Destroying Running state");
  running_thread->ThreadStop();
  delete running_thread; /* LCOV_EXCL_EXCEPTION_BR_LINE: object delete */
  int res = 0;
  if ((res = onStop()) != 0)
    EventHandleError(ON_STOP, res, context, STATE_IDLE);
  LOG_DBG("Destroyed.");
}

IterateStatus Running::Iterate() {
  EVP_RESULT result =
      EVP_processEvent(context->evp_client, EVP_PROCESSEVENT_TIMEOUT_MS);

  if (result == EVP_SHOULDEXIT) {
    LOG_DBG("Exiting the main loop due to EVP_SHOULDEXIT");
    context->SetNextState(STATE_DESTROYING);
    return IterateStatus::Ok;
  }

  return IterateStatus::Ok;
}
