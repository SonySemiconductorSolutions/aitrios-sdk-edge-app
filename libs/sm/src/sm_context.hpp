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

#ifndef AITRIOS_SM_CONTEXT_HPP
#define AITRIOS_SM_CONTEXT_HPP

#include "context.hpp"
#include "dtdl_model/dtdl_model.hpp"
#include "evp_c_sdk/sdk.h"
#include "macros.h"
#include "sensor.h"
#include "sm_configurator.hpp"

class State;

class StateMachineContext : public Context {
 public:
  static StateMachineContext *GetInstance(State *state);
  State *GetCurrentState();

  UT_ATTRIBUTE DtdlModel *GetDtdlModel();
  EdgeAppLibSensorCore GetSensorCore();
  EdgeAppLibSensorStream GetSensorStream();

  void SetCurrentState(State *state);
  void SetSensorCore(EdgeAppLibSensorCore core);
  void SetSensorStream(EdgeAppLibSensorStream stream);

  void SetPendingConfiguration(void *config, size_t configlen);
  size_t GetPendingConfiguration(void **config);
  void ClearPendingConfiguration();
  void SendState();

  EVP_client *evp_client = nullptr;
  StateMachineConfigurator *aitrios_sm_configurator = nullptr;

 protected:
  ~StateMachineContext();

 private:
  DtdlModel dtdl_model;
  State *current_state = nullptr;
  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream = 0;
  void *pending_configuration = nullptr;
  int pending_configuration_len = 0;
};

#endif /* AITRIOS_SM_CONTEXT_HPP */
