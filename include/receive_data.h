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

#ifndef AITRIOS_RECEIVE_DATA_H
#define AITRIOS_RECEIVE_DATA_H

#include <stdint.h>

typedef struct {
  char *url;       /**< url to the data. */
  int urllen;      /**< Length of the data url. */
  char *filename;  /**< Filename, a relative path to save the data. */
  int filenamelen; /**< Length of the filename. */
  char *hash;      /**< saved hash which will be compared with. */
} EdgeAppLibReceiveDataInfo;

typedef enum {
  EdgeAppLibReceiveDataResultSuccess = 0,      /**< Operation succeeded. */
  EdgeAppLibReceiveDataResultFailure = 1,      /**< Operation failed. */
  EdgeAppLibReceiveDataResultTimeout = 2,      /**< Operation timed out. */
  EdgeAppLibReceiveDataResultInvalidParam = 3, /**< Invalid parameter. */
  EdgeAppLibReceiveDataResultDataTooLarge = 4, /**< Data size exceeds limits. */
  EdgeAppLibReceiveDataResultDenied = 5, /**< Operation denied, e.g., attempting
                     to send data without  the device in stream-mode. */
  EdgeAppLibReceiveDataResultEnqueued = 6, /**< Operation has been enqueed. */
  EdgeAppLibReceiveDataResultUninitialized =
      7 /**< Result has not yet been initialized. No operation has been
          performed. */
} EdgeAppLibReceiveDataResult;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Receive data from AITRIOS synchronously.
 *
 * This function initiates a synchronous operation to recieve data from AITRIOS.
 *
 * @param info The description for received data.
 * @param timeout_ms Timeout in milliseconds. -1 to wait until operation
 * ends.
 * @return Result of the synchronous operation.
 * @warning Do not call it in onIterate due to unsafe concurrent calling
 */
EdgeAppLibReceiveDataResult EdgeAppLibReceiveData(
    EdgeAppLibReceiveDataInfo *info, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* AITRIOS_RECEIVE_DATA_H */
