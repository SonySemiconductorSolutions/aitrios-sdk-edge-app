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
static EdgeAppLibNNResult LoadModelStatus = EDGEAPP_LIB_NN_SUCCESS;
static EdgeAppLibNNResult InitContextStatus = EDGEAPP_LIB_NN_SUCCESS;
static EdgeAppLibNNResult SetInputStatus = EDGEAPP_LIB_NN_SUCCESS;
static EdgeAppLibNNResult ComputeStatus = EDGEAPP_LIB_NN_SUCCESS;
static EdgeAppLibNNResult GetOutputStatus = EDGEAPP_LIB_NN_SUCCESS;

// Functions to toggle error simulation for tests
void setLoadModelError() { LoadModelStatus = EDGEAPP_LIB_NN_RUNTIME_ERROR; }
void resetLoadModelStatus() { LoadModelStatus = EDGEAPP_LIB_NN_SUCCESS; }

void setInitContextError() { InitContextStatus = EDGEAPP_LIB_NN_RUNTIME_ERROR; }
void resetInitContextStatus() { InitContextStatus = EDGEAPP_LIB_NN_SUCCESS; }

void setSetInputError() { SetInputStatus = EDGEAPP_LIB_NN_RUNTIME_ERROR; }
void resetSetInputStatus() { SetInputStatus = EDGEAPP_LIB_NN_SUCCESS; }

void setComputeError() { ComputeStatus = EDGEAPP_LIB_NN_RUNTIME_ERROR; }
void resetComputeStatus() { ComputeStatus = EDGEAPP_LIB_NN_SUCCESS; }

void setGetOutputError() { GetOutputStatus = EDGEAPP_LIB_NN_RUNTIME_ERROR; }
void resetGetOutputStatus() { GetOutputStatus = EDGEAPP_LIB_NN_SUCCESS; }

// Mock implementation of LoadModel
EdgeAppLibNNResult LoadModel(const char *model_name, EdgeAppLibGraph *g,
                             EdgeAppLibGraphContext target) {
  if (LoadModelStatus != EDGEAPP_LIB_NN_SUCCESS) return LoadModelStatus;
  *g = (EdgeAppLibGraph)123;  // Dummy graph handle
  return EDGEAPP_LIB_NN_SUCCESS;
}

// Mock implementation of InitContext
EdgeAppLibNNResult InitContext(EdgeAppLibGraph g, EdgeAppLibGraphContext *ctx) {
  if (InitContextStatus != EDGEAPP_LIB_NN_SUCCESS) return InitContextStatus;
  *ctx = (EdgeAppLibGraphContext)456;  // Dummy context handle
  return EDGEAPP_LIB_NN_SUCCESS;
}

// Mock implementation of SetInput
EdgeAppLibNNResult SetInput(EdgeAppLibGraphContext ctx, uint8_t *input_tensor,
                            uint32_t *dim, const float *mean_values,
                            size_t mean_size, const float *norm_values,
                            size_t norm_size) {
  if (SetInputStatus != EDGEAPP_LIB_NN_SUCCESS) return SetInputStatus;
  return EDGEAPP_LIB_NN_SUCCESS;
}

// Mock implementation of Compute
EdgeAppLibNNResult Compute(EdgeAppLibGraphContext ctx) {
  if (ComputeStatus != EDGEAPP_LIB_NN_SUCCESS) return ComputeStatus;
  return EDGEAPP_LIB_NN_SUCCESS;
}

// Mock implementation of GetOutput
EdgeAppLibNNResult GetOutput(EdgeAppLibGraphContext ctx, uint32_t index,
                             float *out_tensor, uint32_t *out_size) {
  if (GetOutputStatus != EDGEAPP_LIB_NN_SUCCESS) return GetOutputStatus;

  *out_size = 5;  // Dummy output size
  for (uint32_t i = 0; i < 5; ++i) {
    out_tensor[i] = (float)i;  // Dummy output data
  }
  return EDGEAPP_LIB_NN_SUCCESS;
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
