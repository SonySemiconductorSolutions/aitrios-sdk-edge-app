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
#ifndef _DEVICE_H
#define _DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// This is a definition of an enumeration type for defining the input data
// format.
typedef enum {
  kJpegInputRgbPlanar_8,  // RGB Planar 8bit.
  kJpegInputRgbPacked_8,  // RGB Packed 8bit.
  kJpegInputBgrPacked_8,  // BGR Packed 8bit.
  kJpegInputGray_8,       // GrayScale 8bit.
  kJpegInputYuv_8         // YUV(NV12) 8bit.
} EsfCodecJpegInputFormat;

// The structure defines an output buffer.
typedef struct {
  // The starting address of the JPEG image output destination. Setting zero is
  // not allowed.
  uint64_t output_adr_handle;

  // Output buffer size.
  int32_t output_buf_size;
} EsfCodecJpegOutputBuf;

// The struct defines the parameters for JPEG encoding.
typedef struct {
  // The starting address of the input data. Setting zero is not allowed.
  uint64_t input_adr_handle;

  // Output buffer information.
  EsfCodecJpegOutputBuf out_buf;

  // Input data format.
  EsfCodecJpegInputFormat input_fmt;

  // Horizontal size of the input image (in pixels). A setting of 0 or less is
  // not allowed.
  int32_t width;

  // Vertical size of the input image (in pixels). A setting of 0 or less is
  // not allowed.
  int32_t height;

  // The stride (in bytes) of the input image, including padding, must not be
  // set to a value smaller than the number of bytes in one row of the input
  // image.
  int32_t stride;

  // Image quality (0: low quality ~ 100: high quality).
  int32_t quality;
} EsfCodecJpegEncParam;

typedef struct {
  EsfCodecJpegInputFormat input_fmt;  // Input data format.
  int32_t width;   // Horizontal size of the input image (in pixels). A setting
                   // of 0 or less is not allowed.
  int32_t height;  // Vertical size of the input image (in pixels). A setting of
                   // 0 or less is not allowed.
  int32_t stride;  // The stride (in bytes) of the input image, including
                   // padding, must not be set to a value smaller than the
                   // number of bytes in one row of the input image.
  int32_t quality;  // Image quality (0: low quality ~ 100: high quality).
} EsfCodecJpegInfo;

typedef enum {
  kJpegSuccess,               // No errors.
  kJpegParamError,            // Parameter error.
  kJpegOssInternalError,      // Internal error in OSS.
  kJpegMemAllocError,         // Memory allocation error.
  kJpegOtherError,            // Other errors.
  kJpegOutputBufferFullError  // Output buffer full error.
} EsfCodecJpegError;

typedef enum {
  kEsfDeviceIdResultOk,
  kEsfDeviceIdResultParamError,
  kEsfDeviceIdResultInternalError,
  kEsfDeviceIdResultEmptyData
} EsfDeviceIdResult;

typedef uint32_t EsfMemoryManagerHandle;

#define WASM_BINDING_DEVICEID_MAX_SIZE (41)

EsfCodecJpegError EsfCodecEncodeJpeg(const EsfCodecJpegEncParam *enc_param,
                                     int32_t *jpeg_size);
EsfDeviceIdResult EsfSystemGetDeviceID(char *data);
EsfCodecJpegError EsfCodecJpegEncodeHandle(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle *output_file_handle, const EsfCodecJpegInfo *info,
    int32_t *jpeg_size);
EsfCodecJpegError EsfCodecJpegEncodeRelease(
    EsfMemoryManagerHandle release_file_handle);
#ifdef __cplusplus
}
#endif

#endif
