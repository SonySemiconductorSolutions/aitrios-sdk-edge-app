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

#include "mock_edgecore.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "log.h"

using namespace EdgeAppCore;

// static mock state
static EdgeAppCoreResult load_model_result = EdgeAppCoreResultSuccess;
static EdgeAppCoreResult send_it_result = EdgeAppCoreResultSuccess;
static EdgeAppCoreResult unload_model_result = EdgeAppCoreResultSuccess;
static bool get_output_result = true;  // Simulate successful output retrieval
static bool get_input_result = true;   // Simulate successful input retrieval
static bool process_result = true;     // Simulate successful processing
static Tensor g_mock_tensor = {};
static EdgeAppLibSensorFrame g_mock_sensor_frame = 0x1234;
static EdgeAppLibSensorStream g_mock_sensor_stream = 0x5678;
static ProcessedFrame g_mock_frame =
    ProcessedFrame(&g_mock_sensor_stream, g_mock_sensor_frame);

/* 0 = no called, 1 = called */
static int EdgeAppCoreLoadModelCalled = 0;
static int EdgeAppCoreProcessCalled = 0;
static int EdgeAppCoreGetOutputCalled = 0;
static int EdgeAppCoreGetInputCalled = 0;
static int EdgeAppCoreUnloadModelCalled = 0;
static int EdgeAppCoreSendInputTensorCalled = 0;

// Get

// Dynamically allocated dummy buffer for Tensor data
static float *dummy_tensor_data = nullptr;
static const size_t kMaxMockTensorSize = 16;

// Structure to map specific ctx pointers to Tensor outputs
static struct {
  const EdgeAppCoreCtx *ctx;
  Tensor output_tensor;
  Tensor input_tensor;
} g_mock_outputs[10];  // Supports up to 10 different ctxs
static int g_mock_output_count = 0;

// Reset all mock state to default
void reset_mock_core_state() {
  load_model_result = EdgeAppCoreResultSuccess;
  send_it_result = EdgeAppCoreResultSuccess;
  unload_model_result = EdgeAppCoreResultSuccess;
  EdgeAppCoreLoadModelCalled = 0;
  EdgeAppCoreProcessCalled = 0;
  EdgeAppCoreGetOutputCalled = 0;
  EdgeAppCoreGetInputCalled = 0;
  EdgeAppCoreUnloadModelCalled = 0;
  EdgeAppCoreSendInputTensorCalled = 0;
  get_output_result = true;
  get_input_result = true;
  process_result = true;
  g_mock_sensor_frame = 0x1234;
  g_mock_sensor_stream = 0x5678;
  g_mock_frame = ProcessedFrame(&g_mock_sensor_stream, g_mock_sensor_frame);
}

// Set dummy data values into tensor data
void set_mock_output_tensor_data(const float *out_data, size_t out_size,
                                 uint32_t index) {
  g_mock_outputs[index].output_tensor.data = (void *)out_data;
  g_mock_outputs[index].output_tensor.size = out_size * sizeof(float);
}

// Set dummy data values into tensor data
void set_mock_input_tensor_data(const char *in_data, size_t in_size,
                                uint32_t index) {
  g_mock_outputs[index].input_tensor.data = (void *)in_data;
  g_mock_outputs[index].input_tensor.size = in_size * sizeof(char);
}

// Clear all ctx-to-tensor registrations
void reset_mock_outputs() {
  g_mock_output_count = 0;
  memset(g_mock_outputs, 0, sizeof(g_mock_outputs));
}

void setLoadModelResult(EdgeAppCoreResult result) {
  load_model_result = result;
}

void setITsendResult(EdgeAppCoreResult result) { send_it_result = result; }

void setUnloadModelResult(EdgeAppCoreResult result) {
  unload_model_result = result;
}

void setGetOutputResult(bool result) { get_output_result = result; }

void setGetInputResult(bool result) { get_input_result = result; }

void setProcessResult(bool result) { process_result = result; }

int wasEdgeAppCoreLoadModelCalled() { return EdgeAppCoreLoadModelCalled; }
int wasEdgeAppCoreProcessCalled() { return EdgeAppCoreProcessCalled; }
int wasEdgeAppCoreGetOutputCalled() { return EdgeAppCoreGetOutputCalled; }
int wasEdgeAppCoreGetInputCalled() { return EdgeAppCoreGetInputCalled; }

int wasEdgeAppCoreUnloadModelCalled() { return EdgeAppCoreUnloadModelCalled; }

int wasEdgeAppCoreSendInputTensorCalled() {
  return EdgeAppCoreSendInputTensorCalled;
}
// ======= MOCK FUNCTION IMPLEMENTATIONS =======

namespace EdgeAppCore {

EdgeAppCoreResult LoadModel(EdgeAppCoreModelInfo, EdgeAppCoreCtx &ctx,
                            EdgeAppCoreCtx *SensorCtx) {
  EdgeAppCoreLoadModelCalled = 1;
  if (SensorCtx) {
    SensorCtx->sensor_stream = &g_mock_sensor_stream;
  }
  if (g_mock_output_count < 10) {  // Ensure we do not exceed the array bounds
    float *dummy_output_data =
        (float *)malloc(kMaxMockTensorSize * sizeof(float));
    memset(dummy_output_data, 0, kMaxMockTensorSize * sizeof(float));

    char *dummy_input_data = (char *)malloc(kMaxMockTensorSize * sizeof(char));
    memset(dummy_input_data, 0, kMaxMockTensorSize * sizeof(char));
    set_mock_output_tensor_data(dummy_output_data, kMaxMockTensorSize,
                                g_mock_output_count);
    set_mock_input_tensor_data(dummy_input_data, kMaxMockTensorSize,
                               g_mock_output_count);
    g_mock_outputs[g_mock_output_count++].ctx = &ctx;
  }

  return load_model_result;
}

ProcessedFrame Process(EdgeAppCoreCtx &, EdgeAppCoreCtx *,
                       EdgeAppLibSensorFrame,
                       EdgeAppLibSensorImageCropProperty &) {
  EdgeAppCoreProcessCalled = 1;
  if (!process_result) {
    return ProcessedFrame();  // Simulate failure
  }
  return ProcessedFrame(&g_mock_sensor_stream, g_mock_sensor_frame);
}

ProcessedFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame) {
  EdgeAppCoreProcessCalled = 1;
  if (!process_result) {
    return ProcessedFrame();  // Simulate failure
  }
  return ProcessedFrame(&g_mock_sensor_stream, g_mock_sensor_frame);
}

Tensor GetOutput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame, uint32_t) {
  EdgeAppCoreGetOutputCalled = 1;
  if (get_output_result) {
    for (int i = 0; i < g_mock_output_count; ++i) {
      if (g_mock_outputs[i].ctx == &ctx) {
        LOG_WARN("Mock GetOutput for ctx[%d]", i);
        return g_mock_outputs[i].output_tensor;
      }
    }
  }
  LOG_WARN("Mock GetOutput: Simulated error, returning empty tensor");
  return g_mock_tensor;  // Return empty tensor
}

// New function for GetOutputs (returns vector)
std::vector<Tensor> GetOutputs(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                               uint32_t max_tensor_num) {
  EdgeAppCoreGetOutputCalled = 1;
  std::vector<Tensor> outputs;

  if (get_output_result) {
    for (int i = 0; i < g_mock_output_count; ++i) {
      if (g_mock_outputs[i].ctx == &ctx) {
        LOG_WARN("Mock GetOutputs for ctx[%d] with max_tensor_num[%d]", i,
                 max_tensor_num);

        // Generate mock tensors - up to max_tensor_num or 4, whichever is
        // smaller
        uint32_t num_tensors = (max_tensor_num < 4) ? max_tensor_num : 4;
        for (uint32_t t = 0; t < num_tensors; ++t) {
          Tensor tensor = g_mock_outputs[i].output_tensor;
          tensor.size =
              (10 - t * 2) * sizeof(float);  // Different sizes: 10, 8, 6, 4
          outputs.push_back(tensor);
        }
        return outputs;
      }
    }
  } else {
    LOG_WARN("Mock GetOutputs: Simulated error, returning empty vector");
  }

  return outputs;  // Return empty vector
}

// New overload for Process with PreprocessCallback parameter
ProcessedFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame,
                       EdgeAppLibSensorImageCropProperty &roi,
                       PreprocessCallback preprocess_func) {
  EdgeAppCoreProcessCalled = 1;
  if (!process_result) {
    return ProcessedFrame();  // Simulate failure
  }

  // For testing purposes, we don't actually call the preprocess_func in the
  // mock Just return the same result as the original Process function
  LOG_WARN("Mock Process with preprocessing callback called");
  return ProcessedFrame(&g_mock_sensor_stream, g_mock_sensor_frame);
}

// 5-argument Process function for compute() method
ProcessedFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame,
                       EdgeAppLibSensorImageCropProperty roi,
                       PreprocessCallback preprocess_func) {
  EdgeAppCoreProcessCalled = 1;
  if (!process_result) {
    return ProcessedFrame();  // Simulate failure
  }
  LOG_WARN("Mock Process with 5 arguments called");
  return ProcessedFrame(&g_mock_sensor_stream, g_mock_sensor_frame);
}

Tensor GetInput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame) {
  EdgeAppCoreGetInputCalled = 1;
  if (!get_input_result) {
    LOG_WARN("Mock GetInput: Simulated error, returning empty tensor");
    return g_mock_tensor;  // Return empty tensor
  }
  for (int i = 0; i < g_mock_output_count; ++i) {
    if (g_mock_outputs[i].ctx == &ctx) {
      return g_mock_outputs[i].input_tensor;
    }
  }
  LOG_WARN("Mock GetInput: No matching ctx found, returning default tensor");
  return g_mock_tensor;
}

EdgeAppCoreResult UnloadModel(EdgeAppCoreCtx &) {
  EdgeAppCoreUnloadModelCalled = 1;
  return unload_model_result;
}

EdgeAppCoreResult SendInputTensor(Tensor *) {
  EdgeAppCoreSendInputTensorCalled = 1;
  return send_it_result;
}

void ProcessedFrame::ProcessInternal(EdgeAppCoreCtx &ctx,
                                     EdgeAppCoreCtx *shared_ctx,
                                     EdgeAppLibSensorFrame frame,
                                     EdgeAppLibSensorImageCropProperty &roi) {}

ProcessedFrame ProcessedFrame::compute() { return std::move(*this); }

}  // namespace EdgeAppCore
