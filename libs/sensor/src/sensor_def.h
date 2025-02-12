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

#ifndef _AITRIOS_SENSOR_DEF_H_
#define _AITRIOS_SENSOR_DEF_H_

#include "log.h"
#include "memory_manager.hpp"
/**
 * @struct EdgeAppLibSensorRawMemoryRef
 * @brief The structure of raw data
 * @details Output from EdgeAppLibSensor channel object
 */
struct EdgeAppLibSensorRawMemoryRef {
  MemoryRef address;  /**< virtual address */
  size_t size;        /**< data size */
  char *type;         /**< data type*/
  uint64_t timestamp; /**< nanoseconds timestamp captured by the device */
};

#endif  // _AITRIOS_SENSOR_DEF_H_
