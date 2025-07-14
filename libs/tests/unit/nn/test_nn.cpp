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

#include <gtest/gtest.h>
#include <stdint.h>
#include <string.h>

#include "nn.h"

using namespace EdgeAppLib;

class EdgeApplibNNTest : public ::testing::Test {
 protected:
  graph g{};
  graph_execution_context ctx{};
  uint8_t input_data[12]{};
  uint32_t dims[4]{1, 2, 2, 3};  // 1x2x2x3
  float output[10]{};
  uint32_t output_size{10};

  void SetUp() override {
    // Usually reset mock state here if needed
  }
};

TEST_F(EdgeApplibNNTest, LoadModelSuccess) {
  auto result = LoadModel("dummy_model.onnx", &g, cpu);
  EXPECT_EQ(result, success);
}

TEST_F(EdgeApplibNNTest, InitContextSuccess) {
  LoadModel("dummy_model.onnx", &g, cpu);
  auto result = InitContext(g, &ctx);
  EXPECT_EQ(result, success);
}

TEST_F(EdgeApplibNNTest, SetInputSuccess) {
  LoadModel("dummy_model.onnx", &g, cpu);
  InitContext(g, &ctx);
  auto result = SetInput(ctx, input_data, dims);
  EXPECT_EQ(result, success);
}

TEST_F(EdgeApplibNNTest, ComputeSuccess) {
  LoadModel("dummy_model.onnx", &g, cpu);
  InitContext(g, &ctx);
  SetInput(ctx, input_data, dims);
  auto result = Compute(ctx);
  EXPECT_EQ(result, success);
}

TEST_F(EdgeApplibNNTest, GetOutputSuccess) {
  LoadModel("dummy_model.onnx", &g, cpu);
  InitContext(g, &ctx);
  SetInput(ctx, input_data, dims);
  Compute(ctx);
  auto result = GetOutput(ctx, 0, output, &output_size);
  EXPECT_EQ(result, success);
  EXPECT_EQ(output_size, 10u);
}
