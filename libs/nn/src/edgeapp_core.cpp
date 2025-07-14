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
#include <errno.h>
#include <fcntl.h>
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
#include "send_data.h"
#include "user_bridge_c.h"

#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"
#define PORTNAME_RAW "full"

using namespace EdgeAppLib;

namespace EdgeAppCore {
static uint32_t model_count = 0;  // Count of loaded models
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
      SensorCoreExit(*ctx.sensor_core);
      return EdgeAppCoreResultFailure;
    }
    struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle = {};

    if (SensorStreamSetProperty(
            *ctx.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
            &ai_model_bundle, sizeof(ai_model_bundle)) < 0) {
      LOG_ERR("Error while setting desired AI model bundle ID");
      SensorCoreCloseStream(*ctx.sensor_core, *ctx.sensor_stream);
      SensorCoreExit(*ctx.sensor_core);
      return EdgeAppCoreResultFailure;
    }
    if (SensorStart(*ctx.sensor_stream) != 0) {
      SensorCoreCloseStream(*ctx.sensor_core, *ctx.sensor_stream);
      SensorCoreExit(*ctx.sensor_core);
      return EdgeAppCoreResultFailure;
    }
  } else {
    graph g;
    if (EdgeAppLib::LoadModel(model.model_name, &g,
                              ToExecutionTarget(model.target)) != 0) {
      LOG_ERR("Failed to load model: %s", model.model_name);
      return EdgeAppCoreResultFailure;
    }

    ctx.graph_ctx =
        (graph_execution_context *)xmalloc(sizeof(graph_execution_context));
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
    SensorStreamSetProperty(*ctx.sensor_stream,
                            AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &roi,
                            sizeof(EdgeAppLibSensorImageCropProperty));
  } else {
    // Clean up any previous temporary input buffer
    if (ctx.temp_input.buffer) {
      free(ctx.temp_input.buffer);
      ctx.temp_input.buffer = nullptr;
    }

    // Get the RAW_IMAGE channel
    EdgeAppLibSensorChannel channel;
    int ret = SensorFrameGetChannelFromChannelId(
        frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_RAW_IMAGE, &channel);
    if (ret < 0) {
      LOG_WARN("SensorFrameGetChannelFromChannelId failed: ret=%d.", ret);
      return AutoFrame(shared_ctx->sensor_stream, frame);  // Return anyway
    }

    // Get the raw data
    struct EdgeAppLibSensorRawData data = {0};
    SensorChannelGetRawData(channel, &data);
    LOG_DBG(
        "input_raw_data.address:%p\ninput_raw_data.size:%zu\ninput_raw_data."
        "timestamp:%llu\ninput_raw_data.type:%s",
        data.address, data.size, data.timestamp, data.type);
    EdgeAppLibDrawBuffer src{};
    EdgeAppLibSensorImageProperty image_property;
    SensorChannelGetProperty(channel, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                             &image_property, sizeof(image_property));
    src.width = image_property.width;
    src.height = image_property.height;
    if (strncmp(image_property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB24,
                sizeof(AITRIOS_SENSOR_PIXEL_FORMAT_RGB24)) == 0) {
      src.format = AITRIOS_DRAW_FORMAT_RGB8;
    } else if (strncmp(image_property.pixel_format,
                       AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
                       sizeof(AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR)) == 0) {
      src.format = AITRIOS_DRAW_FORMAT_RGB8_PLANAR;
    } else {
      LOG_ERR("Unsupported pixel format: %s", image_property.pixel_format);
      return AutoFrame(shared_ctx->sensor_stream, frame);  // Return anyway
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
    SensorChannelGetProperty(channel, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                             &it_image_property, sizeof(it_image_property));

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
      dst.size = dst_size;
      dst.address = (uint8_t *)malloc(dst_size);
      CropRectangle(&src, &dst, roi.left, roi.top, roi.left + roi.width,
                    roi.top + roi.height);

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
    }

    // Set input tensor and run inference
    if (ctx.graph_ctx != nullptr) {
      uint32_t dims[4] = {1, 3, roi.width, roi.height};
      if (SetInput(*(ctx.graph_ctx), ctx.temp_input.buffer, dims) != 0) {
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
    uint32_t total_elements = 0;

    output_tensor.data = base;
    output_tensor.size = MAX_OUTPUT_TENSORS_SIZE * sizeof(float);
    output_tensor.shape_info.ndim = 0;
    output_tensor.type = TensorDataType::TensorTypeFloat32;

    for (size_t j = 0; j < tensor_num; ++j) {
      uint32_t remaining_elements = MAX_OUTPUT_TENSORS_SIZE - total_elements;
      uint32_t outsize = remaining_elements;

      if (EdgeAppLib::GetOutput(*ctx.graph_ctx, j, write_ptr, &outsize) != 0) {
        // LOG_WARN("Failed to get output tensor index %zu", j);
        continue;
      }

      output_tensor.shape_info.dims[output_tensor.shape_info.ndim++] = outsize;
      write_ptr += outsize;
      total_elements += outsize;
    }

    void *tmp = realloc(base, total_elements * sizeof(float));
    if (tmp == nullptr) {
      free(base);
      LOG_ERR("realloc failed");
      return {};
    }
    output_tensor.data = tmp;
    output_tensor.size = total_elements * sizeof(float);

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
      input_tensor.timestamp = 0;  // No timestamp for CPU/NPU input
      input_tensor.type = TensorDataType::TensorTypeUInt8;
      input_tensor.shape_info.ndim = 4;
      input_tensor.shape_info.dims[0] = 1;
      input_tensor.shape_info.dims[1] = temp.height;
      input_tensor.shape_info.dims[2] = temp.width;
      input_tensor.shape_info.dims[3] = 3;
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
 * @param frame Pointer to the current sensor frame.
 * @return A future representing the asynchronous operation of sending the input
 * tensor.
 */

#include <mutex>
#include <string>

struct StreamEntry {
  char name[64];       // Stream name
  uint64_t stream_id;  // Stream ID
  uint32_t width;      // Registered width
  uint32_t height;     // Registered height
};

// Temporal scaling function to resize the input image to the target dimensions
// using nearest neighbor interpolation. This function assumes the input image
// is in RGB format (3 channels).
// @param input Pointer to the input image data.
// @param width Width of the input image.
// @param height Height of the input image.
// @param output Pointer to the output image data.
// @param target_width Desired width of the output image.
// @param target_height Desired height of the output image.
// @param channels Number of channels in the input image (e.g., 3 for RGB).

static void PlaceInputImageWithPadding(const uint8_t *input, uint32_t in_width,
                                       uint32_t in_height, uint8_t *output,
                                       uint32_t out_width, uint32_t out_height,
                                       uint32_t channels) {
  memset(output, 0, out_width * out_height * channels);

  uint32_t copy_height = (in_height < out_height) ? in_height : out_height;
  uint32_t copy_width = (in_width < out_width) ? in_width : out_width;

  uint32_t copy_bytes_per_row = copy_width * channels;

  for (uint32_t y = 0; y < copy_height; ++y) {
    uint8_t *dst_row = output + (y * out_width) * channels;
    const uint8_t *src_row = input + (y * in_width) * channels;

    memcpy(dst_row, src_row, copy_bytes_per_row);
  }
}

static uint64_t CreateStreamIfNameChanged(EdgeAppCore::Tensor *input,
                                          const char *pixel_format,
                                          void **present_data) {
  constexpr size_t MAX_STREAMS = 6;  // Maximum number of stream slots
  static StreamEntry stream_entries[MAX_STREAMS] = {};
  static std::mutex mutex;
  uint64_t stream_id = 0;
  std::lock_guard<std::mutex> lock(mutex);

  if (input->name[0] == '\0') {
    strcpy(input->name, "");
  }
  uint32_t width = input->shape_info.dims[2];
  uint32_t height = input->shape_info.dims[1];
  uint32_t stride_bytes = width * 3;  // Assuming RGB format, 3 bytes per pixel

  // Check if a stream with the same name already exists
  for (size_t i = 0; i < MAX_STREAMS; ++i) {
    if (strcmp(stream_entries[i].name, input->name) == 0) {
      uint32_t target_width = stream_entries[i].width;
      uint32_t target_height = stream_entries[i].height;
      if (width != target_width || height != target_height) {
        LOG_DBG("Resizing input image from %dx%d to %dx%d for stream %s", width,
                height, target_width, target_height, stream_entries[i].name);
        *present_data = malloc(target_width * target_height * 3);
        PlaceInputImageWithPadding((uint8_t *)input->data, width, height,
                                   (uint8_t *)*present_data, target_width,
                                   target_height, 3);
      } else {
        LOG_DBG("Using existing stream %s with ID %llu", stream_entries[i].name,
                stream_entries[i].stream_id);
        *present_data = input->data;  // No scaling needed
      }
      return stream_entries[i].stream_id;
    }
  }

  // Find an empty slot or overwrite the oldest one (FIFO strategy)
  size_t slot = 0;
  for (; slot < MAX_STREAMS; ++slot) {
    if (stream_entries[slot].stream_id == 0) break;
  }
  if (slot == MAX_STREAMS) {
    slot = 0;  // All slots are used, overwrite the first one
  }

  LOG_INFO("Creating new stream for name: %s", input->name);
  stream_id = senscord_ub_create_stream(input->name, width, height,
                                        stride_bytes, pixel_format);

  // Store the new stream info in the selected slot
  strncpy(stream_entries[slot].name, input->name,
          sizeof(stream_entries[slot].name) - 1);
  stream_entries[slot].name[sizeof(stream_entries[slot].name) - 1] = '\0';
  stream_entries[slot].stream_id = stream_id;
  stream_entries[slot].width = width;
  stream_entries[slot].height = height;

  return stream_id;
}

EdgeAppCoreResult SendInputTensor(Tensor *input_tensor) {
  LOG_TRACE("Inside sendInputTensor.");
  if (input_tensor == nullptr || input_tensor->data == nullptr) {
    LOG_ERR("Invalid input tensor data.");
    return EdgeAppCoreResultInvalidParam;
  }
#if 0
  void *present_data = nullptr;
  uint64_t stream_id = CreateStreamIfNameChanged(
      input_tensor, SENSCORD_PIXEL_FORMAT_RGB24, &present_data);

  if (stream_id != 0) {
    LOG_INFO("Stream ID: %llu input_tensor width=%d height=%d ", stream_id,
             input_tensor->shape_info.dims[2],
             input_tensor->shape_info.dims[1]);

    senscord_ub_send_data(stream_id, present_data);
    if (present_data != nullptr && input_tensor->data != present_data) {
      free(present_data);
      present_data = nullptr;  // Free only if we allocated new data
    }
  }
#endif
  auto future = DataExportSendData((char *)PORTNAME_INPUT,
                                   EdgeAppLibDataExportRaw, input_tensor->data,
                                   input_tensor->size, input_tensor->timestamp);

  DataExportAwait(future, -1);
  DataExportCleanup(future);
  if (future == nullptr) {
    free(input_tensor->data);  // Free only if we allocated new data
  }

  return EdgeAppCoreResultSuccess;
}

}  // namespace EdgeAppCore
