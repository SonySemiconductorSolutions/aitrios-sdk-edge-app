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

#ifndef AITRIOS_SM_UTILS_H
#define AITRIOS_SM_UTILS_H

#include <edgeapp/sensor.h>

#include "parson.h"

/**
 * @brief Prints error details from the EdgeAppLib sensor.
 *
 * Retrieves and prints the error message, level, and cause obtained from the
 * EdgeAppLib sensor. This function can be called when there is an error in the
 * EdgeAppLibSensor.
 */
void PrintSensorError();

int SetEdgeAppLibNetwork(EdgeAppLibSensorStream stream, JSON_Object *json);

#endif  // AITRIOS_SM_UTILS_H
