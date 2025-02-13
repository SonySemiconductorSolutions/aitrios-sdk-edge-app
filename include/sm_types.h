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
 * @file sm_types.h
 * @brief Header file for the EdgeAppLib State Machine.
 * @details This file defines the types used in EdgeAppLib State Machine.
 */

#ifndef AITRIOS_SM_TYPE_H
#define AITRIOS_SM_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ResponseCodeOk = 0,
  ResponseCodeCancelled,
  ResponseCodeUnknown,
  ResponseCodeInvalidArgument,
  ResponseCodeDeadlineExceeded,
  ResponseCodeNotFound,
  ResponseCodeAlreadyExists,
  ResponseCodePermissionDenied,
  ResponseCodeResourceExhausted,
  ResponseCodeFailedPrecondition,
  ResponseCodeAborted,
  ResponseCodeOutOfRange,
  ResponseCodeUnimplemented,
  ResponseCodeInternal,
  ResponseCodeUnavaiable,
  ResponseCodeDataLoss,
  ResponseCodeUnauthenticated,
} ResponseCode;

#ifdef __cplusplus
}
#endif

#endif /* AITRIOS_SM_TYPE_H */
