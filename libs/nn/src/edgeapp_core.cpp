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

/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "edgeapp_core.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For python

#ifndef __wasm__
#include <functional>
#include <mutex>
#include <tuple>
#include <utility>
#endif

#include <string>

#include "data_export.h"
#include "draw.h"
#include "log.h"
#include "memory_manager.hpp"
#include "nn.h"
#include "receive_data.h"
#include "send_data.h"
#include "user_bridge_c.h"

#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"
#define PORTNAME_RAW "full"
#define MAX_PATH_LEN 256

using namespace EdgeAppLib;

namespace EdgeAppCore {
static uint32_t model_count = 0;  // Count of loaded models

static bool IsRealFilename(const char *filename, const char *real_filename) {
  if (strncmp(filename, real_filename, strlen(real_filename))) {
    return false;
  }
  if (strlen(filename) == strlen(real_filename)) {
    return true;
  } else if (strlen(filename) > strlen(real_filename)) {
    if (*(filename + strlen(real_filename)) == '.') {
      return true;
    }
  }
  return false;
}

static char *FindFilenameByRealFilename(const char *dir,
                                        const char *real_filename) {
  DIR *dir_p = opendir(dir);
  if (!dir_p) {
    LOG_ERR("Open directory failed.");
    return nullptr;
  }

  struct dirent *entry;
  char *filename = nullptr;
  while ((entry = readdir(dir_p)) != NULL) {
    if (entry->d_type == DT_REG &&
        IsRealFilename(entry->d_name, real_filename)) {
      filename = (char *)malloc(strlen(entry->d_name) + 1);
      snprintf(filename, strlen(entry->d_name) + 1, "%s", entry->d_name);
      break;
    }
  }

  closedir(dir_p);
  return filename;
}

EdgeAppCoreResult LoadModel(EdgeAppCoreModelInfo model, EdgeAppCoreCtx &ctx,
                            EdgeAppCoreCtx *shared_ctx) {
  if (model.model_name == nullptr || model.model_name[0] == '\0') {
    LOG_ERR("LoadModel: model_name is invalid.");
    return EdgeAppCoreResultInvalidParam;
  }
  if (model.target > edge_imx500) {
    LOG_ERR("LoadModel: model.target is invalid.");
    return EdgeAppCoreResultInvalidParam;
  }
  ctx.target = model.target;
  ctx.temp_input.buffer = nullptr;
  ctx.temp_input.size = 0;
  ctx.temp_input.width = 0;
  ctx.temp_input.height = 0;
  ctx.temp_input.timestamp = 0;
  ctx.mean_values = model.mean_values;
  ctx.norm_values = model.norm_values;

  if (model.target == edge_imx500) {
    ctx.sensor_core =
        (EdgeAppLibSensorCore *)xmalloc(sizeof(EdgeAppLibSensorCore));
    if (!ctx.sensor_core || SensorCoreInit(ctx.sensor_core) != 0) {
      return EdgeAppCoreResultFailure;
    }

    // sensor_stream
    ctx.sensor_stream =
        (EdgeAppLibSensorStream *)xmalloc(sizeof(EdgeAppLibSensorStream));
    if (ctx.sensor_stream == nullptr ||
        SensorCoreOpenStream(*ctx.sensor_core,
                             AITRIOS_SENSOR_STREAM_KEY_DEFAULT,
                             ctx.sensor_stream) != 0) {
      return EdgeAppCoreResultFailure;
    }
    struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle = {};
    if (snprintf(ai_model_bundle.ai_model_bundle_id, AI_MODEL_BUNDLE_ID_SIZE,
                 "%s", model.model_name) >= AI_MODEL_BUNDLE_ID_SIZE) {
      LOG_WARN("AI model bundle ID exceeds size limit");
    }

    if (SensorStreamSetProperty(
            *ctx.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
            &ai_model_bundle, sizeof(ai_model_bundle)) < 0) {
      LOG_ERR("Error while setting desired AI model bundle ID");
      return EdgeAppCoreResultFailure;
    }
    if (SensorStart(*ctx.sensor_stream) != 0) {
      return EdgeAppCoreResultFailure;
    }
  } else {
    EdgeAppLibGraph g;
    const char *path = EdgeAppLibReceiveDataStorePath();
    char model_path[MAX_PATH_LEN] = {0};
    char *model_file = FindFilenameByRealFilename(path, model.model_name);
    if (model_file) {
      if (snprintf(model_path, MAX_PATH_LEN, "%s/%s", path, model_file) >=
          MAX_PATH_LEN) {
        LOG_WARN("AI model file absolute path exceeds size limit");
      }
      free(model_file);
    } else {
      if (snprintf(model_path, MAX_PATH_LEN, "%s/%s", path, model.model_name) >=
          MAX_PATH_LEN) {
        LOG_WARN("AI model file absolute path exceeds size limit");
      }
    }
    if (EdgeAppLib::LoadModel((const char *)model_path, &g,
                              (EdgeAppLibExecutionTarget)model.target) != 0) {
      LOG_ERR("Failed to load model: %s", model_path);
      return EdgeAppCoreResultFailure;
    }

    ctx.graph_ctx =
        (EdgeAppLibGraphContext *)xmalloc(sizeof(EdgeAppLibGraphContext));
    if (!ctx.graph_ctx) {
      return EdgeAppCoreResultFailure;
    }
    if (InitContext(g, ctx.graph_ctx) != 0) {
      LOG_ERR("Failed to initialize graph execution context for model: %s",
              model.model_name);
      free(ctx.graph_ctx);
      return EdgeAppCoreResultFailure;
    }
  }
  ctx.model_idx = model_count++;
  return EdgeAppCoreResultSuccess;
}

AutoFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                  EdgeAppLibSensorFrame frame,
                  EdgeAppLibSensorImageCropProperty &roi) {
  // 1If no frame provided, acquire it once (first call)

  if (shared_ctx == nullptr || shared_ctx->sensor_stream == nullptr) {
    LOG_ERR("Shared context or sensor stream is null.");
    return AutoFrame(nullptr, 0);
  }
  if (frame == 0 && ctx.target == edge_imx500) {
    int8_t ret = SensorGetFrame(*shared_ctx->sensor_stream, &frame, -1);
    if (ret < 0) {
      LOG_ERR("SensorGetFrame failed: ret=%d", ret);
    }
  }

  // Model-specific processing
  if (ctx.target == edge_imx500) {
    // For IMX500: just set the ROI on the sensor stream
    int32_t ret = SensorStreamSetProperty(
        *ctx.sensor_stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &roi,
        sizeof(EdgeAppLibSensorImageCropProperty));
    if (ret != 0) {
      LOG_ERR("SensorStreamSetProperty failed with %" PRId32 ".", ret);
      EdgeAppLibLogSensorError();
    }
  } else {
    // Clean up any previous temporary input buffer
    if (ctx.temp_input.buffer) {
      free(ctx.temp_input.buffer);
      ctx.temp_input.buffer = nullptr;
    }

    // Get the RAW_IMAGE channel
    EdgeAppLibSensorChannel channel;
    int32_t ret = SensorFrameGetChannelFromChannelId(
        frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_RAW_IMAGE, &channel);
    if (ret < 0) {
      LOG_WARN("SensorFrameGetChannelFromChannelId failed: ret=%" PRId32 ".",
               ret);
      return AutoFrame(shared_ctx->sensor_stream, frame);  // Return anyway
    }

    // Get the raw data
    struct EdgeAppLibSensorRawData data = {0};
    ret = SensorChannelGetRawData(channel, &data);
    if (ret != 0) {
      LOG_ERR("SensorChannelGetRawData failed with %" PRId32 ".", ret);
      EdgeAppLibLogSensorError();
    }
    LOG_DBG(
        "input_raw_data.address:%p\ninput_raw_data.size:%zu\ninput_raw_data."
        "timestamp:%llu\ninput_raw_data.type:%s",
        data.address, data.size, data.timestamp, data.type);
    EdgeAppLibDrawBuffer src{};
    EdgeAppLibSensorImageProperty image_property;
    ret = SensorChannelGetProperty(channel, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                                   &image_property, sizeof(image_property));
    if (ret != 0) {
      LOG_ERR("SensorChannelGetProperty failed with %" PRId32 ".", ret);
      EdgeAppLibLogSensorError();
    }
    src.width = image_property.width;
    src.height = image_property.height;
    src.stride_byte = image_property.stride_bytes;
    if (strncmp(image_property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB24,
                sizeof(AITRIOS_SENSOR_PIXEL_FORMAT_RGB24)) == 0) {
      src.format = AITRIOS_DRAW_FORMAT_RGB8;
    } else {
      LOG_ERR("Unsupported pixel format: %s", image_property.pixel_format);
      return AutoFrame(
          shared_ctx->sensor_stream,
          frame);  // Not return error until RGB24 is supported by T4 senscord
    }
    src.size = data.size;
    src.address = data.address;
    LOG_DBG("src.address: %p, src.size: %zu, src.width: %d, src.height: %d",
            src.address, src.size, src.width, src.height);

    // Adjust ROI based on actual input image size
    EdgeAppLibSensorImageProperty it_image_property;
    ret = SensorFrameGetChannelFromChannelId(
        frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
    if (ret < 0) {
      LOG_WARN("Failed to get INPUT_IMAGE channel: ret=%d.", ret);
      return AutoFrame(shared_ctx->sensor_stream, frame);
    }
    ret =
        SensorChannelGetProperty(channel, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                                 &it_image_property, sizeof(it_image_property));
    if (ret != 0) {
      LOG_ERR("SensorChannelGetProperty failed with %" PRId32 ".", ret);
      EdgeAppLibLogSensorError();
    }
    if (roi.width > it_image_property.width)
      roi.width = it_image_property.width;
    if (roi.height > it_image_property.height)
      roi.height = it_image_property.height;
    if (it_image_property.width != 0) {
      roi.width = roi.width * image_property.width / it_image_property.width;
      roi.left = roi.left * image_property.width / it_image_property.width;
    }
    if (it_image_property.height != 0) {
      roi.height =
          roi.height * image_property.height / it_image_property.height;
      roi.top = roi.top * image_property.height / it_image_property.height;
    }

    // Crop the image if needed
    EdgeAppLibDrawBuffer dst{};
    size_t dst_size = roi.width * roi.height * 3;
    if (roi.width != 0 && roi.height != 0) {
      dst.width = roi.width;
      dst.height = roi.height;
      dst.format = AITRIOS_DRAW_FORMAT_RGB8;
      dst.stride_byte = dst.width * 3;  // RGB format, 3 bytes per pixel
      dst.size = dst_size;
      dst.address = (uint8_t *)malloc(dst_size);
      if (dst.address == nullptr) {
        LOG_ERR("Failed to allocate memory for cropped image.");
        return AutoFrame(shared_ctx->sensor_stream, 0);  // Return anyway
      }
      CropRectangle(&src, &dst, roi.left, roi.top, roi.left + roi.width - 1,
                    roi.top + roi.height - 1);
      ctx.temp_input.memory_owner = TensorMemoryOwner::App;
      ctx.temp_input.size = dst.size;
      ctx.temp_input.width = dst.width;
      ctx.temp_input.height = dst.height;
      ctx.temp_input.buffer = (uint8_t *)dst.address;
      ctx.temp_input.timestamp = data.timestamp;
    } else {
      // fallback: use the full frame
      ctx.temp_input.buffer = static_cast<uint8_t *>(src.address);
      ctx.temp_input.size = src.size;
      ctx.temp_input.width = src.width;
      ctx.temp_input.height = src.height;
      ctx.temp_input.timestamp = data.timestamp;
      roi.height = src.height;
      roi.width = src.width;
      ctx.temp_input.memory_owner = TensorMemoryOwner::Sensor;
    }

    // Set input tensor and run inference
    if (ctx.graph_ctx != nullptr) {
      // Give NHWC format based dimensions for unknown wasi-nn backends
      uint32_t dims[4] = {1, roi.height, roi.width, 3};
      if (SetInput(*(ctx.graph_ctx), ctx.temp_input.buffer, dims,
                   ctx.mean_values->data(), ctx.mean_values->size(),
                   ctx.norm_values->data(), ctx.norm_values->size()) != 0) {
        LOG_ERR("Failed to set input tensor");
        frame = 0;  // Return empty frame
      }
      if (Compute(*(ctx.graph_ctx)) != 0) {
        LOG_ERR("Failed to compute graph");
        frame = 0;  // Clear frame to indicate failure
      }
    }
  }

  // Return AutoFrame, which ensures frame is released on scope exit
  return AutoFrame(shared_ctx->sensor_stream, frame);
}

Tensor GetOutput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                 uint32_t tensor_num) {
  Tensor output_tensor{};
  LOG_WARN("GetOutput called for target: %d", ctx.target);
  // get output from the sensor
  if (ctx.target == edge_imx500) {
    EdgeAppLibSensorChannel channel;
    int32_t ret = SensorFrameGetChannelFromChannelId(
        frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel);
    if (ret < 0) {
      LOG_WARN("SensorFrameGetChannelFromChannelId failed: ret=%d.", ret);
      return {};
    }

    struct EdgeAppLibSensorRawData data = {0};
    if (SensorChannelGetRawData(channel, &data) < 0) {
      LOG_WARN("SensorChannelGetRawData failed.");
      return {};
    }
    LOG_INFO(
        "output_raw_data.address:%p\noutput_raw_data.size:%zu\noutput_raw_"
        "data."
        "timestamp:%llu\noutput_raw_data.type:%s",
        data.address, data.size, data.timestamp, data.type);

    EdgeAppLibSensorTensorShapesProperty tensor_shape{};
    ret = SensorChannelGetProperty(channel,
                                   AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY,
                                   &tensor_shape, sizeof(tensor_shape));
    if (ret != 0) {
      LOG_ERR("SensorChannelGetProperty failed: %d", ret);
      return {};
    }

    // Parse shape info
    std::vector<std::vector<uint32_t>> shapes;
    uint32_t index = 0;
    while (index < AITRIOS_SENSOR_SHAPES_ARRAY_LENGTH) {
      uint32_t dimension = tensor_shape.shapes_array[index++];
      if (dimension == 0) break;

      shapes.emplace_back();
      for (uint32_t j = 0; j < dimension; ++j) {
        shapes.back().push_back(tensor_shape.shapes_array[index++]);
      }
    }

    output_tensor.data = data.address;
    output_tensor.size = data.size;
    output_tensor.timestamp = data.timestamp;
    output_tensor.type = TensorDataType::TensorTypeFloat32;
    output_tensor.shape_info.ndim = tensor_shape.tensor_count;

    output_tensor.shape_info.ndim = 0;
    for (const auto &shape : shapes) {
      if (output_tensor.shape_info.ndim >= tensor_num) {
        LOG_WARN("Too many dimensions, truncating.");
        break;
      }
      uint32_t dim = 1;
      for (auto s : shape) {
        dim *= s;
      }
      output_tensor.shape_info.dims[output_tensor.shape_info.ndim++] = dim;
    }

  } else {
    // CPU/NPU: use graph_ctx to get output
    if (ctx.graph_ctx == nullptr) {
      LOG_ERR("Graph execution context is not initialized.");
      return {};
    }

    float *base =
        static_cast<float *>(malloc(MAX_OUTPUT_TENSORS_SIZE * sizeof(float)));
    if (!base) {
      LOG_ERR("malloc failed");
      return {};
    }
    float *write_ptr = base;
    uint32_t total_element_size = 0;

    output_tensor.data = base;
    output_tensor.size = MAX_OUTPUT_TENSORS_SIZE * sizeof(float);
    output_tensor.shape_info.ndim = 0;
    output_tensor.type = TensorDataType::TensorTypeFloat32;
    // Use the input tensor's timestamp for CPU/NPU models
    output_tensor.timestamp = ctx.temp_input.timestamp;

    for (size_t j = 0; j < tensor_num; ++j) {
      uint32_t remaining_elements_size =
          MAX_OUTPUT_TENSORS_SIZE - total_element_size;
      uint32_t outsize = remaining_elements_size;

      if (EdgeAppLib::GetOutput(*ctx.graph_ctx, j, write_ptr, &outsize) != 0) {
        // LOG_WARN("Failed to get output tensor index %zu", j);
        continue;
      }

      output_tensor.shape_info.dims[output_tensor.shape_info.ndim++] =
          outsize / sizeof(float);
      write_ptr += outsize / sizeof(float);
      total_element_size += outsize;
    }

    void *tmp = realloc(base, total_element_size);
    if (tmp == nullptr) {
      free(base);
      LOG_ERR("realloc failed");
      return {};
    }
    output_tensor.data = tmp;
    output_tensor.size = total_element_size;

    if (output_tensor.shape_info.ndim == 0) {
      LOG_WARN("No valid output tensors found.");
      free(tmp);
      output_tensor.data = nullptr;
      output_tensor.size = 0;
    }
  }

  // Log shape
  std::string shape_log = "Output tensor shape: [ ";
  for (uint32_t i = 0; i < output_tensor.shape_info.ndim; ++i) {
    shape_log += std::to_string(output_tensor.shape_info.dims[i]) + " ";
  }
  shape_log += "]";
  LOG_INFO("%s", shape_log.c_str());

  return output_tensor;
}

Tensor GetInput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame) {
  if (frame == 0) {
    LOG_ERR("Frame or graph execution context is not initialized.");
    return {};
  }

  Tensor input_tensor{};

  if (ctx.target == edge_imx500) {
    LOG_DBG("GetInput called for imx500 model");

    EdgeAppLibSensorChannel channel;
    int32_t ret = SensorFrameGetChannelFromChannelId(
        frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
    if (ret < 0) {
      LOG_WARN("SensorFrameGetChannelFromChannelId failed: ret=%d.", ret);
      return {};
    }

    struct EdgeAppLibSensorRawData data = {0};
    if (SensorChannelGetRawData(channel, &data) < 0) {
      LOG_WARN("SensorChannelGetRawData failed.");
      return {};
    }

    EdgeAppLibSensorImageProperty property = {};
    if (SensorChannelGetProperty(channel, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                                 &property, sizeof(property)) != 0) {
      LOG_ERR("SensorChannelGetProperty failed for input image");
      free(data.address);
      return {};
    }

    input_tensor.data = data.address;
    input_tensor.size = data.size;
    input_tensor.timestamp = data.timestamp;
    input_tensor.type = TensorDataType::TensorTypeUInt8;
    input_tensor.shape_info.ndim = 4;
    input_tensor.shape_info.dims[0] = 1;
    input_tensor.shape_info.dims[1] = property.height;
    input_tensor.shape_info.dims[2] = property.width;
    input_tensor.shape_info.dims[3] = 3;  // RGB
    input_tensor.memory_owner = TensorMemoryOwner::Sensor;
    snprintf(input_tensor.name, sizeof(input_tensor.name), "imx500_input");

    LOG_DBG("Parsed input tensor:  [ %d ][ %d ][ %d ][ %d ]",
            input_tensor.shape_info.dims[0], input_tensor.shape_info.dims[1],
            input_tensor.shape_info.dims[2], input_tensor.shape_info.dims[3]);

  } else {
    // CPU/NPU
    const auto &temp = ctx.temp_input;
    if (temp.buffer && temp.width > 0 && temp.height > 0) {
      input_tensor.data = temp.buffer;
      input_tensor.size = temp.size;
      input_tensor.timestamp = temp.timestamp;
      input_tensor.type = TensorDataType::TensorTypeUInt8;
      input_tensor.shape_info.ndim = 4;
      input_tensor.shape_info.dims[0] = 1;
      input_tensor.shape_info.dims[1] = temp.height;
      input_tensor.shape_info.dims[2] = temp.width;
      input_tensor.shape_info.dims[3] = 3;
      input_tensor.memory_owner = temp.memory_owner;
      snprintf(input_tensor.name, sizeof(input_tensor.name), "wasi_nn_input_%d",
               ctx.model_idx);

      LOG_DBG("Parsed input tensor:  [ %d ][ %d ][ %d ][ %d ]",
              input_tensor.shape_info.dims[0], input_tensor.shape_info.dims[1],
              input_tensor.shape_info.dims[2], input_tensor.shape_info.dims[3]);
      ctx.temp_input.buffer = nullptr;  // Clear buffer to avoid double free
      ctx.temp_input.size = 0;          // Reset size
    }
  }

  return input_tensor;
}

EdgeAppCoreResult UnloadModel(EdgeAppCoreCtx &ctx) {
  if (ctx.target == edge_imx500 && ctx.sensor_stream != 0) {
    SensorStop(*ctx.sensor_stream);
    SensorCoreCloseStream(*ctx.sensor_core, *ctx.sensor_stream);
    SensorCoreExit(*ctx.sensor_core);

    free(ctx.sensor_stream);
    ctx.sensor_stream = nullptr;
    free(ctx.sensor_core);
    ctx.sensor_core = nullptr;
  }

  // For CPU/NPU models, free the temporary input buffer
  if (ctx.temp_input.buffer != nullptr && ctx.target != edge_imx500) {
    free(ctx.temp_input.buffer);
    ctx.temp_input.buffer = nullptr;
  }

  if (ctx.graph_ctx != nullptr && ctx.graph_ctx != 0) {
    free(ctx.graph_ctx);
    ctx.graph_ctx = nullptr;
  }
  model_count--;
  return EdgeAppCoreResultSuccess;
}

EdgeAppCoreResult SendInference(void *data, size_t datalen,
                                EdgeAppLibSendDataType datatype,
                                uint64_t timestamp) {
  EdgeAppLibSendDataResult result =
      EdgeAppLib::SendDataSyncMeta(data, datalen, datatype, timestamp, -1);
  // free(data);
  return result == EdgeAppLibSendDataResultSuccess ? EdgeAppCoreResultSuccess
                                                   : EdgeAppCoreResultFailure;
}
/**
 * @brief Sends the Input Tensor to the cloud asynchronously.
 *
 * This function sends the input tensor data from the provided frame to the
 * cloud. It returns a future object representing the asynchronous operation.
 *
 * By returning a future, this function allows for non-blocking execution.
 * The caller can await this future after sending the output tensor, ensuring
 * that both awaits are done consecutively without blocking the sending of the
 * rest of the data.
 *
 * @param Tensor Pointer to the input tensor to be sent.
 * @return A future representing the asynchronous operation of sending the input
 * tensor.
 */

EdgeAppCoreResult SendInputTensor(Tensor *input_tensor) {
  LOG_TRACE("Inside sendInputTensor.");
  if (input_tensor == nullptr || input_tensor->data == nullptr) {
    LOG_ERR("Invalid input tensor data.");
    return EdgeAppCoreResultInvalidParam;
  }

  EdgeAppLibImageProperty image_property = {};
  image_property.width = input_tensor->shape_info.dims[2];
  image_property.height = input_tensor->shape_info.dims[1];
  image_property.stride_bytes = input_tensor->shape_info.dims[2] * 3;  // RGB
  snprintf(image_property.pixel_format, sizeof(image_property.pixel_format),
           "%s", AITRIOS_SENSOR_PIXEL_FORMAT_RGB24);

  EdgeAppLibSendDataResult ret = SendDataSyncImage(
      input_tensor->data, input_tensor->size,
      (EdgeAppLibImageProperty *)&image_property, input_tensor->timestamp, -1);
  if (input_tensor->memory_owner == TensorMemoryOwner::App) {
    // Free the input tensor data if it was allocated by the app
    free(input_tensor->data);
    input_tensor->data = nullptr;
  }
  return (ret == EdgeAppLibSendDataResultSuccess) ? EdgeAppCoreResultSuccess
                                                  : EdgeAppCoreResultFailure;
}

}  // namespace EdgeAppCore
