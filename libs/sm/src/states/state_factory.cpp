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

#include "states/state_factory.hpp"

#include "states/applying.hpp"
#include "states/coolingdown.hpp"
#include "states/creating.hpp"
#include "states/destroying.hpp"
#include "states/idle.hpp"
#include "states/running.hpp"

State *StateFactory::Create(STATE state) {
  switch (state) {
    case STATE_CREATING:
      return new Creating;
    case STATE_IDLE:
      return new Idle;
    case STATE_RUNNING:
      return new Running(new RunningThread);
    case STATE_DESTROYING:
      return new Destroying;
    case STATE_COOLINGDOWN:
      return new CoolingDown;
    case STATE_APPLYING:
      return new Applying;
    default:
      break;
  }
  return NULL;
}
