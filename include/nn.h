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

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "wasi_nn_types.h"
#ifdef __cplusplus
}
#endif

typedef wasi_nn_error EdgeAppLibNNResult;
#ifdef __cplusplus
namespace EdgeAppLib {
extern "C" {
#endif

// WASI-NN wrappers
EdgeAppLibNNResult LoadModel(const char *model_name, graph *g,
                             execution_target target);
EdgeAppLibNNResult InitContext(graph g, graph_execution_context *ctx);
EdgeAppLibNNResult SetInput(graph_execution_context ctx, uint8_t *input_tensor,
                            uint32_t *dim);
EdgeAppLibNNResult Compute(graph_execution_context ctx);
EdgeAppLibNNResult GetOutput(graph_execution_context ctx, uint32_t index,
                             float *out_tensor, uint32_t *out_size);

#ifdef __cplusplus
}  // extern "C"
}  // namespace EdgeAppCore
#endif
#endif
