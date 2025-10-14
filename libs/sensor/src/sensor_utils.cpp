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

#include "log.h"
#include "sensor.h"

#define BUFSIZE 128

EdgeAppLibSensorErrorCause EdgeAppLibLogSensorError() {
  uint32_t length = BUFSIZE;
  char message_buffer[BUFSIZE] = {0};
  EdgeAppLib::SensorGetLastErrorString(
      EdgeAppLibSensorStatusParam::AITRIOS_SENSOR_STATUS_PARAM_MESSAGE,
      message_buffer, &length);

  EdgeAppLibSensorErrorCause cause = EdgeAppLib::SensorGetLastErrorCause();
  LOG_ERR("level: %d - cause: %d - message: %s",
          EdgeAppLib::SensorGetLastErrorLevel(), cause, message_buffer);

  return cause;
}
