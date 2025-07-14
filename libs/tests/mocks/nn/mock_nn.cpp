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

#include "mock_nn.hpp"

#include <string.h>

// Global flags to simulate success or error
static EdgeAppLibNNResult LoadModelStatus = success;
static EdgeAppLibNNResult InitContextStatus = success;
static EdgeAppLibNNResult SetInputStatus = success;
static EdgeAppLibNNResult ComputeStatus = success;
static EdgeAppLibNNResult GetOutputStatus = success;

// Functions to toggle error simulation for tests
void setLoadModelError() { LoadModelStatus = runtime_error; }
void resetLoadModelStatus() { LoadModelStatus = success; }

void setInitContextError() { InitContextStatus = runtime_error; }
void resetInitContextStatus() { InitContextStatus = success; }

void setSetInputError() { SetInputStatus = runtime_error; }
void resetSetInputStatus() { SetInputStatus = success; }

void setComputeError() { ComputeStatus = runtime_error; }
void resetComputeStatus() { ComputeStatus = success; }

void setGetOutputError() { GetOutputStatus = runtime_error; }
void resetGetOutputStatus() { GetOutputStatus = success; }

// Mock implementation of LoadModel
EdgeAppLibNNResult LoadModel(const char *model_name, graph *g,
                             execution_target target) {
  if (LoadModelStatus != success) return LoadModelStatus;
  *g = (graph)123;  // Dummy graph handle
  return success;
}

// Mock implementation of InitContext
EdgeAppLibNNResult InitContext(graph g, graph_execution_context *ctx) {
  if (InitContextStatus != success) return InitContextStatus;
  *ctx = (graph_execution_context)456;  // Dummy context handle
  return success;
}

// Mock implementation of SetInput
EdgeAppLibNNResult SetInput(graph_execution_context ctx, uint8_t *input_tensor,
                            uint32_t *dim) {
  if (SetInputStatus != success) return SetInputStatus;
  return success;
}

// Mock implementation of Compute
EdgeAppLibNNResult Compute(graph_execution_context ctx) {
  if (ComputeStatus != success) return ComputeStatus;
  return success;
}

// Mock implementation of GetOutput
EdgeAppLibNNResult GetOutput(graph_execution_context ctx, uint32_t index,
                             float *out_tensor, uint32_t *out_size) {
  if (GetOutputStatus != success) return GetOutputStatus;

  *out_size = 5;  // Dummy output size
  for (uint32_t i = 0; i < 5; ++i) {
    out_tensor[i] = (float)i;  // Dummy output data
  }
  return success;
}

#include <stdlib.h>

uint64_t senscord_ub_create_stream(const char *name, uint32_t width,
                                   uint32_t height, uint32_t stride_bytes,
                                   const char *pixel_format) {
  // Return a dummy stream ID (non-zero to simulate success)
  return 123456789ULL;
}

int32_t senscord_ub_send_data(uint64_t handle, void *data) {
  (void)handle;
  (void)data;
  // Simulate success
  return 0;
}
