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

#ifndef DTDL_MODEL_UTILS_H
#define DTDL_MODEL_UTILS_H

#include "properties.h"
#include "sensor.h"

bool IsAlmostEqual(double a, double b);
int IsInteger(double value);
EdgeAppLibSensorErrorCause SmUtilsPrintSensorError();
CODE CodeFromSensorErrorCause(EdgeAppLibSensorErrorCause error_cause);

#endif /* DTDL_MODEL_PROPERTIES_H */
