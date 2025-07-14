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

#include "states/creating.hpp"

#include "callbacks/configuration.hpp"
#include "data_export.h"
#include "data_export_private.h"
#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "receive_data_private.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/state_defs.h"

using EdgeAppLib::DataExportInitialize;

Creating::Creating() {
  LOG_DBG("Initializing the state");
  context = StateMachineContext::GetInstance(this);
}

IterateStatus Creating::Iterate() {
  EdgeAppLibDataExportResult ade_res = EdgeAppLibDataExportResultFailure;
  EVP_setConfigurationCallback(context->evp_client, configuration_cb, context);
  if ((ade_res = DataExportInitialize(context, context->evp_client)) !=
      EdgeAppLibDataExportResultSuccess) {
    StateHandleError(AITRIOS_DATA_EXPORT_INITIALIZE, int(ade_res));
    return IterateStatus::Error;
  }

  EdgeAppLibReceiveDataResult are_res = EdgeAppLibReceiveDataResultFailure;
  if ((are_res = EdgeAppLibReceiveDataInitialize(context->evp_client)) !=
      EdgeAppLibReceiveDataResultSuccess) {
    return IterateStatus::Error;
  }

  context->SetNextState(STATE_IDLE);
  context->EnableNotification();
  LOG_DBG("Successfully ended Iterate().");
  return IterateStatus::Ok;
}
