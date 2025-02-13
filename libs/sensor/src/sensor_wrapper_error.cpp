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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "edge_app/senscord.h"
#include "sensor.h"
#include "sensor_def.h"

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif
/**
 * Sensor Wrapper implementation API.
 */
enum EdgeAppLibSensorErrorLevel SensorGetLastErrorLevel(void) {
  LOG_TRACE("SensorGetLastErrorLevel start");
  EdgeAppLibSensorErrorLevel level =
      static_cast<EdgeAppLibSensorErrorLevel>(senscord_get_last_error_level());

  LOG_TRACE("SensorGetLastErrorLevel end");
  return level;
}

/**
 * Sensor Wrapper implementation API.
 */
enum EdgeAppLibSensorErrorCause SensorGetLastErrorCause(void) {
  LOG_TRACE("SensorGetLastErrorCause start");
  EdgeAppLibSensorErrorCause cause =
      static_cast<EdgeAppLibSensorErrorCause>(senscord_get_last_error_cause());

  LOG_TRACE("SensorGetLastErrorCause end");
  return cause;
}

/**
 * Sensor Wrapper implementation API.
 */
int32_t SensorGetLastErrorString(enum EdgeAppLibSensorStatusParam param,
                                 char *buffer, uint32_t *length) {
  LOG_TRACE("SensorGetLastErrorString start");
  int32_t result = senscord_get_last_error_string(
      static_cast<senscord_status_param_t>(param), buffer, length);

  LOG_TRACE("SensorGetLastErrorString end");
  return result;
}
#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib
