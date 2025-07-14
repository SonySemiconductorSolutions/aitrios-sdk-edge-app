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

#include "states/destroying.hpp"

#include "data_export.h"
#include "data_export_private.h"
#include "log.h"
#include "receive_data_private.h"
#include "sensor.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/state_defs.h"

using EdgeAppLib::DataExportUnInitialize;
using EdgeAppLib::SensorCoreCloseStream;
using EdgeAppLib::SensorCoreExit;

Destroying::Destroying() {
  LOG_DBG("Initializing the state");
  context = StateMachineContext::GetInstance(this);
}

IterateStatus Destroying::Iterate() {
  int res;
  if ((res = onDestroy()) != 0) {
    StateHandleError(ON_DESTROY, res);
    return IterateStatus::Error;
  }

  EdgeAppLibSensorCore core = context->GetSensorCore();
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  /* To avoid calling CloseStream with Uninitialized Handler */
  if ((core != 0) && (stream != 0)) {
    LOG_DBG("Closing the stream and exiting the core.");
    int32_t ret = -1;
    if ((ret = SensorCoreCloseStream(core, stream)) < 0) {
      LOG_ERR("SensorCoreCloseStream : ret=%d", ret);
      StateHandleError(SENSOR_CORE_CLOSE_STREAM, ret);
      return IterateStatus::Error;
    }
    if ((ret = SensorCoreExit(core)) < 0) {
      LOG_ERR("SensorCoreExit : ret=%d", ret);
      StateHandleError(SENSOR_CORE_EXIT, ret);
      return IterateStatus::Error;
    }
  } else {
    LOG_WARN("SensorCore or SensorStream is not initialized.");
  }

  EdgeAppLibDataExportResult ade_res = EdgeAppLibDataExportResultFailure;
  if ((ade_res = DataExportUnInitialize()) !=
      EdgeAppLibDataExportResultSuccess) {
    StateHandleError(AITRIOS_DATA_EXPORT_UNINITIALIZE, int(ade_res));
    return IterateStatus::Error;
  }
  EdgeAppLibReceiveDataResult are_res = EdgeAppLibReceiveDataResultFailure;
  if ((are_res = EdgeAppLibReceiveDataUnInitialize()) !=
      EdgeAppLibReceiveDataResultSuccess) {
    return IterateStatus::Error;
  }
  context->SetNextState(STATE_EXITING);
  LOG_DBG("Successfully ended the Iterate().");
  return IterateStatus::Ok;
}
