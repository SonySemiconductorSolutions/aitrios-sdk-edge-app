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

#ifndef AITRIOS_SM_H
#define AITRIOS_SM_H

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief API for Event Functions
 *
 * Defines set of functions that developers must implement
 * to handle events during the application lifecycle.
 * These functions are called by the State Machine at specific states.
 */

/**
 * @brief Initializes the developer's code during the 'Creating' state.
 *
 * Initializes the developer's code. It can be used to start the stream with
 * sensors, initialize variables, and perform other setup tasks.
 *
 * @return 0 for success, -1 for failure.
 */
int onCreate();

/**
 * @brief Configure developer's code, State Machine or States.
 *
 * @warning It's the callee's responsibility to free the `value` parameter
 * to avoid memory leaks.
 *
 * @param topic
 * @param value
 * @param valuesize
 * @return 0 for success, -1 for failure.
 */
int onConfigure(char *topic, void *value, int valuelen);

/**
 * @brief Self-contained cycle of the application in the 'Running' state.
 *
 * @return 0 for success, -1 for failure.
 */
int onIterate();

/**
 * @brief Stops the application when transitioning from 'Running' to 'Idle'
 * state.
 *
 * @return 0 on success, -1 on failure.
 */
int onStop();

/**
 * @brief Resumes or starts the application when transitioning from 'Idle' or
 * 'Creating' to 'Running' state.
 *
 * @return 0 on success, -1 on failure.
 */
int onStart();

/**
 * @brief Cleans up and deallocates resources associated with the developer's
 * code.
 *
 * @return 0 for success, -1 for failure.
 */
int onDestroy();

#ifdef __cplusplus
}
#endif

#endif /* AITRIOS_SM_H */
