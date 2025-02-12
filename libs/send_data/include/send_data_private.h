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
 * @file send_data_private.h
 * @brief Header file for the private interfaces of the EdgeAppLib Send Data.
 * @details This file contains declarations of internal functions and structures
 * used by the EdgeAppLib Send Data. It is intended for use within the
 * EdgeAppLib Send Data implementation, not for external use.
 */

#ifndef SEND_DATA_PRIVATE_H
#define SEND_DATA_PRIVATE_H

#include "send_data_types.h"

#ifdef __cplusplus
extern "C" {
#endif
namespace EdgeAppLib {

typedef struct {
  const char *key;
  JSON_Value *value;
} InfElem;

/**
 * @brief Append Output Tensor to output_tensor_vec
 * @param key A pointer to AI model bundle ID
 * @param value A pointer to Output Tensor
 * @return A result of SendDataAppendOutputTensor
 */
EdgeAppLibSendDataResult SendDataAppendOutputTensor(const char *key,
                                                    JSON_Value *value);

uint64_t SendDataConvertTimeToNanoseconds(const char *datetime);

#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib

#endif /* AITRIOS_DATA_EXPORT_PRIVATE_H */
