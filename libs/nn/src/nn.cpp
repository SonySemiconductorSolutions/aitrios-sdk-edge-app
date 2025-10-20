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
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#define INPUT_TENSOR_DIMS 4

#ifdef __cplusplus
extern "C" {
#include "wasi_nn.h"
namespace EdgeAppLib {
#endif

static EdgeAppLibNNResult convert_err_code_from_wasi_nn(wasi_nn_error err) {
  return static_cast<EdgeAppLibNNResult>(err);
}

EdgeAppLibNNResult LoadModel(const char *model_name, EdgeAppLibGraph *g,
                             EdgeAppLibExecutionTarget target) {
  if (model_name == NULL || g == NULL) {
    return EDGEAPP_LIB_NN_INVALID_ARGUMENT;
  }

  // Call the WASI-NN load_by_name function
  wasi_nn_error err = load_by_name(model_name, strlen(model_name), (graph *)g);
  return convert_err_code_from_wasi_nn(err);
}

EdgeAppLibNNResult InitContext(EdgeAppLibGraph g, EdgeAppLibGraphContext *ctx) {
  wasi_nn_error err =
      init_execution_context((graph)g, (graph_execution_context *)ctx);
  return convert_err_code_from_wasi_nn(err);
}

EdgeAppLibNNResult SetInputFromTensor(EdgeAppLibGraphContext ctx,
                                      uint8_t *input_tensor, uint32_t (*dim)[4],
                                      EdgeAppLibTensorType type) {
  tensor_dimensions dims;
  dims.size = INPUT_TENSOR_DIMS;
  dims.buf = (uint32_t *)malloc(dims.size * sizeof(uint32_t));
  if (dims.buf == NULL) return EDGEAPP_LIB_NN_TOO_LARGE;

  for (int i = 0; i < dims.size; ++i) {
    dims.buf[i] = (*dim)[i];
  }
  tensor tensor;
  tensor.dimensions = &dims;
  tensor.type = (tensor_type)type;
  tensor.data = (uint8_t *)input_tensor;

  // Call wasi-nn set_input
  wasi_nn_error err = set_input((graph_execution_context)ctx, 0, &tensor);

  // Clean up temporary buffers
  free(dims.buf);

  return convert_err_code_from_wasi_nn(err);
}

EdgeAppLibNNResult SetInput(EdgeAppLibGraphContext ctx, uint8_t *input_tensor,
                            uint32_t *dim, const float *mean_values,
                            size_t mean_size, const float *norm_values,
                            size_t norm_size) {
  tensor_dimensions dims;
  dims.size = INPUT_TENSOR_DIMS;
  dims.buf = (uint32_t *)malloc(dims.size * sizeof(uint32_t));
  if (dims.buf == NULL) return EDGEAPP_LIB_NN_TOO_LARGE;

  for (int i = 0; i < dims.size; ++i) {
    dims.buf[i] = dim[i];
  }

  uint32_t n = dims.buf[0];
  uint32_t h = dims.buf[1];
  uint32_t w = dims.buf[2];
  uint32_t c = dims.buf[3];
  size_t num_elements = n * c * h * w;

  float *float_buffer = (float *)malloc(num_elements * sizeof(float));
  if (float_buffer == NULL) {
    free(dims.buf);
    return EDGEAPP_LIB_NN_TOO_LARGE;
  }

  // Normalize channel by channel uint8[0-255] would be mapped to float
  // [0.0-1.0], then (value - mean) / norm
  for (uint32_t ci = 0; ci < c; ++ci) {
    for (uint32_t i = ci; i < num_elements; i += c) {
      float val =
          static_cast<float>(input_tensor[i]) / 255.0f;  // Scale to [0.0, 1.0]
      float_buffer[i] = (val - mean_values[ci]) / norm_values[ci];
    }
  }

  tensor tensor;
  tensor.dimensions = &dims;
  tensor.type = fp32;
  tensor.data = (uint8_t *)float_buffer;

  // Call wasi-nn set_input
  wasi_nn_error err = set_input((graph_execution_context)ctx, 0, &tensor);

  // Clean up temporary buffers
  free(float_buffer);
  free(dims.buf);

  return convert_err_code_from_wasi_nn(err);
}

EdgeAppLibNNResult Compute(EdgeAppLibGraphContext ctx) {
  wasi_nn_error err = compute((graph_execution_context)ctx);

  return convert_err_code_from_wasi_nn(err);
}

EdgeAppLibNNResult GetOutput(EdgeAppLibGraphContext ctx, uint32_t index,
                             float *out_tensor, uint32_t *out_size) {
  wasi_nn_error err = get_output((graph_execution_context)ctx, index,
                                 (tensor_data)out_tensor, out_size);
  return convert_err_code_from_wasi_nn(err);
}

#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib
