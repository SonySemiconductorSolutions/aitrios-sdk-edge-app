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
 * @file send_data_types.h
 * @brief Header file for the EdgeAppLib Send Data types.
 * @details This file defines the types used in EdgeAppLib Send Data.
 */

#ifndef AITRIOS_SEND_DATA_TYPES_H
#define AITRIOS_SEND_DATA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  EdgeAppLibSendDataResultSuccess = 0,      /**< Operation succeeded. */
  EdgeAppLibSendDataResultFailure = 1,      /**< Operation failed. */
  EdgeAppLibSendDataResultTimeout = 2,      /**< Operation timed out. */
  EdgeAppLibSendDataResultInvalidParam = 3, /**< Invalid parameter. */
  EdgeAppLibSendDataResultDataTooLarge = 4, /**< Data size exceeds limits. */
  EdgeAppLibSendDataResultDenied = 5,   /**< Operation denied, e.g., attempting
                       to send data without  the device in stream-mode. */
  EdgeAppLibSendDataResultEnqueued = 6, /**< Operation has been enqueed. */
  EdgeAppLibSendDataResultUninitialized =
      7 /**< Result has not yet been initialized. No operation has been
           performed. */
} EdgeAppLibSendDataResult;

typedef enum {
  EdgeAppLibSendDataBase64 = 0,
  EdgeAppLibSendDataJson = 1
} EdgeAppLibSendDataType;

struct EdgeAppLibImageProperty {
  uint32_t width;        /**< Image width. */
  uint32_t height;       /**< Image height. */
  uint32_t stride_bytes; /**< Image stride. */
  /** The format of a pixel. */
  char pixel_format[64];
};

#ifdef __cplusplus
}
#endif

#endif /* AITRIOS_SEND_DATA_TYPES_H */
