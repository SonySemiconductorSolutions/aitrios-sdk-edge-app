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

/**
 * @file send_data.h
 * @brief Header file for the EdgeAppLib Send Data.
 * @details This file defines the interface for interacting with the EdgeAppLib
 * Send Data, including data types, result codes, and functions for
 * synchronous operations. It provides functionalities such as
 * sending data.
 */

#ifndef AITRIOS_SEND_DATA_H
#define AITRIOS_SEND_DATA_H

#define MAX_NUMBER_OF_INFERENCE_QUEUE 100

#include "send_data_types.h"
#include "stdint.h"

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Sends data to AITRIOS synchronously.
 *
 * This function initiates a synchronous operation to send  data to AITRIOS.
 *
 * @param data The serialized data to upload.
 * @param datalen The length of the serialized data.
 * @param datatype The type of the data to upload.
 * @param timestamp The timestamp of the processed frame in nanoseconds.
 * @param timeout_ms Timeout in milliseconds. -1 to wait until operation
 * ends.
 * @return Result of the synchronous operation.
 */
EdgeAppLibSendDataResult SendDataSyncMeta(void *data, int datalen,
                                          EdgeAppLibSendDataType datatype,
                                          uint64_t timestamp, int timeout_ms);

#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib

#endif /* AITRIOS_SEND_DATA_H */
