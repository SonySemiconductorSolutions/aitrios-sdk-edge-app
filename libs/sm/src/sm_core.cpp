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

#include "sm_core.hpp"

#include <assert.h>

#include "data_export.h"
#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_context.hpp"
#include "states/state_factory.hpp"

StateMachine::StateMachine() {
  context =
      StateMachineContext::GetInstance(StateFactory::Create(STATE_CREATING));
}

StateMachine::~StateMachine() { context->Delete(); }

IterateStatus StateMachine::LoopIterate() {
  IterateStatus status = context->GetCurrentState()->Iterate();

  if (status == IterateStatus::Error) {
    LOG_WARN("State %d Iterate failed", context->GetCurrentState()->GetEnum());
  }

  STATE current_state_enum = context->GetCurrentState()->GetEnum();
  LOG_DBG("StateMachine::run: current_state_enum %d", current_state_enum);
  if (current_state_enum == STATE_DESTROYING) return IterateStatus::Break;

  STATE next_state_enum = context->GetNextState();
  LOG_DBG("StateMachine::run: next_state_enum %d", next_state_enum);
  if (next_state_enum != current_state_enum) {
    State *state = StateFactory::Create(next_state_enum);
    STATE new_next_state_enum = context->GetNextState();
    if (new_next_state_enum != next_state_enum &&
        next_state_enum == STATE_RUNNING && new_next_state_enum == STATE_IDLE) {
      // onStart failed. Transition to idle.
      delete state;
      state = StateFactory::Create(new_next_state_enum);
    }
    context->SetCurrentState(state);
  }

  // sending state after setting it to avoid reporting states before being
  // effective
  if (context->IsPendingNotification()) {
    context->SendState();
    context->ClearNotification();
  }

  return IterateStatus::Ok;
}

int StateMachine::Run() {
  LOG_DBG("Running State Machine.");
  LOG_DBG("StateMachine::run: current_state_enum %d",
          context->GetCurrentState()->GetEnum());
  LOG_DBG("StateMachine::run: next_state_enum %d", context->GetNextState());
  while (true) {
    IterateStatus res = LoopIterate();
    if (res != IterateStatus::Ok) break;
  } /* LCOV_EXCL_LINE: condition always true. Exit loop with break */
  LOG_DBG("State Machine: exiting gracefully. Thanks!");
  return 0;
}
