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

#include "sm_context.hpp"

#include <string.h>

#include "context.hpp"
#include "dtdl_model/properties.h"
#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "sm_configurator.hpp"
#include "states/state_utils.hpp"

StateMachineContext *StateMachineContext::GetInstance(State *state) {
  if (context_ == nullptr) {
    StateMachineContext *sm_context = new StateMachineContext();
    context_ = sm_context;
    sm_context->evp_client = EVP_initialize();
    sm_context->aitrios_sm_configurator =
        new StateMachineConfigurator(sm_context);
    sm_context->SetCurrentState(state);
  }
  return (StateMachineContext *)context_;
}

void StateMachineContext::SetCurrentState(State *state) {
  if (current_state != nullptr)
    delete current_state; /* LCOV_EXCL_LINE: null check */
  current_state = state;
  // next state is the same, except if the next state is Destroy
  if (state != nullptr) {
    if (GetNextState() == STATE_DESTROYING) {
      LOG_WARN("Edge app will stop in next iteration");
      return;
    }
    STATE current_enum = state->GetEnum();
    if (current_enum == STATE_IDLE || current_enum == STATE_RUNNING) {
      if (dtdl_model.GetCommonSettings()->GetProcessState() != current_enum) {
        dtdl_model.GetCommonSettings()->SetProcessState(current_enum);
      }
    }
    SetNextState(state->GetEnum());
  }
}

EdgeAppLibSensorCore StateMachineContext::GetSensorCore() { return core; }

void StateMachineContext::SetSensorCore(EdgeAppLibSensorCore core) {
  this->core = core;
}

EdgeAppLibSensorStream StateMachineContext::GetSensorStream() { return stream; }

void StateMachineContext::SetSensorStream(EdgeAppLibSensorStream stream) {
  this->stream = stream;
}

State *StateMachineContext::GetCurrentState() { return current_state; }

DtdlModel *StateMachineContext::GetDtdlModel() { return &dtdl_model; }

void StateMachineContext::SetPendingConfiguration(void *config,
                                                  size_t configlen) {
  if (pending_configuration) {
    LOG_WARN("Previous pending configuration not null");
    free(pending_configuration);
  }
  pending_configuration = malloc(configlen);
  if (pending_configuration == nullptr) {
    LOG_ERR("Failed to allocate memory for pending configuration");
    return;
  }
  memcpy(pending_configuration, config, configlen);
  pending_configuration_len = configlen;
}

size_t StateMachineContext::GetPendingConfiguration(void **config) {
  *config = pending_configuration;
  return pending_configuration_len;
}
void StateMachineContext::ClearPendingConfiguration() {
  if (pending_configuration) free(pending_configuration);
  pending_configuration = nullptr;
  pending_configuration_len = 0;
}

/* simple implementation of send state callback */
static void SendStateCallback(EVP_STATE_CALLBACK_REASON reason,
                              void *userData) {
  if (reason != EVP_STATE_CALLBACK_REASON_SENT &&
      reason != EVP_STATE_CALLBACK_REASON_OVERWRITTEN) {
    LOG_ERR("SendStateCallback: callback failed because of reason: %d", reason);
  }
  free(userData);
}

void StateMachineContext::SendState() {
  char *state = GetDtdlModel()->Serialize();
  EVP_RESULT res = EVP_sendState(evp_client, TOPIC, (void *)state,
                                 strlen(state), SendStateCallback, state);
  if (res != EVP_OK) {
    LOG_WARN("EVP_sendState failed: %d", res);
  }
}

StateMachineContext::~StateMachineContext() {
  LOG_TRACE("In StateMachineContext destructor");
  if (aitrios_sm_configurator) delete aitrios_sm_configurator;
  if (current_state) delete current_state;
}
