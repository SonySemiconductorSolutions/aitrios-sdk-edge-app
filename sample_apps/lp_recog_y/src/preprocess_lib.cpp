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

#include "preprocess_lib.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include "log.h"
extern uint32_t lpr_input_tensor_width;
extern uint32_t lpr_input_tensor_height;

namespace PreprocessLib {

EdgeAppCoreResult normalizePreprocess(
    const void *input_data, EdgeAppLibImageProperty input_property,
    void **output_data, EdgeAppLibImageProperty *output_property) {
  // Allocate output buffer for float data
  size_t pixel_count = input_property.stride_bytes * input_property.height;
  output_property->height = input_property.height;
  output_property->width = input_property.width;
  output_property->stride_bytes =
      input_property.stride_bytes * sizeof(float);  // float
  snprintf(output_property->pixel_format, sizeof(output_property->pixel_format),
           "FLOAT32");
  *output_data = malloc(pixel_count * sizeof(float));  // Assuming RGB input
  if (*output_data == nullptr) {
    LOG_ERR("Failed to allocate memory for preprocessing.");
    return EdgeAppCoreResultFailure;
  }

  // Copy and normalize pixel values from [0,255] to [0,1]
  const uint8_t *src = static_cast<const uint8_t *>(input_data);
  float *dst = static_cast<float *>(*output_data);

  for (size_t i = 0; i < pixel_count; ++i) {
    dst[i] = src[i] / 255.0f;
  }

  return EdgeAppCoreResultSuccess;
}

EdgeAppCoreResult grayscalePreprocess(const void *input_data,
                                      EdgeAppLibImageProperty input_property,
                                      EdgeAppCore::Tensor *output_tensor) {
  LOG_DBG("Grayscale Preprocess: input width=%u, height=%u",
          input_property.width, input_property.height);
  uint32_t height = lpr_input_tensor_height;
  uint32_t width = lpr_input_tensor_width;
  LOG_INFO("Target size: width=%u, height=%u", width, height);
  cv::Size target_size(width, height);
  cv::Mat src(input_property.height, input_property.width, CV_8UC3,
              const_cast<void *>(input_data), input_property.width * 3);
  if (src.empty()) {
    LOG_ERR("Input data is empty.");
    return EdgeAppCoreResultFailure;
  }
  cv::Mat dst;
  cv::Mat gray;

  if (!height || !width) {
    LOG_INFO("Skip resizing.");
    height = input_property.height;
    width = input_property.width;
    try {
      cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);  // CV_8UC1
    } catch (const cv::Exception &e) {
      LOG_ERR("Convert to gray failure due to: %s", e.what());
      return EdgeAppCoreResultFailure;
    }
  } else {
    try {
      cv::resize(src, dst, target_size, 0, 0, cv::INTER_LINEAR);
    } catch (const cv::Exception &e) {
      LOG_ERR("Resize failure due to: %s", e.what());
      return EdgeAppCoreResultFailure;
    }
    try {
      cv::cvtColor(dst, gray, cv::COLOR_BGR2GRAY);  // CV_8UC1
    } catch (const cv::Exception &e) {
      LOG_ERR("Convert to gray failure due to: %s", e.what());
      return EdgeAppCoreResultFailure;
    }
  }

  // Assume RGB input, output will be 1/3 the size
  size_t pixel_count = height * width;  // RGB pixels
  int8_t *output_data = (int8_t *)malloc(pixel_count);
  if (output_data == nullptr) {
    LOG_ERR("Failed to allocate memory for preprocessing.");
    return EdgeAppCoreResultFailure;
  }

  memcpy(output_data, gray.data, pixel_count);

  // Fill output tensor
  output_tensor->data = output_data;
  output_tensor->size = pixel_count * sizeof(int8_t);
  output_tensor->type = EdgeAppCore::TensorDataType::TensorTypeUInt8;
  output_tensor->shape_info.ndim = 4;
  output_tensor->shape_info.dims[0] = 1;
  output_tensor->shape_info.dims[1] = height;
  output_tensor->shape_info.dims[2] = width;
  output_tensor->shape_info.dims[3] = 1;  // Grayscale
  output_tensor->memory_owner = TensorMemoryOwner::App;

  return EdgeAppCoreResultSuccess;
}

}  // namespace PreprocessLib
