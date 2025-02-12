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

#ifndef STATES_STATE_HPP
#define STATES_STATE_HPP

#include <string>

#include "macros.h"
#include "sm_context.hpp"
#include "states/state_defs.h"
#include "states/state_factory.hpp"

enum class IterateStatus { Ok, Error, Break };

class State {
 public:
  virtual ~State() = default;
  virtual IterateStatus Iterate() = 0;
  virtual STATE GetEnum() = 0;

 protected:
  StateMachineContext *context;

  UT_ATTRIBUTE void StateHandleError(const char *event, int res);
};

#endif /* STATES_STATE_HPP */
