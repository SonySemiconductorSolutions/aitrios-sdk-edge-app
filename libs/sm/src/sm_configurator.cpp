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

#include "sm_configurator.hpp"

#include <string.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "states/state_utils.hpp"

int StateMachineConfigurator::UpdateProcessState(STATE state) {
  LOG_TRACE("Inside UpdateProcessState");
  if (state == context->GetNextState()) {
    LOG_INFO(
        "Requested state coincides with the previously configured one. "
        "Continuing with the previous configurations.");
    return 0;
  }

  if (IsFeasibleTransition(context->GetCurrentState()->GetEnum(), state)) {
    LOG_INFO(
        "Requested state transition is correct, setting the new "
        "configuration.");
    context->SetNextState(state);
    context->EnableNotification();
  } else {
    LOG_INFO(
        "Requested state transition from %d to %d is infeasible. "
        "Continuing with the previous configurations.",
        context->GetCurrentState()->GetEnum(), state);
    return -1;
  }
  return 0;
}
