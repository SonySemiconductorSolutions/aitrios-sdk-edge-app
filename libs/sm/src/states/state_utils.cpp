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

#include "states/state_utils.hpp"

#include <memory>
#include <string>
#include <vector>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_configurator.hpp"

static const int kFeasibleTransitionBitmap[STATE_COUNT][STATE_COUNT] = {
    // create, idle, run, destroy, exiting, coolingdown, applying
    {1, 0, 1, 1, 0, 0, 0},  // create
    {0, 1, 1, 1, 0, 0, 1},  // idle
    {0, 1, 1, 1, 0, 1, 1},  // run
    {0, 0, 0, 1, 1, 0, 0},  // destroy
    {0, 0, 0, 0, 1, 0, 0},  // exiting
    {0, 0, 0, 1, 0, 1, 0},  // coolingdown
    {0, 1, 1, 1, 0, 0, 0}   // applying
};

int IsFeasibleTransition(STATE start, STATE end) {
  return kFeasibleTransitionBitmap[start][end];
}

void EventHandleError(const char *event, int res, StateMachineContext *context,
                      STATE next_state, bool is_update_res_info, CODE code) {
  LOG_ERR("Error in %s (ret=%d).", event, res);
  char buf[LOGBUGSIZE] = "";
  snprintf(buf, LOGBUGSIZE, "%s call gave error res=%d", event, res);
  DtdlModel *dtdl = context->GetDtdlModel();
  dtdl->GetCommonSettings()->SetProcessState(next_state);
  if (is_update_res_info) {
    dtdl->GetResInfo()->SetDetailMsg(buf);
    dtdl->GetResInfo()->SetCode(code);
  } else {
    LOG_ERR("%s code=%d", buf, code);
  }
}
