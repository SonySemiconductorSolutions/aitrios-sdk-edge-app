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
 * @file process_format.hpp
 * @details This file contains the declarations of the process_format functions.
 */

#ifndef PROCESS_FORMAT_H
#define PROCESS_FORMAT_H

#include <stdint.h>

#include "memory_manager.hpp"
#include "parson.h"
#include "send_data_types.h"

typedef enum {
  kProcessFormatResultOk,           /**< Operation succeeded. */
  kProcessFormatResultFailure,      /**< Operation failed. */
  kProcessFormatResultInvalidParam, /**< Invalid parameter. */
  kProcessFormatResultMemoryError,  /**< Memory error occurred. */
  kProcessFormatResultOther
} ProcessFormatResult;

typedef enum {
  kProcessFormatImageTypeRaw,  /**< Image type is RAW. */
  kProcessFormatImageTypeJpeg, /**< Image type is JPEG. */
  kProcessFormatImageTypeBmp,  /**< Image type is BMP. (not implemented) */
  kProcessFormatImageTypeOther /**< Image type is other. (not implemented) */
} ProcessFormatImageType;

/**
 * @brief Format the data to be Output Tensor
 * @param in_data Pointer of output tensor buffer.
 * @param in_size Pointer of output tensor buffer size.
 * @param datatype The type of the data to upload.
 * @param timestamp The timestamp of the processed frame in nanoseconds.
 * @param json_buffer Pointer of JSON buffer.
 * @param buffer_size Size of JSON buffer.
 * @return Result of the formating operation for meta data.
 */
ProcessFormatResult ProcessFormatMeta(void *in_data, uint32_t in_size,
                                      EdgeAppLibSendDataType datatype,
                                      uint64_t timestamp, char *json_buffer,
                                      size_t buffer_size);

/**
 * @brief Encode the data to be Input Tensor
 * @param in_data Pointer or handle for input tensor buffer.
 * @param in_size Size of input tensor buffer.
 * @param image_property Pointer to the image property structure.
 * @param datatype The type of the data to upload.
 * @param timestamp The timestamp of the processed frame in nanoseconds.
 * @param image Pointer or handle of encoded input tensor buffer.
 * @param image_size Size of encoded input tensor buffer.
 * @return Result of the formating operation for image data.
 */
ProcessFormatResult ProcessFormatInput(MemoryRef in_data, uint32_t in_size,
                                       ProcessFormatImageType datatype,
                                       EdgeAppLibImageProperty *image_property,
                                       uint64_t timestamp, void **image,
                                       int32_t *image_size);

#endif /* PROCESS_FORMAT_H */
