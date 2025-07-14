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

#include "nn.h"

// Mock error toggle functions
void setLoadModelError();
void resetLoadModelStatus();

void setInitContextError();
void resetInitContextStatus();

void setSetInputError();
void resetSetInputStatus();

void setComputeError();
void resetComputeStatus();

void setGetOutputError();
void resetGetOutputStatus();

// Mock implementations of EdgeAppLib functions
namespace EdgeAppLib {

// Mock implementation of LoadModel
EdgeAppLibNNResult LoadModel(const char *model_name, graph *g,
                             execution_target target);

// Mock implementation of InitContext
EdgeAppLibNNResult InitContext(graph g, graph_execution_context *ctx);

// Mock implementation of SetInput
EdgeAppLibNNResult SetInput(graph_execution_context ctx, uint8_t *input_tensor,
                            uint32_t *dim);

// Mock implementation of Compute
EdgeAppLibNNResult Compute(graph_execution_context ctx);

// Mock implementation of GetOutput
EdgeAppLibNNResult GetOutput(graph_execution_context ctx, uint32_t index,
                             float *out_tensor, uint32_t *out_size);

}  // namespace EdgeAppLib

uint64_t senscord_ub_create_stream(const char *name, uint32_t width,
                                   uint32_t height, uint32_t stride_bytes,
                                   const char *pixel_format);
