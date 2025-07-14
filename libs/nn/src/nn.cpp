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

#include "nn.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#define INPUT_TENSOR_DIMS 4
namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#include "wasi_nn.h"
#endif

EdgeAppLibNNResult LoadModel(const char *model_name, graph *g,
                             execution_target target) {
  EdgeAppLibNNResult res = load_by_name(model_name, strlen(model_name), g);
  return res;
}

EdgeAppLibNNResult InitContext(graph g, graph_execution_context *ctx) {
  return init_execution_context(g, ctx);
}

EdgeAppLibNNResult SetInput(graph_execution_context ctx, uint8_t *input_tensor,
                            uint32_t *dim) {
  tensor_dimensions dims;
  dims.size = INPUT_TENSOR_DIMS;
  dims.buf = (uint32_t *)malloc(dims.size * sizeof(uint32_t));
  if (dims.buf == NULL) return EdgeAppLibNNResult::too_large;

  for (int i = 0; i < dims.size; ++i) {
    dims.buf[i] = dim[i];
  }

  // Calculate total elements
  size_t num_elements = 1;
  for (int i = 0; i < dims.size; ++i) {
    num_elements *= dims.buf[i];
  }

  // Allocate float buffer
  float *float_buffer = (float *)malloc(num_elements * sizeof(float));
  if (float_buffer == NULL) {
    free(dims.buf);
    return EdgeAppLibNNResult::too_large;
  }

  // Convert uint8 [0-255] -> float [0-1]
  for (size_t i = 0; i < num_elements; ++i) {
    float_buffer[i] = static_cast<float>(input_tensor[i]) / 255.0f;
  }

  // Prepare tensor
  tensor tensor;
  tensor.dimensions = &dims;
  tensor.type = fp32;
  tensor.data = (uint8_t *)float_buffer;

  // Call set_input
  EdgeAppLibNNResult err = set_input(ctx, 0, &tensor);

  // Clean up
  free(float_buffer);
  free(dims.buf);

  return err;
}

EdgeAppLibNNResult Compute(graph_execution_context ctx) { return compute(ctx); }

EdgeAppLibNNResult GetOutput(graph_execution_context ctx, uint32_t index,
                             float *out_tensor, uint32_t *out_size) {
  return get_output(ctx, index, (uint8_t *)out_tensor, out_size);
}

#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib
