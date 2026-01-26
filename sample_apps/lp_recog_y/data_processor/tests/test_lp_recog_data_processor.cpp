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

#include <cstring>
#include <fstream>
#include <list>
#include <sstream>
#include <string>

#include "data_processor_api.hpp"
#include "lp_recog_utils.hpp"
#include "parson.h"
#include "sensor.h"
#include "testing_utils.hpp"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;

EdgeAppLibSensorStream s_stream = 0;
extern char lpd_imx500_model_id[AI_MODEL_BUNDLE_ID_SIZE];
extern float lpr_threshold;

#define MAX_PREDICTIONS_PROP \
  "ai_models_imx500.lp_detection.parameters.max_detections"
#define THRESHOLD_PROP "ai_models_imx500.lp_detection.parameters.threshold"
#define INPUT_WIDTH_PROP "ai_models_imx500.lp_detection.parameters.input_width"
#define INPUT_HEIGHT_PROP \
  "ai_models_imx500.lp_detection.parameters.input_height"
#define BBOX_NORM_PROP \
  "ai_models_imx500.lp_detection.parameters.bbox_normalization"

#define EPSILON 1e-4
#define MODEL_ID "ModelID"
#define DEVICE_ID "DeviceID"
#define BUF_IMAGE "Image"
#define BUF_TIME "T"
#define BUF_OUTPUT "O"
#define BUF_INFERENCE "Inferences"

class ConfigureAnalyzeFixtureTests : public ::testing::Test {
 protected:
  const char *output_tensor_lpd_path =
      "../../../test_data/output_tensor_lpd.jsonc";
  const char *config_json_path = "../../../test_data/custom_parameter.json";
  float *out_data = nullptr;
  size_t out_size = 0;
  uint32_t num_array_elements = 0;
  JSON_Value *config_json_val = nullptr;
  JSON_Object *config_json_object = nullptr;
  char *config = nullptr;
  char network_id[37];  // UUID

  void LoadConfigJson() {
    config_json_val = json_parse_file(config_json_path);
    ASSERT_FALSE(config_json_val == NULL);
    config_json_object = json_object(config_json_val);
    ASSERT_FALSE(config_json_object == NULL);

    // Generate UUID for network_id
    generate_random_uuid(network_id);
    JSON_Status stat = json_object_dotset_string(
        config_json_object, "ai_models_imx500.lp_detection.ai_model_bundle_id",
        network_id);
    ASSERT_EQ(stat, JSONSuccess);

    config = json_serialize_to_string_pretty(config_json_val);
    ASSERT_FALSE(config == NULL);
  }

  void LoadTensorData() {
    // Read file content and remove comments for JSONC support
    FILE *file = fopen(output_tensor_lpd_path, "r");
    ASSERT_NE(file, nullptr)
        << "Failed to open tensor file: " << output_tensor_lpd_path;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size + 1);
    ASSERT_NE(file_content, nullptr);

    size_t read_size = fread(file_content, 1, file_size, file);
    file_content[read_size] = '\0';
    fclose(file);

    // Remove line comments (// ...) for JSONC compatibility
    std::string content_str(file_content);
    free(file_content);

    std::string cleaned_content;
    std::istringstream iss(content_str);
    std::string line;

    while (std::getline(iss, line)) {
      size_t comment_pos = line.find("//");
      if (comment_pos != std::string::npos) {
        line = line.substr(0, comment_pos);
      }
      // Remove trailing whitespace
      while (!line.empty() && std::isspace(line.back())) {
        line.pop_back();
      }
      cleaned_content += line + "\n";
    }

    JSON_Value *output_tensor_val = json_parse_string(cleaned_content.c_str());
    ASSERT_FALSE(output_tensor_val == NULL);
    char *output_tensor = json_serialize_to_string(output_tensor_val);
    ASSERT_FALSE(output_tensor == NULL);

    out_data = StringToFloatArray(output_tensor, &num_array_elements);
    ASSERT_FALSE(out_data == NULL);
    out_size = num_array_elements;

    json_value_free(output_tensor_val);
    json_free_serialized_string(output_tensor);
  }

  void SetUp() override {
    LoadConfigJson();
    LoadTensorData();
  }

  void TearDown() override {
    if (config_json_val) {
      json_value_free(config_json_val);
    }
    if (config) {
      json_free_serialized_string(config);
    }
    if (out_data) {
      free(out_data);
    }
    SensorCoreExit(0);
  }

  char *generate_random_uuid(char uuid[128]) {
    const char *hex_chars = "0123456789abcdef";
    srand((unsigned int)time(NULL));

    for (int i = 0; i < 32; i++) {
      uuid[i] = hex_chars[rand() % 16];
    }
    uuid[32] = '\0';
    return uuid;
  }

  char *output_tensor = nullptr;
  uint64_t nanoseconds = 0;
};

void check_values(JSON_Object *json) {
  // extract parameters
  int max_detections = json_object_dotget_number(json, MAX_PREDICTIONS_PROP);
  float threshold = json_object_dotget_number(json, THRESHOLD_PROP);
  int input_width = json_object_dotget_number(json, INPUT_WIDTH_PROP);
  int input_height = json_object_dotget_number(json, INPUT_HEIGHT_PROP);
  extern DataProcessorCustomParam_LPD detection_param;
  // assert equalities
  EXPECT_EQ(max_detections, detection_param.max_detections);
  EXPECT_EQ(threshold, detection_param.threshold);
  EXPECT_EQ(input_width, detection_param.input_width);
  EXPECT_EQ(input_height, detection_param.input_height);
}

TEST(Initialize, InitializeTest) {
  DataProcessorResultCode res = DataProcessorInitialize();
  EXPECT_EQ(res, kDataProcessorOk);
}

TEST(ResetState, ResetStateTest) {
  DataProcessorResultCode res = DataProcessorResetState();
  EXPECT_EQ(res, kDataProcessorOk);
}

TEST(Finalize, FinalizeTest) {
  DataProcessorResultCode res = DataProcessorFinalize();
  EXPECT_EQ(res, kDataProcessorOk);
}

TEST_F(ConfigureAnalyzeFixtureTests, DefaultValuesTest) {
  // Create a minimal configuration without parameters to test default values
  const char *minimal_config = R"({
    "ai_models_imx500": {
      "lp_detection": {
        "ai_model_bundle_id": "sample_network_id"
      }
    },
    "ai_models_cpu": {
      "lp_recognition": {}
    },
    "metadata_settings": {
      "format": 0
    }
  })";

  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)minimal_config, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);  // Returns InvalidParam because
                                               // defaults are set

  // Check that default values are applied correctly
  extern DataProcessorCustomParam_LPD detection_param;
  EXPECT_EQ(detection_param.max_detections, DEFAULT_MAX_DETECTIONS_IMX500);
  EXPECT_FLOAT_EQ(detection_param.threshold, DEFAULT_THRESHOLD_IMX500);
  EXPECT_EQ(detection_param.input_width, DEFAULT_INPUT_TENSOR_WIDTH_IMX500);
  EXPECT_EQ(detection_param.input_height, DEFAULT_INPUT_TENSOR_HEIGHT_IMX500);
  EXPECT_TRUE(detection_param.bbox_normalized);  // Default should be true

  // Check CPU model defaults
  EXPECT_FLOAT_EQ(lpr_threshold, 0.5f);  // Default CPU threshold should be 0.5

  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, PartialParametersTest) {
  // Test with only some parameters specified to verify defaults are applied to
  // missing ones
  const char *partial_config = R"({
    "ai_models_imx500": {
      "lp_detection": {
        "ai_model_bundle_id": "sample_network_id",
        "parameters": {
          "threshold": 0.5
        }
      }
    },
    "ai_models_cpu": {
      "lp_recognition": {}
    },
    "metadata_settings": {
      "format": 0
    }
  })";

  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)partial_config, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);  // Returns InvalidParam because
                                               // defaults are set

  // Check that default values are applied to missing parameters
  extern DataProcessorCustomParam_LPD detection_param;
  EXPECT_EQ(detection_param.max_detections,
            DEFAULT_MAX_DETECTIONS_IMX500);          // Default applied
  EXPECT_FLOAT_EQ(detection_param.threshold, 0.5f);  // User specified
  EXPECT_EQ(detection_param.input_width,
            DEFAULT_INPUT_TENSOR_WIDTH_IMX500);  // Default applied
  EXPECT_EQ(detection_param.input_height,
            DEFAULT_INPUT_TENSOR_HEIGHT_IMX500);  // Default applied
  EXPECT_TRUE(detection_param.bbox_normalized);   // Default applied

  // Check CPU model parameters
  EXPECT_FLOAT_EQ(lpr_threshold,
                  DEFAULT_THRESHOLD_CPU);  // Default applied (0.5)

  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, WrongJsonValueTest) {
  char *output = NULL;
  const char *config_mod = "Not a json file";
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_FALSE(out_value == NULL);
  json_value_free(out_value);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, ThresholdOutOfRangeTest) {
  JSON_Status stat = json_object_dotset_number(
      config_json_object, "ai_models_imx500.lp_detection.parameters.threshold",
      1.5);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  extern DataProcessorCustomParam_LPD detection_param;
  EXPECT_EQ((float)DEFAULT_THRESHOLD_IMX500, detection_param.threshold);
  EXPECT_EQ(res, kDataProcessorOutOfRange);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, InvalidAIModelIMX500ObjectTest) {
  JSON_Status stat =
      json_object_dotset_number(config_json_object, "ai_models_imx500", 1.5);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, MaxDetectionsOverwriteNegative) {
  JSON_Status stat =
      json_object_dotset_number(config_json_object, MAX_PREDICTIONS_PROP, -1);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOutOfRange);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, InputWidthOverwriteNegative) {
  JSON_Status stat =
      json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, -1);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOutOfRange);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, InputHeightOverwriteNegative) {
  JSON_Status stat =
      json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, -1);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOutOfRange);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, ParameterInvalidError) {
  std::list<std::string> parameters = {MAX_PREDICTIONS_PROP, THRESHOLD_PROP,
                                       INPUT_WIDTH_PROP, INPUT_HEIGHT_PROP};
  for (const std::string &parameter : parameters) {
    JSON_Status stat =
        json_object_dotremove(config_json_object, parameter.c_str());
    ASSERT_EQ(stat, JSONSuccess);
    const char *config_mod = json_serialize_to_string_pretty(config_json_val);
    char *output = NULL;
    DataProcessorResultCode res =
        DataProcessorConfigure((char *)config_mod, &output);
    // Should return InvalidParam when extractors can't find required parameters
    EXPECT_EQ(res, kDataProcessorInvalidParam);
    json_free_serialized_string((char *)config_mod);
    if (output) free(output);

    // Restore the parameter
    if (config_json_val) {
      json_value_free(config_json_val);
    }
    if (config) {
      json_free_serialized_string(config);
    }
    LoadConfigJson();
  }
}

TEST_F(ConfigureAnalyzeFixtureTests, AIModelCPUNullTest) {
  // Set the AI model path to null
  JSON_Status stat = json_object_dotremove(config_json_object, "ai_models_cpu");
  ASSERT_EQ(stat, JSONSuccess);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);

  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, AIModelCPUParameterNullTest) {
  // Set the AI model path to null
  JSON_Status stat = json_object_dotremove(
      config_json_object, "ai_models_cpu.lp_recognition.parameters");
  ASSERT_EQ(stat, JSONSuccess);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);

  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Verify that default threshold is applied when parameters section is removed
  EXPECT_FLOAT_EQ(lpr_threshold, DEFAULT_THRESHOLD_CPU);

  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, AIModelCPUThresholdNullTest) {
  // Remove only the threshold parameter, keeping the parameters section
  JSON_Status stat = json_object_dotremove(
      config_json_object, "ai_models_cpu.lp_recognition.parameters.threshold");
  ASSERT_EQ(stat, JSONSuccess);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);

  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Verify that default threshold is applied when threshold parameter is
  // missing
  EXPECT_FLOAT_EQ(lpr_threshold, DEFAULT_THRESHOLD_CPU);

  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, CorrectLPDAnalyzeJsonTest) {
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  printf("out_size=%zu\n", out_size);

  LPDataProcessorAnalyzeParam param;
  LPAnalysisParam lp_param;
  EdgeAppLibSensorImageCropProperty roi = {0, 0, 300, 300};
  lp_param.roi = &roi;

  // Setup test tensor data
  EdgeAppCore::Tensor test_tensor;
  test_tensor.data = out_data;  // Use the loaded tensor data
  test_tensor.size = out_size * sizeof(float);
  test_tensor.type = EdgeAppCore::TensorTypeFloat32;
  test_tensor.timestamp = 123456789;
  strcpy(test_tensor.name, "test_tensor");

  // Setup shape info for tensor (assuming 4D tensor: batch, height, width,
  // channels)
  test_tensor.shape_info.ndim = 4;
  test_tensor.shape_info.dims[0] = 1;  // batch size
  test_tensor.shape_info.dims[1] = 1;  // height
  test_tensor.shape_info.dims[2] = 4;  // width (number of detections)
  test_tensor.shape_info.dims[3] =
      6;  // channels (x1, y1, x2, y2, score, class)

  lp_param.tensor = &test_tensor;
  param.app_specific = &lp_param;

  res = LPDDataProcessorAnalyze(out_data, out_size, &param);
  EXPECT_EQ(res, kDataProcessorOk);
  EXPECT_EQ(lp_param.roi->left, 30);
  EXPECT_EQ(lp_param.roi->top, 30);
  EXPECT_EQ(lp_param.roi->width, 30);
  EXPECT_EQ(lp_param.roi->height, 15);  // Only upper half
}

TEST_F(ConfigureAnalyzeFixtureTests, NullParamLPAnalyzeTest) {
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  res = LPDDataProcessorAnalyze(out_data, num_array_elements, nullptr);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
}

TEST_F(ConfigureAnalyzeFixtureTests, DataProcessorGetDataTypeJson) {
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  EdgeAppLibSendDataType data_type = DataProcessorGetDataType();
  EXPECT_EQ(data_type, EdgeAppLibSendDataJson);
}

// LPR Data Processor Test Fixture
class LPRDataProcessorTestFixture : public ::testing::Test {
 protected:
  const char *output_tensor_lpr_path =
      "../../../test_data/output_tensor_lpr.jsonc";
  float *lpr_data = nullptr;
  size_t lpr_size = 0;
  uint32_t num_array_elements = 0;
  char *output_data = nullptr;
  uint32_t output_size = 0;

  void LoadLPRTensorData() {
    // Read file content and remove comments for JSONC support
    FILE *file = fopen(output_tensor_lpr_path, "r");
    ASSERT_NE(file, nullptr)
        << "Failed to open LPR tensor file: " << output_tensor_lpr_path;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size + 1);
    ASSERT_NE(file_content, nullptr);

    size_t read_size = fread(file_content, 1, file_size, file);
    file_content[read_size] = '\0';
    fclose(file);

    // Remove line comments (// ...) for JSONC compatibility
    std::string content_str(file_content);
    free(file_content);

    std::string cleaned_content;
    std::istringstream iss(content_str);
    std::string line;

    while (std::getline(iss, line)) {
      size_t comment_pos = line.find("//");
      if (comment_pos != std::string::npos) {
        line = line.substr(0, comment_pos);
      }
      // Remove trailing whitespace
      while (!line.empty() && std::isspace(line.back())) {
        line.pop_back();
      }
      cleaned_content += line + "\n";
    }

    JSON_Value *output_tensor_val = json_parse_string(cleaned_content.c_str());
    ASSERT_FALSE(output_tensor_val == NULL);
    char *output_tensor = json_serialize_to_string(output_tensor_val);
    ASSERT_FALSE(output_tensor == NULL);

    constexpr size_t kTimeSteps = 28;
    constexpr size_t kVocabSize = 248;

    lpr_data = StringToFloatArray(output_tensor, &num_array_elements);
    ASSERT_EQ(num_array_elements, kVocabSize);
    ASSERT_FALSE(lpr_data == NULL);
    lpr_size = num_array_elements;

    float *original = lpr_data;
    float *expanded = (float *)malloc(kTimeSteps * kVocabSize * sizeof(float));

    for (size_t t = 0; t < kTimeSteps; ++t) {
      memcpy(expanded + t * kVocabSize, original, kVocabSize * sizeof(float));
    }

    free(original);
    lpr_data = expanded;
    lpr_size = kTimeSteps * kVocabSize;

    json_value_free(output_tensor_val);
    json_free_serialized_string(output_tensor);
  }

  void SetUp() override { LoadLPRTensorData(); }

  void TearDown() override {
    if (lpr_data) {
      free(lpr_data);
    }
    if (output_data) {
      free(output_data);
    }
    SensorCoreExit(0);
  }
};

TEST_F(LPRDataProcessorTestFixture, LPRDataProcessorAnalyzeTest) {
  // Check initial threshold value
  printf("Initial lpr_threshold: %f\n", lpr_threshold);

  DataProcessorResultCode res = LPRDataProcessorAnalyze(
      lpr_data, lpr_size * sizeof(float), &output_data, &output_size);

  EXPECT_EQ(res, kDataProcessorOk);
  ASSERT_NE(output_data, nullptr);
  ASSERT_GT(output_size, 0);

  printf("LPR Analysis result: %s\n", output_data);

  // Parse and verify it's valid JSON
  JSON_Value *result_json = json_parse_string(output_data);
  ASSERT_NE(result_json, nullptr);

  // The result should contain license plate interpretation
  std::string result_str(output_data);

  std::string expected_str = "\"\xE6\x89\x80\"";
  EXPECT_EQ(result_str, expected_str) << "Actual result: " << result_str
                                      << "\nExpected result: " << expected_str;

  json_value_free(result_json);
}

TEST_F(LPRDataProcessorTestFixture, LPRDataProcessorAnalyzeNullInputTest) {
  DataProcessorResultCode res = LPRDataProcessorAnalyze(
      nullptr, lpr_size * sizeof(float), &output_data, &output_size);

  EXPECT_EQ(res, kDataProcessorInvalidParam);
}

TEST_F(LPRDataProcessorTestFixture, LPRDataProcessorAnalyzeNullOutputTest) {
  DataProcessorResultCode res = LPRDataProcessorAnalyze(
      lpr_data, lpr_size * sizeof(float), nullptr, &output_size);

  EXPECT_EQ(res, kDataProcessorInvalidParam);
}
char *generate_random_uuid(char uuid[128]) {
  const char *hex_chars = "0123456789abcdef";
  srand((unsigned int)time(NULL));

  for (int i = 0; i < 32; i++) {
    uuid[i] = hex_chars[rand() % 16];
  }
  uuid[32] = '\0';
  return uuid;
}

TEST_F(LPRDataProcessorTestFixture,
       LPRDataProcessorAnalyzeWithConfiguredThresholdTest) {
  // Load and configure with custom configuration
  const char *config_json_path = "../../../test_data/custom_parameter.json";
  JSON_Value *config_json_val = json_parse_file(config_json_path);
  ASSERT_NE(config_json_val, nullptr);

  JSON_Object *config_json_object = json_object(config_json_val);
  ASSERT_NE(config_json_object, nullptr);

  char network_id[37];
  generate_random_uuid(network_id);
  JSON_Status stat = json_object_dotset_string(
      config_json_object, "ai_models_imx500.lp_detection.ai_model_bundle_id",
      network_id);
  ASSERT_EQ(stat, JSONSuccess);

  char *config = json_serialize_to_string_pretty(config_json_val);
  ASSERT_NE(config, nullptr);

  // Configure the processor
  char *output = NULL;
  DataProcessorResultCode config_res = DataProcessorConfigure(config, &output);
  EXPECT_EQ(config_res, kDataProcessorOk);

  // Check that threshold was updated
  printf("Configured lpr_threshold: %f\n", lpr_threshold);
  EXPECT_FLOAT_EQ(lpr_threshold, 0.5f);

  printf("lpr_size=%zu\n", lpr_size);

  DataProcessorResultCode res = LPRDataProcessorAnalyze(
      lpr_data, lpr_size * sizeof(float), &output_data, &output_size);

  EXPECT_EQ(res, kDataProcessorOk);
  ASSERT_NE(output_data, nullptr);
  ASSERT_GT(output_size, 0);

  printf("LPR Analysis result with configured threshold: %s\n", output_data);

  // Parse and verify it's valid JSON
  JSON_Value *result_json = json_parse_string(output_data);
  ASSERT_NE(result_json, nullptr);

  // The result should contain license plate interpretation
  std::string result_str(output_data);
  std::string expected_str = "\"\xE6\x89\x80\"";
  EXPECT_EQ(result_str, expected_str) << "Actual result: " << result_str
                                      << "\nExpected result: " << expected_str;

  json_value_free(result_json);
  json_value_free(config_json_val);
  json_free_serialized_string(config);
  if (output) free(output);
}
