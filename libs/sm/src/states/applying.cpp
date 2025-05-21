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

#include "states/applying.hpp"

#include "data_export_private.h"
#include "log.h"
#include "sensor.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/state_defs.h"

using EdgeAppLib::SensorCoreCloseStream;
using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorCoreInit;
using EdgeAppLib::SensorCoreOpenStream;

static bool is_initialized = false;

// TODO: find some better way to replace the hardcoded stream key in
// applying.cpp
#ifdef PYTHON_MODULE
std::string s_py_stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
#endif

Applying::Applying() {
  LOG_DBG("Initializing the state");
  context = StateMachineContext::GetInstance(this);
}

IterateStatus Applying::Iterate() {
  if (!is_initialized) {
    EdgeAppLibSensorCore core = 0;
    int32_t ret = -1;
    if ((ret = SensorCoreInit(&core)) < 0) {
      LOG_ERR("SensorCoreInit : ret=%d", ret);
      StateHandleError(SENSOR_CORE_INIT, ret);
      return IterateStatus::Error;
    }
    EdgeAppLibSensorStream stream = 0;
#ifdef PYTHON_MODULE
    const char *stream_key = s_py_stream_key.c_str();
#else
    const char *stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
#endif
    if ((ret = SensorCoreOpenStream(core, stream_key, &stream)) < 0) {
      LOG_ERR("SensorCoreOpenStream : ret=%d", ret);
      SensorCoreExit(core);
      StateHandleError(SENSOR_CORE_OPEN_STREAM, ret);
      return IterateStatus::Error;
    }

    context->SetSensorCore(core);
    context->SetSensorStream(stream);

    DtdlModel *dtdl = context->GetDtdlModel();
    dtdl->InitializeValues();
    // NOTE: first state report with default values
    // at this stage `custom_settings` is empty

    int res = 0;
    if ((res = onCreate()) != 0) {
      EventHandleError(ON_CREATE, res, context, STATE_IDLE);
      SensorCoreCloseStream(core, stream);
      SensorCoreExit(core);
      context->SetSensorCore(0);
      context->SetSensorStream(0);
      return IterateStatus::Error;
    }
    is_initialized = true;
  }

  LOG_TRACE("Applying configuration");
  void *config = nullptr;
  size_t configlen = context->GetPendingConfiguration(&config);

  int res = config == nullptr;
  if (res == 0) res = context->GetDtdlModel()->Update(config);

  context->ClearPendingConfiguration();
  if (res != 0) { /* LCOV_EXCL_START: error check */
    LOG_ERR("Invalid configuration moving to Idle");
    context->SetNextState(STATE_IDLE);
    context->EnableNotification();
    return IterateStatus::Error;
  } else {
    STATE previous_state =
        (STATE)context->GetDtdlModel()->GetCommonSettings()->GetProcessState();
    LOG_DBG("Restoring state %d", previous_state);
    context->SetNextState(previous_state);
    // no explicit notification, depends whether DTDL has been updated
  } /* LCOV_EXCL_STOP */
  return IterateStatus::Ok;
}
