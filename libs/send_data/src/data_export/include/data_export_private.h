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
 * @file data_export_private.h
 * @brief Header file for the private interfaces of the EdgeAppLib Data Export.
 * @details This file contains declarations of internal functions and structures
 * used by the EdgeAppLib Data Export. It is intended for use within the
 * EdgeAppLib Data Export implementation, not for external use.
 */

#ifndef AITRIOS_DATA_EXPORT_PRIVATE_H
#define AITRIOS_DATA_EXPORT_PRIVATE_H

#include <pthread.h>
#include <sys/stat.h>

#include "context.hpp"
#include "data_export_types.h"
#include "evp_c_sdk/sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  struct EVP_BlobLocalStore localStore;
  char *upload;
  char *blob_buff;      /* buffer for blob actions over memory */
  int blob_buff_size;   /* Max buffer size */
  int blob_buff_offset; /* Current buff size used */
  off_t size;
  uint32_t identifier;
} module_vars_t;

/**
 * @struct EdgeAppLibDataExportFuture
 * @brief Represents the state of an asynchronous operation.
 * @details This structure holds the necessary information to track and
 * synchronize the status of an asynchronous tasks.
 */
struct EdgeAppLibDataExportFuture {
  EdgeAppLibDataExportResult result; /**< @brief The result of the asynchronous
                        operation. This field is set to a value from the
                        EdgeAppLibDataExportResult enum, indicating the outcome
                        of the operation (e.g., success, failure). */

  pthread_mutex_t
      mutex; /**< @brief A mutex for synchronizing access to the structure. */

  pthread_cond_t cond; /**< @brief A condition variable for blocking and
                          waking threads.*/
  bool is_processed; /**< @brief true if EVP operation callback has been called.
                      */
  bool is_cleanup_requested; /**< @brief true if function
                                EdgeAppLibDataExportCleanup has been called. */
  bool is_cleanup_sent_data; /**< @brief true if function
                                EdgeAppLibDataExportCleanup has been called and
                                data has been sent. */

  module_vars_t module_vars; /**< @brief Arguments for evp module*/
};

#ifdef __cplusplus
}
#endif

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Cancels the pending callback operation.
 * @param none
 * @return EdgeAppLibDataExportResult indicating the success or failure
 */
/**
 * @brief Initializes the EdgeAppLib Data Export system with the given context.
 * @param context A pointer to the Context structure providing necessary
 * configurations and parameters required for initializing the system.
 * @param evp_client A pointer to the evp client which State Machine
 * initialized.
 * @return EdgeAppLibDataExportResult indicating the success or failure of the
 * initialization process. Returns EdgeAppLibDataExportResultSuccess on
 * successful initialization, or an appropriate error code otherwise.
 */
EdgeAppLibDataExportResult DataExportInitialize(Context *context,
                                                void *evp_client);

/**
 * @brief UnInitializes the current EdgeAppLib Data Export system.
 *
 * @return EdgeAppLibDataExportResult indicating the success or failure of the
 * uninitialization process. Returns EdgeAppLibDataExportResultSuccess on
 * successful uninitialization, or an appropriate error code otherwise.
 */
EdgeAppLibDataExportResult DataExportUnInitialize();

/**
 * @return True if there are pending operations.
 */
bool DataExportHasPendingOperations();

/**
 * @brief Formats a Unix timestamp in nanoseconds as yyyyMMddHHmmssSSS, in UTC.
 * @param buffer Buffer where to store formatted string.
 * @param buffer_size Size of buffer.
 * @param timestamp The Unix timestamp to be formatted in nanoseconds.
 */
void DataExportFormatTimestamp(char *buffer, size_t buffer_size,
                               uint64_t timestamp);
/**
 * @brief Get the upload filename extension based on datatype and codecsetting.
 * @param buffer Buffer where to store filename extension.
 * @param buffer_size Size of buffer.
 * @param datatype The type of the data to upload.
 */
void DataExportFileSuffix(char *buffer, size_t buffer_size,
                          EdgeAppLibDataExportDataType datatype);

#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib

#endif /* AITRIOS_DATA_EXPORT_PRIVATE_H */
