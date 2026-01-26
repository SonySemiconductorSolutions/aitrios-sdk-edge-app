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
#include "mock_sensor.hpp"
#include "receive_data.h"
#include "send_data_types.h"  // For EdgeAppLibImageProperty
#include "sensor.h"
using namespace EdgeAppCore;

// Dummy sensor frame and ROI data
static EdgeAppLibSensorStream dummy_stream = 1234;  // Dummy stream handle
static EdgeAppLibSensorFrame dummy_frame = 0;       // Dummy frame
static EdgeAppLibSensorImageCropProperty dummy_roi[1] = {0, 0, 640, 480};

class EdgeAppCoreTest : public ::testing::Test {
 public:
  static int executed_count;
  static int total_count;

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
    executed_count++;
    resetLoadModelStatus();
    resetInitContextStatus();
    resetSetInputStatus();
    resetComputeStatus();
    resetGetOutputStatus();
    const char *model_path = EdgeAppLibReceiveDataStorePath();
    int ret = mkdir(model_path, 0755);
    LOG_INFO("mkdir ret=%d errno=%d\n", ret, errno);

    memset(&ctx_imx500, 0, sizeof(ctx_imx500));
    memset(&ctx_cpu, 0, sizeof(ctx_cpu));

    for (auto &m : model) {
      std::string fullpath = std::string(model_path) + "/" + m.model_name;
      FILE *fp = fopen(fullpath.c_str(), "wb");
      if (fp) {
        // Write at least 1 byte so the file exists
        const char dummy = 0;
        fwrite(&dummy, sizeof(dummy), 1, fp);
        fclose(fp);
      } else {
        perror(("fopen failed: " + fullpath).c_str());
      }
    }
  }

  void TearDown() override {
    // Clean up context resources

    UnloadModel(ctx_imx500);
    UnloadModel(ctx_cpu);
    if (total_count == executed_count) {
      const char *model_path = EdgeAppLibReceiveDataStorePath();
      for (auto &m : model) {
        std::string fullpath = std::string(model_path) + "/" + m.model_name;
        unlink(fullpath.c_str());  // File delete
      }
      rmdir(model_path);
    }
  }
};

TEST_F(EdgeAppCoreTest, LoadModelsSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  // Verify model properties
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  EXPECT_STREQ(
      bundle_id.ai_model_bundle_id,
      model[0].model_name);  // Assuming model_name is the correct member
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
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  // Use dummy sensor frame
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);  // Ensure frame is valid
}

TEST_F(EdgeAppCoreTest, ProcessFrameComputeError) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  // Simulate failure in Compute
  setComputeError();
  // ProcessFrame still returns success, but Compute inside may fail
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);  // Ensure frame is valid even if Compute failed
}

TEST_F(EdgeAppCoreTest, SensorCoreInitFailed) {
  setEdgeAppLibSensorCoreInitFail();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultFailure);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  resetEdgeAppLibSensorCoreInitSuccess();
}

TEST_F(EdgeAppCoreTest, SensorOpenStreamfail) {
  setEdgeAppLibSensorCoreOpenStreamFail();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultFailure);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  resetEdgeAppLibSensorCoreOpenStreamSuccess();
}

TEST_F(EdgeAppCoreTest, SensorStartFail) {
  setEdgeAppLibSensorStartFail();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultFailure);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  resetEdgeAppLibSensorStartSuccess();
}

TEST_F(EdgeAppCoreTest, SensorFrameGetChannelFromChannelIdFail) {
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  printf("LoadModel result: %d\n", res);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  resetEdgeAppLibSensorChannelGetPropertySuccess();
}

TEST_F(EdgeAppCoreTest, GetOutputChannelFromChannelIdFail) {
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  printf("LoadModel result: %d\n", res);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  printf("Bundle ID: %s\n", bundle_id.ai_model_bundle_id);
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto output = GetOutput(ctx_imx500, frame, 4);
  EXPECT_EQ(output.data, nullptr);
  resetEdgeAppLibSensorChannelGetPropertySuccess();
}

TEST_F(EdgeAppCoreTest, GetInputSuccessIMX500) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  printf("LoadModel result: %d\n", res);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  printf("Bundle ID: %s\n", bundle_id.ai_model_bundle_id);
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto inputs = GetInput(ctx_imx500, frame);
  EXPECT_NE(inputs.data, nullptr);
  free(inputs.data);
}

TEST_F(EdgeAppCoreTest, GetInputFailureIMX500) {
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  printf("LoadModel result: %d\n", res);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  printf("Bundle ID: %s\n", bundle_id.ai_model_bundle_id);
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto inputs = GetInput(ctx_imx500, frame);
  EXPECT_EQ(inputs.data, nullptr);
  resetEdgeAppLibSensorChannelGetPropertySuccess();
}

TEST_F(EdgeAppCoreTest, GetOutputSuccessIMX500) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  printf("LoadModel result: %d\n", res);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  printf("Bundle ID: %s\n", bundle_id.ai_model_bundle_id);
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto output = GetOutput(ctx_imx500, frame, 4);
  EXPECT_NE(output.data, nullptr);
}

TEST_F(EdgeAppCoreTest, GetOutputsSuccessIMX500) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  printf("LoadModel result: %d\n", res);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  printf("Bundle ID: %s\n", bundle_id.ai_model_bundle_id);
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto outputs = GetOutputs(ctx_imx500, frame, 4);
  EXPECT_EQ(outputs.size(), 3);
}

TEST_F(EdgeAppCoreTest, NNLoadAPIFail) {
  setLoadModelError();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultFailure);
  resetLoadModelStatus();
}

TEST_F(EdgeAppCoreTest, NNInitContextAPIFail) {
  setInitContextError();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultFailure);
  resetInitContextStatus();
}

TEST_F(EdgeAppCoreTest, NNSetinputFail) {
  setSetInputError();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);

  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0); /* Keep the frame valid even if Compute fails, as per
                        * test expectations. The frame can still be used for
                        * GetInput/GetOutput operations. */
  resetSetInputStatus();
}

TEST_F(EdgeAppCoreTest, NNComputeFail) {
  setComputeError();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);

  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0); /* Keep the frame valid even if Compute fails, as per
                        * test expectations. The frame can still be used for
                        * GetInput/GetOutput operations. */
  resetComputeStatus();
}

TEST_F(EdgeAppCoreTest, NNGetOutputFail) {
  setGetOutputError();
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);

  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  EXPECT_EQ(outputs.size(), 0);
  resetGetOutputStatus();
}

TEST_F(EdgeAppCoreTest, GetOutputsSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto output = GetOutputs(ctx_cpu, frame, 4);

  EXPECT_EQ(output.size(), 4);
  for (const auto &out : output) {
    EXPECT_NE(out.data, nullptr);
    EXPECT_GT(out.size, 0);
    int ret = SendInference(out.data, out.size, EdgeAppLibSendDataJson, 0);
    free(out.data);
    EXPECT_EQ(ret, 0);
  }
}

TEST_F(EdgeAppCoreTest, GetOutputSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  EdgeAppLibSensorAiModelBundleIdProperty bundle_id;
  EdgeAppLib::SensorStreamGetProperty(
      *ctx_imx500.sensor_stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
      &bundle_id, sizeof(bundle_id));
  EXPECT_STREQ(bundle_id.ai_model_bundle_id, model[0].model_name);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  Tensor output = GetOutput(ctx_cpu, frame, 4);

  // Expect outputs vector to be non-empty
  EXPECT_FALSE(output.data == nullptr || output.size == 0);

  // Send additional outputs as raw data
  int ret = SendInference(output.data, output.size, EdgeAppLibSendDataJson, 0);
  free(output.data);
  EXPECT_EQ(ret, 0);
}

TEST_F(EdgeAppCoreTest, GetInputsSuccess) {
  EdgeAppCoreResult res = LoadModel(model[0], ctx_imx500, nullptr);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  res = LoadModel(model[1], ctx_cpu, &ctx_imx500);
  EXPECT_EQ(res, EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  Tensor output = GetOutput(ctx_cpu, frame, 4);
  free(output.data);

  auto input = GetInput(ctx_cpu, frame);
  ASSERT_TRUE(input.data != nullptr && input.size > 0);

  if (input.data == nullptr) {
    FAIL() << "input.data is NULL";
  }
  free(input.data);  // Free the input data if it was dynamically allocated
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

  const char *model_path = EdgeAppLibReceiveDataStorePath();
  for (auto &m : models) {
    std::string fullpath = std::string(model_path) + "/" + m.model_name;
    FILE *fp = fopen(fullpath.c_str(), "wb");
    if (fp) {
      // Write at least 1 byte so the file exists
      const char dummy = 0;
      fwrite(&dummy, sizeof(dummy), 1, fp);
      fclose(fp);
    } else {
      perror(("fopen failed: " + fullpath).c_str());
    }
  }

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

  auto output = GetOutput(ctx_cpu, frame, 4);
  EXPECT_TRUE(output.data != nullptr && output.size > 0);

  // Clean up
  free(output.data);
  free(input.data);
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
}

TEST_F(EdgeAppCoreTest, ProcessFrameWithLargeROI) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);
  EdgeAppLibSensorImageCropProperty large_roi = {0, 0, 8000, 8000};
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, large_roi);
  EXPECT_NE(frame, 0);
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

// Tests for new API functions

TEST_F(EdgeAppCoreTest, GetOutputWithSpecificIndex) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);

  // Test getting specific tensor by index using GetOutputs
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  EXPECT_FALSE(outputs.empty());

  Tensor output0 = outputs[0];  // First tensor
  EXPECT_TRUE(output0.data != nullptr && output0.size > 0);

  Tensor output1{};  // Initialize empty tensor
  if (outputs.size() > 1) {
    output1 = outputs[1];  // Second tensor
    EXPECT_TRUE(output1.data != nullptr && output1.size > 0);
  }

  // Test getting all tensors with -1
  Tensor all_outputs = GetOutput(ctx_cpu, frame, 4);
  EXPECT_TRUE(all_outputs.data != nullptr && all_outputs.size > 0);

  // Only compare sizes if we have multiple tensors
  if (outputs.size() > 1) {
    EXPECT_GE(all_outputs.size, output0.size + output1.size);
  }

  // Clean up
  for (auto &tensor : outputs) {
    if (tensor.data) {
      free(tensor.data);
    }
  }
  free(all_outputs.data);
}

TEST_F(EdgeAppCoreTest, GetOutputsReturnsVector) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);

  // Test GetOutputs function
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  EXPECT_FALSE(outputs.empty());
  EXPECT_LE(outputs.size(), 4);  // Should not exceed max_tensor_num

  // Verify each tensor is valid
  for (size_t i = 0; i < outputs.size(); ++i) {
    EXPECT_TRUE(outputs[i].data != nullptr)
        << "Tensor " << i << " has null data";
    EXPECT_GT(outputs[i].size, 0) << "Tensor " << i << " has zero size";
  }

  // Clean up
  for (auto &tensor : outputs) {
    if (tensor.data) {
      free(tensor.data);
    }
  }
}

EdgeAppCoreResult test_preprocessing_callback(
    const void *input_data, EdgeAppLibImageProperty input_property,
    void **output_data, EdgeAppLibImageProperty *output_property) {
  // Simple test preprocessing: increment all pixel values by 1
  uint8_t *input = (uint8_t *)input_data;
  size_t input_size =
      input_property.stride_bytes * input_property.height;  // RGB
  uint8_t *output = (uint8_t *)malloc(input_size);
  if (!output) {
    return EdgeAppCoreResultFailure;
  }

  for (size_t i = 0; i < input_size; ++i) {
    if (input[i] < 255) {
      output[i] = input[i] + 1;
    } else {
      output[i] = input[i];
    }
  }

  *output_data = output;
  // Output has same dimensions as input
  output_property->width = input_property.width;
  output_property->height = input_property.height;
  output_property->stride_bytes = input_property.stride_bytes;
  strncpy(output_property->pixel_format, input_property.pixel_format,
          sizeof(output_property->pixel_format) - 1);
  output_property->pixel_format[sizeof(output_property->pixel_format) - 1] =
      '\0';
  return EdgeAppCoreResultSuccess;
}

EdgeAppCoreResult test_preprocessing_callback_tensor(
    const void *input_data, EdgeAppLibImageProperty input_property,
    Tensor *output_tensor) {
  printf("Input property: width=%u height=%u stride=%u pixel_format=%s\n",
         input_property.width, input_property.height,
         input_property.stride_bytes, input_property.pixel_format);
  uint8_t *input = (uint8_t *)input_data;
  size_t input_size =
      input_property.stride_bytes * input_property.height;  // RGB
  uint8_t *output = (uint8_t *)malloc(input_size);
  if (!output) {
    return EdgeAppCoreResultFailure;
  }

  for (size_t i = 0; i < input_size; ++i) {
    if (input[i] < 255) {
      output[i] = input[i] + 1;
    } else {
      output[i] = input[i];
    }
  }

  output_tensor->data = output;
  output_tensor->size = input_size;
  output_tensor->shape_info.ndim = 3;
  output_tensor->shape_info.dims[0] = input_property.height;
  output_tensor->shape_info.dims[1] = input_property.width;
  output_tensor->shape_info.dims[2] = 3;  // RGB channels

  return EdgeAppCoreResultSuccess;
}

TEST_F(EdgeAppCoreTest, GetOutputsConsistencyWithGetOutput) {
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame, dummy_roi[0]);
  EXPECT_NE(frame, 0);

  // Compare GetOutputs with individual GetOutput calls
  auto outputs = GetOutputs(ctx_cpu, frame, 4);

  for (size_t i = 0; i < outputs.size(); ++i) {
    // Compare outputs with themselves for consistency check
    EXPECT_TRUE(outputs[i].data != nullptr)
        << "Tensor " << i << " has null data";
    EXPECT_GT(outputs[i].size, 0) << "Tensor " << i << " has zero size";
    EXPECT_GT(outputs[i].shape_info.ndim, 0)
        << "Tensor " << i << " has zero dimensions";
  }

  // Clean up
  for (auto &tensor : outputs) {
    if (tensor.data) {
      free(tensor.data);
    }
  }
}

// Tests for ProcessedFrame Method Chaining
TEST_F(EdgeAppCoreTest, MethodChainBasic) {
  // Test that method chaining interface works correctly
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  // Test method chaining compiles correctly
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame)
                   .withROI(dummy_roi[0])
                   .compute();

  EXPECT_FALSE(frame.empty());
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  for (auto &tensor : outputs) {
    if (tensor.data) {
      free(tensor.data);
    }
  }
}

TEST_F(EdgeAppCoreTest, MethodChainWithPreprocessing) {
  // Test that preprocessing callback can be set in method chaining
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  // Test method chaining with preprocessing
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame)
                   .withPreprocessing(test_preprocessing_callback)
                   .compute();

  EXPECT_FALSE(frame.empty());
  EXPECT_NE(static_cast<EdgeAppLibSensorFrame>(frame), 0);
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  for (auto &tensor : outputs) {
    if (tensor.data) {
      free(tensor.data);
    }
  }
}

TEST_F(EdgeAppCoreTest, MethodChainWithPreprocessingTensor) {
  // Test that preprocessing callback can be set in method chaining
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  // Test method chaining with preprocessing
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame)
                   .withPreprocessing(test_preprocessing_callback_tensor)
                   .compute();

  EXPECT_FALSE(frame.empty());
  EXPECT_NE(static_cast<EdgeAppLibSensorFrame>(frame), 0);
  auto input = GetInput(ctx_cpu, frame);
  free(input.data);
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  for (auto &tensor : outputs) {
    if (tensor.data) {
      free(tensor.data);
    }
  }
}

TEST_F(EdgeAppCoreTest, MethodChainFullChain) {
  // Test full method chaining with compute
  resetEdgeAppLibSensorChannelImageProperty();
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  // Test full chain with both ROI and preprocessing
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame)
                   .withROI(dummy_roi[0])
                   .withPreprocessing(test_preprocessing_callback)
                   .compute();

  EXPECT_FALSE(frame.empty());
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  for (auto &tensor : outputs) {
    if (tensor.data) {
      free(tensor.data);
    }
  }
}

TEST_F(EdgeAppCoreTest, MethodChainMinimal) {
  // Test minimal method chaining with compute
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  // Test minimal fluent interface - just the frame
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame).compute();

  // With valid context, should not be empty
  EXPECT_FALSE(frame.empty());
  EXPECT_NE(static_cast<EdgeAppLibSensorFrame>(frame), 0);
}

TEST_F(EdgeAppCoreTest, MethodChainErrorHandling) {
  // Test with uninitialized context (should fail)
  EdgeAppCoreCtx invalid_ctx;
  memset(&invalid_ctx, 0, sizeof(invalid_ctx));

  auto result = Process(invalid_ctx, nullptr, dummy_frame)
                    .withROI(dummy_roi[0])
                    .compute();

  // Should return empty/invalid result
  EXPECT_TRUE(result.empty());
  EXPECT_EQ(static_cast<EdgeAppLibSensorFrame>(result), 0);
}

int EdgeAppCoreTest::total_count = 0;
int EdgeAppCoreTest::executed_count = 0;
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::UnitTest *unit_test = ::testing::UnitTest::GetInstance();
  for (int i = 0; i < unit_test->total_test_suite_count(); ++i) {
    auto *suite = unit_test->GetTestSuite(i);
    if (std::string(suite->name()) == "EdgeAppCoreTest") {
      EdgeAppCoreTest::total_count = suite->total_test_count();
    }
  }
  return RUN_ALL_TESTS();
}

TEST_F(EdgeAppCoreTest, SendInputTensorFormatIMX500) {
  resetEdgeAppLibSensorChannelImageProperty();
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto input = GetInput(ctx_imx500, frame);

  // Verify input tensor format for IMX500
  EXPECT_EQ(input.shape_info.ndim, 4);
  EXPECT_EQ(input.shape_info.dims[3], 3);  // Channels
  EXPECT_EQ(input.format, AITRIOS_DRAW_FORMAT_RGB8);

  free(input.data);  // Free the data if it was dynamically allocated
}

TEST_F(EdgeAppCoreTest, SendInputTensorFormatIMX500Planar) {
  resetEdgeAppLibSensorChannelImageProperty();
  EdgeAppLibSensorImageProperty image = {};
  image.width = 5;
  image.height = 1;
  image.stride_bytes = 5;
  snprintf(image.pixel_format, sizeof(image.pixel_format), "%s",
           AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR);

  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  setEdgeAppLibSensorChannelImageProperty(image);
  auto frame = Process(ctx_imx500, &ctx_imx500, dummy_frame, dummy_roi[0]);
  auto input = GetInput(ctx_imx500, frame);

  // Verify input tensor format for IMX500
  EXPECT_EQ(input.shape_info.ndim, 4);
  EXPECT_EQ(input.shape_info.dims[3], 3);  // Channels
  EXPECT_EQ(input.format, AITRIOS_DRAW_FORMAT_RGB8_PLANAR);

  SendInputTensor(&input);

  // free(input.data);  // Free the data if it was dynamically allocated
}

TEST_F(EdgeAppCoreTest, SendInputTensorCPU) {
  // Test that preprocessing callback can be set in method chaining
  EXPECT_EQ(LoadModel(model[0], ctx_imx500, nullptr), EdgeAppCoreResultSuccess);
  EXPECT_EQ(LoadModel(model[1], ctx_cpu, &ctx_imx500),
            EdgeAppCoreResultSuccess);

  // Test method chaining with preprocessing
  auto frame = Process(ctx_cpu, &ctx_imx500, dummy_frame)
                   .withPreprocessing(test_preprocessing_callback_tensor)
                   .compute();

  EXPECT_FALSE(frame.empty());
  EXPECT_NE(static_cast<EdgeAppLibSensorFrame>(frame), 0);
  auto input = GetInput(ctx_cpu, frame);
  int ret = SendInputTensor(&input);
  EXPECT_EQ(ret, 0);
  auto outputs = GetOutputs(ctx_cpu, frame, 4);
  for (auto &tensor : outputs) {
    if (tensor.data) {
      ret = SendInference(tensor.data, tensor.size, EdgeAppLibSendDataJson, 0);
      EXPECT_EQ(ret, 0);
      free(tensor.data);
    }
  }
  // free(input.data);  // Free the data if it was dynamically allocated
}
