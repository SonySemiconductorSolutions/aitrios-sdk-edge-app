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
#include <string.h>

#include <vector>

#include "edgeapp_core.h"
#include "mock_nn.hpp"  // Mock implementation of nn
#include "sensor.h"
using namespace EdgeAppCore;

// Dummy sensor frame and ROI data
static EdgeAppLibSensorStream dummy_stream = 1234;  // Dummy stream handle
static EdgeAppLibSensorFrame dummy_frame = 12345;   // Dummy frame
static EdgeAppLibSensorImageCropProperty dummy_roi[1] = {0, 0, 640, 480};

class EdgeAppCoreTest : public ::testing::Test {
 protected:
  EdgeAppCoreCtx ctx_imx500;
  EdgeAppCoreCtx ctx_cpu;
  const std::vector<float> mean_values = {0.485f, 0.456f, 0.406f};
  const std::vector<float> norm_values = {0.229f, 0.224f, 0.225f};
  EdgeAppCoreModelInfo model[2] = {
      {"dummy_model.onnx", edge_imx500, {}, {}},
      {"dummy_model2.onnx", edge_cpu, &mean_values, &norm_values}};

  void SetUp() override {
    // Reset mock nn to success state
    resetLoadModelStatus();
    resetInitContextStatus();
    resetSetInputStatus();
    resetComputeStatus();
    resetGetOutputStatus();
    memset(&ctx_imx500, 0, sizeof(ctx_imx500));
    memset(&ctx_cpu, 0, sizeof(ctx_cpu));
  }

  void TearDown() override {
    // Clean up context resources

    UnloadModel(ctx_imx500);
    UnloadModel(ctx_cpu);
  }
};

TEST_F(EdgeAppCoreTest, LoadModelsSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
}

TEST_F(EdgeAppCoreTest, LoadModelsInvalidParam) {
  // Check invalid parameters
  EdgeAppCoreModelInfo invalid_model{nullptr, edge_imx500, {}, {}};
  EXPECT_EQ(LoadModel(invalid_model, ctx_cpu, nullptr),
            EdgeAppCoreResultInvalidParam);
}

TEST_F(EdgeAppCoreTest, ProcessFrameSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);

  // Use dummy sensor frame
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);  // Ensure frame is valid
  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream,
                                 frame);  // It would be released by AutoFrame
}

TEST_F(EdgeAppCoreTest, ProcessFrameComputeError) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);

  // Simulate failure in Compute
  setComputeError();
  // ProcessFrame still returns success, but Compute inside may fail
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);  // Ensure frame is valid even if Compute failed

  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream,
                                 frame);  // It would be released by AutoFrame
}

TEST_F(EdgeAppCoreTest, GetOutputsSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  Tensor output = GetOutput(ctx_cpu, frame);

  // Expect outputs vector to be non-empty
  EXPECT_FALSE(output.data == nullptr || output.size == 0);

  // Send additional outputs as raw data
  free(output.data);  // Free the data if it was dynamically allocated

  EdgeAppLib::SensorReleaseFrame(
      *ctx_imx500.sensor_stream,
      frame);  // It would be released by AutoFrame
               // Free the output data if it was allocated
}

TEST_F(EdgeAppCoreTest, GetInputsSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  Tensor output = GetOutput(ctx_cpu, frame);
  free(output.data);

  auto input = GetInput(ctx_cpu, frame);
  ASSERT_TRUE(input.data != nullptr && input.size > 0);

  if (input.data == nullptr) {
    FAIL() << "input.data is NULL";
  }
  free(input.data);  // Free the input data if it was dynamically allocated
  // Clean up
  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream, frame);
}

TEST_F(EdgeAppCoreTest, LoadMultipleModelsAndProcess) {
  // Setup for more models (simulate up to 4)
  EdgeAppCoreCtx ctx_list[4];
  memset(ctx_list, 0, sizeof(ctx_list));
  const std::vector<float> mean_values = {0.485f, 0.456f, 0.406f};
  const std::vector<float> norm_values = {0.229f, 0.224f, 0.225f};
  EdgeAppCoreModelInfo models[4] = {
      {"model0.onnx", edge_imx500, {}, {}},
      {"model1.onnx", edge_cpu, &mean_values, &norm_values},
      {"model2.onnx", edge_cpu, &mean_values, &norm_values},
      {"model3.onnx", edge_cpu, &mean_values, &norm_values},
  };

  // Load models
  for (int i = 0; i < 4; ++i) {
    EdgeAppCoreResult res =
        LoadModel(models[i], ctx_list[i], (i == 0) ? nullptr : &ctx_list[0]);
    EXPECT_EQ(res, EdgeAppCoreResultSuccess) << "Failed to load model " << i;
  }
  {
    // Process frame for each
    auto frame = Process(ctx_list[0], &ctx_list[0], dummy_frame, dummy_roi[0]);
    EXPECT_NE(frame, 0);

    for (int i = 1; i < 4; ++i) {
      frame = Process(ctx_list[i], &ctx_list[0], frame, dummy_roi[0]);
      EXPECT_NE(frame, 0) << "Process failed for model " << i;
    }
  }

  // Unload models
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(UnloadModel(ctx_list[i]), EdgeAppCoreResultSuccess)
        << "Failed to unload model " << i;
  }
}

TEST_F(EdgeAppCoreTest, GetInputAndOutputForAllModels) {
  // Load 2 models
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  // Process
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);

  // Get input/output tensors
  auto input = GetInput(ctx_cpu, frame);
  EXPECT_TRUE(input.data != nullptr && input.size > 0);

  auto output = GetOutput(ctx_cpu, frame);
  EXPECT_TRUE(output.data != nullptr && output.size > 0);

  // Clean up
  free(output.data);
  free(input.data);
  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream, frame);
}

TEST_F(EdgeAppCoreTest, UnloadModelTwiceDoesNotCrash) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(UnloadModel(ctx_imx500), EdgeAppCoreResultSuccess);

  // Second unload (should not crash, even if redundant)
  EXPECT_EQ(UnloadModel(ctx_imx500), EdgeAppCoreResultSuccess);
}

TEST_F(EdgeAppCoreTest, LoadModelWithEmptyNameFails) {
  EdgeAppCoreModelInfo invalid_model = {"", edge_cpu, {}, {}};
  EXPECT_EQ(LoadModel(invalid_model, ctx_cpu, nullptr),
            EdgeAppCoreResultInvalidParam);
}

TEST_F(EdgeAppCoreTest, ComputeErrorIsLoggedButDoesNotCrash) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  setComputeError();
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);

  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream, frame);
}

TEST_F(EdgeAppCoreTest, LoadAndUnloadMultipleTimes) {
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr),
              EdgeAppCoreResultSuccess);
    EXPECT_EQ(UnloadModel(ctx_imx500), EdgeAppCoreResultSuccess);
  }
}

TEST_F(EdgeAppCoreTest, LoadModelWithInvalidTarget) {
  EdgeAppCoreModelInfo invalid_model = {
      "model.onnx", (EdgeAppCoreTarget)9999, {}, {}};
  EXPECT_EQ(LoadModel(invalid_model, ctx_cpu, nullptr),
            EdgeAppCoreResultInvalidParam);
}

TEST_F(EdgeAppCoreTest, ProcessFrameWithNullSharedCtx) {
  // Try calling Process with a nullptr shared context, which should fail or
  // warn
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, nullptr), EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, nullptr, dummy_frame, dummy_roi[0]);
  EXPECT_EQ(frame, 0);
  // Still expect valid frame returned or safe fallback
}

TEST_F(EdgeAppCoreTest, GetInputForIMX500ReturnsExpectedDims) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto input = GetInput(ctx_imx500, frame);
  EXPECT_EQ(input.shape_info.ndim, 4);
  EXPECT_EQ(input.shape_info.dims[3], 3);
  free(input.data);  // Free the data if it was dynamically allocated
  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream, frame);
}

TEST_F(EdgeAppCoreTest, ProcessFrameWithLargeROI) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);
  EdgeAppLibSensorImageCropProperty large_roi = {0, 0, 8000, 8000};
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, large_roi);
  EXPECT_NE(frame, 0);
  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream, frame);
}

TEST_F(EdgeAppCoreTest, ComputeFailureDoesNotCorruptNextFrame) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  setComputeError();
  auto frame1 = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame1, 0);

  // Reset error and process again
  resetComputeStatus();
  auto frame2 = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame2, 0);

  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream, frame1);
  EdgeAppLib::SensorReleaseFrame(*ctx_imx500.sensor_stream, frame2);
}

TEST_F(EdgeAppCoreTest, ModelIndexIsUniquePerModel) {
  EdgeAppCoreCtx ctx0, ctx1, ctx2;
  memset(&ctx0, 0, sizeof(ctx0));
  memset(&ctx1, 0, sizeof(ctx1));
  memset(&ctx2, 0, sizeof(ctx2));

  EXPECT_EQ(LoadModel(model[0], ctx0, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx1, &ctx0), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx2, &ctx0), EdgeAppCoreResultSuccess);

  EXPECT_NE(ctx0.model_idx, ctx1.model_idx);
  EXPECT_NE(ctx1.model_idx, ctx2.model_idx);

  // Clean up
  EXPECT_EQ(UnloadModel(ctx0), EdgeAppCoreResultSuccess);
  EXPECT_EQ(UnloadModel(ctx1), EdgeAppCoreResultSuccess);
  EXPECT_EQ(UnloadModel(ctx2), EdgeAppCoreResultSuccess);
}
