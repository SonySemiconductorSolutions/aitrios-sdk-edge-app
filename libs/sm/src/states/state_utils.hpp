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

#ifndef STATES_STATE_UTILS_HPP
#define STATES_STATE_UTILS_HPP

#include <memory>
#include <vector>

#include "dtdl_model/properties.h"
#include "states/state.hpp"
#include "states/state_defs.h"

STATE StringToState(const char *text);

State *StateFromEnum(STATE state);

int IsFeasibleTransition(STATE start, STATE end);

void EventHandleError(const char *event, int res, StateMachineContext *context,
                      STATE next_state, bool is_update_res_info = true,
                      CODE code = CODE_FAILED_PRECONDITION);

#endif /* STATES_STATE_UTILS_HPP */
