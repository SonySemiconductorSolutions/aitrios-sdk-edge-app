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
 * @file data_export.h
 * @brief Header file for the EdgeAppLib Data Export.
 * @details This file defines the interface for interacting with the EdgeAppLib
 * Data Export, including data types, result codes, and functions for
 * asynchronous operations. It provides functionalities such as sending data,
 * managing asynchronous tasks, and controlling the state of the EdgeAppLib Data
 * Export.
 */

#ifndef AITRIOS_DATA_EXPORT_H
#define AITRIOS_DATA_EXPORT_H

#include "data_export_types.h"
#include "parson.h"
#include "stdint.h"

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Waits for the completion of an asynchronous operation.
 *
 * This function blocks the current thread until the asynchronous operation
 * represented by the provided future is completed or until the specified
 * timeout period is reached.
 *
 * @note Calling this function is optional. The operation will eventually
 *       be executed. Its purpose is to synchronize the operation.
 * @warning Undefined behavior may occur if an invalid future or NULL is passed.
 *
 * @param future The future representing the operation.
 * @param timeout_ms Timeout in milliseconds. -1 to wait until operation ends.
 * @return Result of the asynchronous operation.
 */
EdgeAppLibDataExportResult DataExportAwait(EdgeAppLibDataExportFuture *future,
                                           int timeout_ms);

/**
 * @brief Cleans up resources associated with a future.
 *
 * Releases resources associated with the provided future. It is essential
 * to call this function to avoid memory leaks and ensure proper cleanup.
 *
 * @remark Cleaning up a future does not cancel the operation associated with
 * it.
 * @warning Undefined behavior may occur if an invalid future or NULL is passed.
 *
 * @param future The future to clean up.
 * @return Result of the clean-up operation.
 */
EdgeAppLibDataExportResult DataExportCleanup(
    EdgeAppLibDataExportFuture *future);

/**
 * @brief Sends data to AITRIOS asynchronously.
 *
 * This function initiates an asynchronous operation to send serialized data to
 * AITRIOS.
 *
 * @warning It's the caller's responsibility to keep portname and data valid
 * until the operation has finished.
 * @note Use EdgeAppLibDataExportAwait to verify that the operation has
 * finished.
 * @note Use EdgeAppLibDataExportIsEnabled to check if data upload is enabled.
 *
 * @param portname The port name of the destination. [Parameter currently
 * unused]
 * @param datatype The type of the data to upload.
 * @param data The serialized data to upload.
 * @param datalen The length of the serialized data.
 * @param timestamp The timestamp of the processed frame in nanoseconds.
 *
 * @return Reference to the future representing the asynchronous operation.
 *         Returns NULL on failure or when disabled.
 */
EdgeAppLibDataExportFuture *DataExportSendData(
    char *portname, EdgeAppLibDataExportDataType datatype, void *data,
    int datalen, uint64_t timestamp, uint32_t current = 1,
    uint32_t division = 1);

/**
 * @brief Send state asynchronously.
 *
 * @warning Takes ownership of state.
 *
 * @param topic Destination topic.
 * @param state State data.
 * @param statelen State length.
 * @return EdgeAppLibDataExportResult
 */
EdgeAppLibDataExportResult DataExportSendState(const char *topic, void *state,
                                               int statelen);
/**
 * @brief Notifies State Machine to transition to 'Idle' State.
 *
 * @return EdgeAppLibDataExportResult
 */
EdgeAppLibDataExportResult DataExportStopSelf();

/**
 * @brief Gets whether sending data of the specified type is enabled.
 *
 * @note When disabled, EdgeAppLibDataExportSendData will return NULL and not
 * upload any data.
 *
 * @param datatype The type of the data to upload.
 * @return True if enabled; otherwise, false.
 */
bool DataExportIsEnabled(EdgeAppLibDataExportDataType datatype);

/**
 * @brief Get port settings.
 *
 * This function returns a JSON object containing the current port settings.
 *
 * @return A JSON object containing the current port settings.
 */
JSON_Object *DataExportGetPortSettings(void);

#ifdef __cplusplus
}
#endif

}  // namespace EdgeAppLib

#endif /* AITRIOS_DATA_EXPORT_H */
