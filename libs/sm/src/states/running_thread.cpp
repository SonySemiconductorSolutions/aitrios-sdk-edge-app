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

#include "states/running_thread.hpp"

#include <assert.h>
#include <pthread.h>

#include "data_export_private.h"
#include "dtdl_model/properties.h"
#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "sm.h"
#include "sm_context.hpp"
#include "states/state_defs.h"
#include "states/state_utils.hpp"
#ifdef EVP_REMOTE_SDK
#include "../../../py/src/py_shared_state.hpp"
#endif

using EdgeAppLib::DataExportHasPendingOperations;

#define NUMBER_OF_ITERATIONS "number_of_iterations"

void RunningThread::ThreadStart() {
  LOG_DBG("Creating thread...");
  int res = 0;
  /* LCOV_EXCL_START: error check */
  if ((res = pthread_create(&command_thread, nullptr,
                            &RunningThread::ThreadEntrypoint, this)) != 0) {
    LOG_ERR("pthread_create failed: %d", res);
    /* LCOV_EXCL_STOP */
  }
  pthread_mutex_lock(&command_mutex);
  /* LCOV_EXCL_START: thread is always uninitialized */
  while (command == Command::UNINITIALIZED)
    pthread_cond_wait(&command_cond, &command_mutex);
  /* LCOV_EXCL_STOP */
  LOG_DBG("Thread created.");
  pthread_mutex_unlock(&command_mutex);
}

void RunningThread::ThreadStop() {
  LOG_DBG("Stopping thread...");
  pthread_mutex_lock(&command_mutex);
  if (command == Command::UNINITIALIZED || command == Command::EXIT) {
    LOG_DBG("Thread not initialized");
    pthread_mutex_unlock(&command_mutex);
    return;
  }
  command = Command::EXIT;
  pthread_mutex_unlock(&command_mutex);
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  void *status = nullptr;
  int res;
  time_t start_time, current_time;
  const int timeout_seconds = 60;
  start_time = time(NULL);
  while (1) {
#if defined(__APPLE__)
    res = 0;
#else
    res = pthread_tryjoin_np(command_thread, &status);
#endif
    if (res == 0) {
      break;
    } else {
#ifdef EVP_REMOTE_SDK
      pthread_mutex_lock(&shared_state.mutex);
      while (shared_state.operation_in_progress) {
        pthread_cond_wait(&shared_state.cond, &shared_state.mutex);
      }
      shared_state.process_event_in_progress = true;
      pthread_mutex_unlock(&shared_state.mutex);
#endif  // EVP_REMOTE_SDK
      EVP_RESULT res =
          EVP_processEvent(context->evp_client, EVP_PROCESSEVENT_TIMEOUT_MS);
#ifdef EVP_REMOTE_SDK
      pthread_mutex_lock(&shared_state.mutex);
      shared_state.process_event_in_progress = false;
      pthread_cond_signal(&shared_state.cond);
      pthread_mutex_unlock(&shared_state.mutex);
#endif  // EVP_REMOTE_SDK
    }
    current_time = time(NULL);
    if (difftime(current_time, start_time) >= timeout_seconds) {
      LOG_ERR("pthread_join timeout: %d", res);
      return;
    }
  }
  LOG_INFO("Thread stopped: %d", (int)(intptr_t)status);
}

IterateStatus RunningThread::ThreadLoopIterate() {
  LOG_TRACE("Calling onIterate");
  int res = 0;

  if ((res = onIterate()) != 0) { /* LCOV_EXCL_START: error check */
    StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
    EventHandleError(ON_ITERATE, res, context, STATE_IDLE);
    return IterateStatus::Error;
    /* LCOV_EXCL_STOP */
  }

  pthread_mutex_lock(&command_mutex);
  if (command == Command::EXIT) {
    pthread_mutex_unlock(&command_mutex);
    return IterateStatus::Break;
  }
  pthread_mutex_unlock(&command_mutex);

  return IterateStatus::Ok;
}

void *RunningThread::ThreadEntrypoint(void *arg) {
  RunningThread *running = (RunningThread *)arg;

  pthread_mutex_lock(&running->command_mutex);
  running->command = Command::RUNNING;
  pthread_cond_signal(&running->command_cond);
  pthread_mutex_unlock(&running->command_mutex);

  IterateStatus ls = IterateStatus::Ok;
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  uint32_t num_iters = context->GetDtdlModel()
                           ->GetCommonSettings()
                           ->GetInferenceSettings()
                           ->GetNumberOfIterations();

  bool is_infinite = num_iters == 0;
  for (int i = 0; is_infinite || i < num_iters; i++) {
    ls = running->ThreadLoopIterate();
    if (ls != IterateStatus::Ok) break;
  }

  int res =
      (ls == IterateStatus::Error) ? -1 : 0; /* LCOV_EXCL_LINE: error check */

  if (num_iters > 0 && res != -1) { /* LCOV_EXCL_BR_LINE: error check */
    context->SetNextState(STATE_COOLINGDOWN);
  }

  return (void *)(intptr_t)res;
}
