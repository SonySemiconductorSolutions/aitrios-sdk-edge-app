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

#pragma once

#include <pthread.h>

// Shared state for synchronization
struct SharedState {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  bool process_event_in_progress;
  bool operation_in_progress;
  bool operation_cb_in_progress;
};

// Declare shared_state as extern to ensure it is shared
extern SharedState shared_state;
