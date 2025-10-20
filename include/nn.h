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

#ifndef NN_WRAPPER_H_
#define NN_WRAPPER_H_

#include <stddef.h>
#include <stdint.h>
// Return codes
typedef enum {
  EDGEAPP_LIB_NN_SUCCESS = 0,
  EDGEAPP_LIB_NN_INVALID_ARGUMENT,
  EDGEAPP_LIB_NN_INVALID_ENCODING,
  EDGEAPP_LIB_NN_TIMEOUT,
  EDGEAPP_LIB_NN_RUNTIME_ERROR,
  EDGEAPP_LIB_NN_UNSUPPORTED_OPERATION,
  EDGEAPP_LIB_NN_TOO_LARGE,
  EDGEAPP_LIB_NN_NOT_FOUND,
  EDGEAPP_LIB_NN_SECURITY,
  EDGEAPP_LIB_NN_UNKNOWN,
  EDGEAPP_LIB_NN_END_OF_SEQUENCE = 100,
  EDGEAPP_LIB_NN_CONTEXT_FULL = 101,
  EDGEAPP_LIB_NN_PROMPT_TOO_LONG = 102,
  EDGEAPP_LIB_NN_MODEL_NOT_FOUND = 103
} EdgeAppLibNNResult;

typedef uint32_t EdgeAppLibGraphContext;
typedef uint32_t EdgeAppLibGraph;

#ifdef __cplusplus
namespace EdgeAppLib {
extern "C" {
#endif
// Execution target
typedef enum {
  EDGEAPP_TARGET_CPU = 0,
  EDGEAPP_TARGET_GPU,
  EDGEAPP_TARGET_NPU,
  EDGEAPP_TARGET_OTHER
} EdgeAppLibExecutionTarget;

typedef enum {
  TensorTypeFloat16 = 0,
  TensorTypeFloat32 = 1,
  TensorTypeUInt8 = 2,
  TensorTypeInt32 = 3,
  TensorTypeInt64 = 4
  // Extend as needed
} EdgeAppLibTensorType;

// API functions
EdgeAppLibNNResult LoadModel(const char *model_name, EdgeAppLibGraph *graph,
                             EdgeAppLibExecutionTarget target);
EdgeAppLibNNResult InitContext(EdgeAppLibGraph graph,
                               EdgeAppLibGraphContext *ctx);
EdgeAppLibNNResult SetInput(EdgeAppLibGraphContext ctx, uint8_t *input_tensor,
                            uint32_t *dim, const float *mean_values,
                            size_t mean_size, const float *norm_values,
                            size_t norm_size);

EdgeAppLibNNResult SetInputFromTensor(EdgeAppLibGraphContext ctx,
                                      uint8_t *input_tensor, uint32_t (*dim)[4],
                                      EdgeAppLibTensorType type);
EdgeAppLibNNResult Compute(EdgeAppLibGraphContext ctx);
EdgeAppLibNNResult GetOutput(EdgeAppLibGraphContext ctx, uint32_t index,
                             float *out_tensor, uint32_t *out_size);

#ifdef __cplusplus

}  // extern "C"
}  // namespace EdgeAppCore
#endif
#endif
