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
 * @file data_export_types.h
 * @brief Header file for the EdgeAppLib Data Export types.
 * @details This file defines the types used in EdgeAppLib Data Export.
 */

#ifndef AITRIOS_DATA_EXPORT_TYPES_H
#define AITRIOS_DATA_EXPORT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef EdgeAppLibDataExportResult
 * @brief Represents the result of an EdgeAppLib Data Export operation.
 */
typedef enum {
  EdgeAppLibDataExportResultSuccess = 0,      /**< Operation succeeded. */
  EdgeAppLibDataExportResultFailure = 1,      /**< Operation failed. */
  EdgeAppLibDataExportResultTimeout = 2,      /**< Operation timed out. */
  EdgeAppLibDataExportResultInvalidParam = 3, /**< Invalid parameter. */
  EdgeAppLibDataExportResultDataTooLarge = 4, /**< Data size exceeds limits. */
  EdgeAppLibDataExportResultDenied = 5, /**< Operation denied, e.g., attempting
                     to send data without  the device in stream-mode. */
  EdgeAppLibDataExportResultEnqueued = 6, /**< Operation has been enqueed. */
  EdgeAppLibDataExportResultUninitialized =
      7 /**< Result has not yet been initialized. No operation has been
           performed. */
} EdgeAppLibDataExportResult;

/**
 * @typedef EdgeAppLibDataExportDataType
 * @brief Represents the datatype for EdgeAppLib Data Export data.
 */
typedef enum {
  EdgeAppLibDataExportRaw =
      0, /**< Represents raw data in unprocessed binary format. */
  EdgeAppLibDataExportMetadata = 1 /**< Represents metadata. */
} EdgeAppLibDataExportDataType;

/**
 * @typedef EdgeAppLibDataExportFuture
 * @brief Defines a structure used for managing the state and result of
 * asynchronous EdgeAppLib Data Export operations.
 */
typedef struct EdgeAppLibDataExportFuture EdgeAppLibDataExportFuture;

#ifdef __cplusplus
}
#endif

#endif /* AITRIOS_DATA_EXPORT_TYPES_H */
