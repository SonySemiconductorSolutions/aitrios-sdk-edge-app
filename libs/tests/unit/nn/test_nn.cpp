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
  EdgeAppLibGraph g{};
  EdgeAppLibGraphContext ctx{};
  uint8_t input_data[12]{};
  uint32_t dims[4]{1, 2, 2, 3};  // 1x2x2x3
  float output[10]{};
  uint32_t output_size{10};

  void SetUp() override {
    // Usually reset mock state here if needed
  }
};

TEST_F(EdgeApplibNNTest, LoadModelSuccess) {
  auto result = LoadModel("dummy_model.onnx", &g, EDGEAPP_TARGET_CPU);
  EXPECT_EQ(result, EDGEAPP_LIB_NN_SUCCESS);
}

TEST_F(EdgeApplibNNTest, InitContextSuccess) {
  LoadModel("dummy_model.onnx", &g, EDGEAPP_TARGET_CPU);
  auto result = InitContext(g, &ctx);
  EXPECT_EQ(result, EDGEAPP_LIB_NN_SUCCESS);
}

TEST_F(EdgeApplibNNTest, SetInputSuccess) {
  LoadModel("dummy_model.onnx", &g, EDGEAPP_TARGET_CPU);
  InitContext(g, &ctx);
  const float mean_values[] = {0.0f, 0.0f, 0.0f};
  const size_t mean_size = 3;
  const float norm_values[] = {1.0f, 1.0f, 1.0f};
  const size_t norm_size = 3;
  auto result = SetInput(ctx, input_data, dims, mean_values, mean_size,
                         norm_values, norm_size);
  EXPECT_EQ(result, EDGEAPP_LIB_NN_SUCCESS);
}

TEST_F(EdgeApplibNNTest, ComputeSuccess) {
  LoadModel("dummy_model.onnx", &g, EDGEAPP_TARGET_CPU);
  InitContext(g, &ctx);
  const float mean_values[] = {0.0f, 0.0f, 0.0f};
  const size_t mean_size = 3;
  const float norm_values[] = {1.0f, 1.0f, 1.0f};
  const size_t norm_size = 3;
  SetInput(ctx, input_data, dims, mean_values, mean_size, norm_values,
           norm_size);
  auto result = Compute(ctx);
  EXPECT_EQ(result, EDGEAPP_LIB_NN_SUCCESS);
}

TEST_F(EdgeApplibNNTest, GetOutputSuccess) {
  LoadModel("dummy_model.onnx", &g, EDGEAPP_TARGET_CPU);
  InitContext(g, &ctx);
  const float mean_values[] = {0.0f, 0.0f, 0.0f};
  const size_t mean_size = 3;
  const float norm_values[] = {1.0f, 1.0f, 1.0f};
  const size_t norm_size = 3;
  SetInput(ctx, input_data, dims, mean_values, mean_size, norm_values,
           norm_size);
  Compute(ctx);
  auto result = GetOutput(ctx, 0, output, &output_size);
  EXPECT_EQ(result, EDGEAPP_LIB_NN_SUCCESS);
  EXPECT_EQ(output_size, 10u);
}
