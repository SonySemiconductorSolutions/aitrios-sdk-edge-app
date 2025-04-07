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
#include "mock_device.hpp"

#include "device.h"
#include "memory_manager.hpp"
#include "string.h"

static EsfDeviceIdResult EsfSystemGetDeviceIDSuccess = kEsfDeviceIdResultOk;
static EsfMemoryManagerResult EsfMemoryManagerPreadSuccess =
    kEsfMemoryManagerResultSuccess;
static EsfCodecJpegError EsfCodecJpegEncodeSuccess = kJpegSuccess;
static EsfCodecJpegError EsfCodecJpegEncodeReleaseSuccess = kJpegSuccess;

void setEsfSystemGetDeviceIDFail() {
  EsfSystemGetDeviceIDSuccess = kEsfDeviceIdResultParamError;
}

void resetEsfSystemGetDeviceIDSuccess() {
  EsfSystemGetDeviceIDSuccess = kEsfDeviceIdResultOk;
}

void setEsfCodecJpegEncodeFail() {
  EsfCodecJpegEncodeSuccess = kJpegParamError;
}

void resetEsfCodecJpegEncodeSuccess() {
  EsfCodecJpegEncodeSuccess = kJpegSuccess;
}

void setEsfCodecJpegEncodeReleaseFail() {
  EsfCodecJpegEncodeReleaseSuccess = kJpegParamError;
}

void resetEsfCodecJpegEncodeReleaseSuccess() {
  EsfCodecJpegEncodeReleaseSuccess = kJpegSuccess;
}

EsfDeviceIdResult EsfSystemGetDeviceID(char *data) {
  if (data == NULL) return kEsfDeviceIdResultParamError;
  if (EsfSystemGetDeviceIDSuccess != kEsfDeviceIdResultOk)
    return EsfSystemGetDeviceIDSuccess;
  const char *subject_name = "test_id";
  strncpy(data, subject_name, strlen(subject_name));
  data[strlen(subject_name)] = '\0';
  return kEsfDeviceIdResultOk;
}
void setEsfMemoryManagerPreadFail() {
  EsfMemoryManagerPreadSuccess = kEsfMemoryManagerResultParamError;
}

void resetEsfMemoryManagerPreadSuccess() {
  EsfMemoryManagerPreadSuccess = kEsfMemoryManagerResultSuccess;
}

EsfMemoryManagerResult EsfMemoryManagerPread(EsfMemoryManagerHandle handle,
                                             void *buffer, size_t size,
                                             uint64_t offset,
                                             size_t *bytes_read) {
  memset(buffer, 0xAA, size);  // Fill buffer with mock data
  *bytes_read = size;
  return EsfMemoryManagerPreadSuccess;
}

EsfCodecJpegError EsfCodecEncodeJpeg(const EsfCodecJpegEncParam *enc_param,
                                     int32_t *jpeg_size) {
  if (enc_param == NULL || jpeg_size == NULL) return kJpegParamError;
  if (enc_param->input_adr_handle == 0 ||
      enc_param->out_buf.output_adr_handle == 0)
    return kJpegParamError;
  *jpeg_size = enc_param->out_buf.output_buf_size;
  return EsfCodecJpegEncodeSuccess;
}

EsfCodecJpegError EsfCodecJpegEncodeHandle(
    EsfMemoryManagerHandle input_file_handle,
    EsfMemoryManagerHandle *output_file_handle, const EsfCodecJpegInfo *info,
    int32_t *jpeg_size) {
  if (!input_file_handle || !output_file_handle || !info || !jpeg_size) {
    return kJpegParamError;
  }
  *output_file_handle = (EsfMemoryManagerHandle)(1);  // Mock handle
  *jpeg_size =
      static_cast<int32_t>(info->width * info->height / 2);  // Mock size
  return EsfCodecJpegEncodeSuccess;
}

EsfCodecJpegError EsfCodecJpegEncodeRelease(
    EsfMemoryManagerHandle release_file_handle) {
  if (!release_file_handle) {
    return kJpegParamError;
  }
  return EsfCodecJpegEncodeReleaseSuccess;
}
