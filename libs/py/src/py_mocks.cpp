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

#include <senscord.h>

#include "log.h"

// Mock implementations of some functions needed when linking with Senscord

// Only implemented in senscord WAMR API, not C API
int32_t senscord_channel_get_raw_data_handle(
    senscord_channel_t channel, struct senscord_raw_data_handle_t *raw_data) {
  LOG_CRITICAL("senscord_channel_get_raw_data_handle is not implemented!");
  return -1;
}

// Not linking with Esf
int32_t EsfSensorLatencySetMode(bool is_enable, uint32_t backlog) { return 0; }

int32_t EsfSensorLatencyGetTimestamps(uint64_t sequence_number,
                                      EsfSensorLatencyTimestamps *timestamps) {
  for (size_t i = 0; i < ESF_SENSOR_LATENCY_POINTS_MAX; ++i) {
    timestamps->points[i] = i * 100;
  }
  return 0;
}
