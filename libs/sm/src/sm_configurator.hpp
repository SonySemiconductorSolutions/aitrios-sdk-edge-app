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

#ifndef AITRIOS_SM_CONFIGURATOR_HPP
#define AITRIOS_SM_CONFIGURATOR_HPP

#include "context.hpp"

class StateMachineContext;

class StateMachineConfigurator {
 public:
  StateMachineConfigurator(StateMachineContext *context) : context(context){};
  int UpdateProcessState(STATE state);

 private:
  StateMachineContext *context = nullptr;
};

#endif /* AITRIOS_SM_CONFIGURATOR_HPP */
