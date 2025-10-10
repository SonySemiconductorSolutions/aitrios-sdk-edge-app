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

#include <math.h>
#include <stdio.h>

#include "log.h"
#include "properties.h"
#include "sensor.h"

using namespace EdgeAppLib;

bool IsAlmostEqual(double a, double b) { return fabs(a - b) < TOLERANCE; }

int IsInteger(double value) { return IsAlmostEqual(value - floor(value), 0.0); }

EdgeAppLibSensorErrorCause SmUtilsPrintSensorError() {
  return EdgeAppLibLogSensorError();
}

void CodeFromSensorErrorCause(EdgeAppLibSensorErrorCause error_cause,
                              CODE *code) {
  if (error_cause == AITRIOS_SENSOR_ERROR_OUT_OF_RANGE) {
    *code = CODE_OUT_OF_RANGE;
  } else if (error_cause ==
             AITRIOS_SENSOR_ERROR_INVALID_CAMERA_OPERATION_PARAMETER) {
    *code = CODE_INVALID_ARGUMENT;
  }
}
