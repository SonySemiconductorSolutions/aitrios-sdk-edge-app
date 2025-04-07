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

#include <list>
#include <string>

#include "data_processor_api.hpp"
#include "flatbuffers/flatbuffers.h"
#include "parson.h"
#include "segmentation_utils.hpp"
#include "semantic_segmentation_generated.h"
#include "sensor.h"
#include "testing_utils.hpp"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;

EdgeAppLibSensorStream s_stream = 0;

#define INPUT_WIDTH_PROP "ai_models.segmentation.parameters.input_width"
#define INPUT_HEIGHT_PROP "ai_models.segmentation.parameters.input_height"

#define EPSILON 1e-4

class CommonTestFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    config_json_val = json_parse_file(
        "../../../test_data/segmentation_custom_parameter.json");
    config_json_object = json_object(config_json_val);

    generate_random_uuid(network_id);
    json_object_dotset_string(config_json_object,
                              "ai_models.segmentation.ai_model_bundle_id",
                              network_id);

    config = json_serialize_to_string(config_json_val);

    output_tensor_val =
        json_parse_file_with_comments("../../../test_data/output_tensor.jsonc");

    output_tensor = json_serialize_to_string(output_tensor_val);

    out_data = StringToFloatArray((char *)output_tensor, &num_array_elements);
  }

  void TearDown() override {
    json_value_free(config_json_val);
    json_value_free(output_tensor_val);
    json_free_serialized_string(output_tensor);
    json_free_serialized_string(config);
    free(out_data);
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
  JSON_Value *config_json_val = nullptr;
  JSON_Value *output_tensor_val = nullptr;
  JSON_Object *config_json_object = nullptr;
  char *config = nullptr;
  char *output_tensor = nullptr;
  float *out_data = nullptr;
  uint32_t num_array_elements = 0;
  uint32_t out_size = 0;
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
};

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

TEST_F(CommonTestFixture, CorrectConfigurationTest) {
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure(config, &output);
  int input_width =
      json_object_dotget_number(config_json_object, INPUT_WIDTH_PROP);
  int input_height =
      json_object_dotget_number(config_json_object, INPUT_HEIGHT_PROP);
  extern DataProcessorCustomParam seg_param;
  EXPECT_EQ(input_width, seg_param.inputWidth);
  EXPECT_EQ(input_height, seg_param.inputHeight);
  EXPECT_EQ(res, kDataProcessorOk);
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle = {};
  SensorStreamGetProperty(
      0, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY, &ai_model_bundle,
      sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty));
  ASSERT_EQ(memcmp(ai_model_bundle.ai_model_bundle_id, network_id,
                   sizeof(network_id)),
            0);
}

TEST_F(CommonTestFixture, WrongJsonValueTest) {
  char *output = NULL;
  const char *config_mod = "Not a json file";
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  free(output);
}

TEST_F(CommonTestFixture, ParameterInvalidError) {
  std::list<std::string> parametersList = {"threshold", INPUT_WIDTH_PROP,
                                           INPUT_HEIGHT_PROP};
  float def_params[3] = {DEFAULT_THRESHOLD, DEFAULT_SS_INPUT_TENSOR_WIDTH,
                         DEFAULT_SS_INPUT_TENSOR_HEIGHT};

  const char *config_mod;
  DataProcessorResultCode res;
  JSON_Status stat;
  for (const std::string &parameter : parametersList) {
    stat = json_object_dotremove(config_json_object, parameter.c_str());
  }
  config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  res = DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  extern DataProcessorCustomParam seg_param;
  EXPECT_NEAR(DEFAULT_THRESHOLD, seg_param.threshold, EPSILON);
  EXPECT_EQ(DEFAULT_SS_INPUT_TENSOR_WIDTH, seg_param.inputWidth);
  EXPECT_EQ(DEFAULT_SS_INPUT_TENSOR_HEIGHT, seg_param.inputHeight);
  JSON_Value *out_value = json_parse_string(output);
  JSON_Object *json = json_object(out_value);
  EXPECT_EQ(json_object_dotget_number(json, INPUT_HEIGHT_PROP),
            DEFAULT_SS_INPUT_TENSOR_HEIGHT);

  json_value_free(out_value);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(CommonTestFixture, InputWidthOverwriteNegative) {
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

TEST_F(CommonTestFixture, InputHeightOverwriteNegative) {
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

TEST_F(CommonTestFixture, HeaderIdFailTest) {
  JSON_Status stat = json_object_remove(config_json_object, "header");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOk);
  json_free_serialized_string((char *)config_mod);
}

TEST_F(CommonTestFixture, CorrectAnalyzeTest) {
  char *output = NULL;
  DataProcessorConfigure((char *)config, &output);
  uint32_t in_size = 16;
  char *p_out_buf = nullptr;
  uint32_t out_size = 0;
  DataProcessorResultCode res = DataProcessorAnalyze(
      out_data, num_array_elements * sizeof(float), &p_out_buf, &out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  free(p_out_buf);
}

TEST_F(CommonTestFixture, CorrectAnalyzeOutputTest) {
  char *output = NULL;
  extern DataProcessorCustomParam seg_param;
  seg_param.inputHeight = 4;
  seg_param.inputWidth = 4;
  DataProcessorConfigure(config, &output);
  uint32_t in_size = 16;
  char *p_out_buf = nullptr;
  uint32_t out_size = 0;

  DataProcessorResultCode res = DataProcessorAnalyze(
      out_data, num_array_elements * sizeof(float), &p_out_buf, &out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  std::vector<uint16_t> expected_res = {1, 2, 2, 2, 2, 3, 4, 4,
                                        1, 1, 1, 4, 3, 3, 3, 1};

  auto flatBufferOut =
      flatbuffers::GetRoot<SmartCamera::SemanticSegmentationTop>(p_out_buf);

  const auto *class_id_map = flatBufferOut->perception()->class_id_map();
  EXPECT_NE(class_id_map, nullptr);
  EXPECT_EQ(class_id_map->size(), expected_res.size());

  for (size_t i = 0; i < class_id_map->size(); ++i) {
    EXPECT_EQ(class_id_map->Get(i), expected_res[i]);
  }
  free(p_out_buf);
}

TEST_F(CommonTestFixture, AIModelsNotNullTest) {
  JSON_Status stat = json_object_remove(config_json_object, "ai_models");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  JSON_Value *value = json_parse_string(output);
  EXPECT_TRUE(value);
  free(output);
  json_value_free(value);
  json_free_serialized_string((char *)config_mod);
}

TEST_F(CommonTestFixture, AiModelBundleIdNotNullTest) {
  JSON_Status stat = json_object_dotremove(
      config_json_object, "ai_models.segmentation.ai_model_bundle_id");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParamSetError);
  JSON_Value *value = json_parse_string(output);
  EXPECT_TRUE(value);
  free(output);
  json_value_free(value);
  json_free_serialized_string((char *)config_mod);
}

TEST_F(CommonTestFixture, NullTensorAnalyzeTest) {
  uint32_t in_size;
  char *out_data = nullptr;
  uint32_t out_size = 0;  // Initialize out_size
  float *output_tensor = NULL;
  DataProcessorResultCode res = DataProcessorAnalyze(
      output_tensor, num_array_elements * sizeof(float), &out_data, &out_size);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
}

TEST_F(CommonTestFixture, ParameterInvalidAnalyzeOutputTest) {
  char *output = NULL;
  extern DataProcessorCustomParam seg_param;
  seg_param.inputHeight = 5;  // invalid parameter.
  seg_param.inputWidth = 4;
  char *p_out_buf = nullptr;
  uint32_t out_size = 0;

  DataProcessorResultCode res = DataProcessorAnalyze(
      out_data, num_array_elements * sizeof(float), &p_out_buf, &out_size);
  EXPECT_EQ(res, kDataProcessorOther);
}
