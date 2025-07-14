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

#include <stdio.h>
#include <string.h>

#include "wasi_nn.h"

// Always return success by default
wasi_nn_error load(graph_builder_array *builder, graph_encoding encoding,
                   execution_target target, graph *g) {
  *g = (graph)1;  // Dummy handle
  return success;
}

wasi_nn_error load_by_name(const char *name, uint32_t name_len, graph *g) {
  *g = (graph)1;  // Dummy handle
  return success;
}

wasi_nn_error init_execution_context(graph g, graph_execution_context *ctx) {
  *ctx = (graph_execution_context)1;  // Dummy context
  return success;
}

wasi_nn_error set_input(graph_execution_context ctx, uint32_t index,
                        tensor *t) {
  (void)ctx;
  (void)index;
  (void)t;
  return success;
}

wasi_nn_error compute(graph_execution_context ctx) {
  (void)ctx;
  return success;
}

wasi_nn_error get_output(graph_execution_context ctx, uint32_t index,
                         tensor_data output_tensor,
                         uint32_t *output_tensor_size) {
  (void)ctx;
  (void)index;
  memset(output_tensor, 0xAA, *output_tensor_size);  // Dummy output
  *output_tensor_size = 10;                          // Dummy output size
  return success;
}

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
