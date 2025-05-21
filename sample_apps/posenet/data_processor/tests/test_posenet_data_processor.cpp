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

#include <fstream>
#include <list>
#include <string>

#include "data_processor_api.hpp"
#include "flatbuffers/flatbuffers.h"
#include "parson.h"
#include "poseestimation_generated.h"
#include "posenet_utils.hpp"
#include "sensor.h"
#include "testing_utils.hpp"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;

EdgeAppLibSensorStream s_stream = 0;

#define INPUT_WIDTH_PROP "ai_models.posenet.parameters.input_width"
#define INPUT_HEIGHT_PROP "ai_models.posenet.parameters.input_height"
#define MAX_PREDICTIONS_PROP "ai_models.posenet.parameters.max_pose_detections"
#define SCORE_THRESHOLD_PROP "ai_models.posenet.parameters.score_threshold"
#define OUTPUT_WIDTH_PROP "ai_models.posenet.parameters.output_width"
#define OUTPUT_HEIGHT_PROP "ai_models.posenet.parameters.output_height"
#define IOU_THRESHOLD_PROP "ai_models.posenet.parameters.iou_threshold"
#define NMS_RADIUS_PROP "ai_models.posenet.parameters.nms_radius"
#define HEATMAP_INDEX_PRO "ai_models.posenet.parameters.heatmap_index"
#define OFFSET_INDEX_PRO "ai_models.posenet.parameters.offset_index"
#define FD_INDEX_PRO "ai_models.posenet.parameters.forward_displacement_index"
#define BD_INDEX_PRO "ai_models.posenet.parameters.backward_displacement_index"

#define EPSILON 1e-4

class ConfigureAnalyzeFixtureTests : public ::testing::Test {
 protected:
  void SetUp() override {
    config_json_val =
        json_parse_file("../../../test_data/custom_parameter.json");
    nanoseconds = 1726161043914069133;
    config_json_object = json_object(config_json_val);
    generate_random_uuid(network_id);
    json_object_dotset_string(
        config_json_object, "ai_models.posenet.ai_model_bundle_id", network_id);

    config = json_serialize_to_string(config_json_val);

    uint32_t num_array_elements = 0;
    out_data = load_output_tensor(
        "../../../test_data/westworld_out_w481_h353.bin", &num_array_elements);
    out_size = num_array_elements;
  }

  void TearDown() override {
    json_value_free(config_json_val);
    json_value_free(output_tensor_val);
    json_free_serialized_string(output_tensor);
    json_free_serialized_string(config);
    delete[] out_data;
    SensorCoreExit(0);
  }

  float *load_output_tensor(const std::string &in_bin_file,
                            uint32_t *out_size) {
    float *data = nullptr;
    std::ifstream ifs(in_bin_file, std::ios::in | std::ios::binary);

    if (!ifs) {
      return nullptr;
    }

    ifs.seekg(0, ifs.end);
    uint32_t file_size = ifs.tellg();
    ifs.seekg(0, ifs.beg);

    uint32_t num_floats = file_size / sizeof(float);

    data = new float[num_floats];
    if (!data) {
      ifs.close();
      return nullptr;
    }

    ifs.read(reinterpret_cast<char *>(data), file_size);
    ifs.close();
    *out_size = file_size;
    return data;
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
  JSON_Object *config_json_object = nullptr;
  JSON_Value *output_tensor_val = nullptr;
  char *output_tensor = nullptr;
  char *config = nullptr;
  uint32_t num_array_elements = 0;
  uint32_t out_size = 0;
  float *out_data = nullptr;
  uint64_t nanoseconds = 0;
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
};

void check_values(JSON_Object *json) {
  int max_predictions = json_object_dotget_number(json, MAX_PREDICTIONS_PROP);
  extern DataProcessorCustomParam g_posenet_param;
  EXPECT_EQ(max_predictions, g_posenet_param.max_pose_detections);
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

TEST_F(ConfigureAnalyzeFixtureTests, CorrectConfigurationTest) {
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  printf("config: %s\n", config);
  EXPECT_EQ(res, kDataProcessorOk);
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;
  SensorStreamGetProperty(
      0, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY, &ai_model_bundle,
      sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty));
  printf("ai_model_bundle.ai_model_bundle_id: %s\n",
         ai_model_bundle.ai_model_bundle_id);
  printf("network_id: %s\n", network_id);
  ASSERT_EQ(strncmp(ai_model_bundle.ai_model_bundle_id, network_id,
                    sizeof(network_id)),
            0);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigureTestFailWrongJsonValue) {
  const char *config_mod = "Not a json string";
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  EXPECT_TRUE(out_value != NULL);
  json_value_free(out_value);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigureTestFailWrongJsonObject) {
  const char *config_mod = "[]";
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  EXPECT_FALSE(out_value == NULL);
  json_value_free(out_value);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests,
       ConfigureTestCorrectMaxPredictionsOverwriteLow) {
  JSON_Status stat =
      json_object_dotset_number(config_json_object, MAX_PREDICTIONS_PROP, 1);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOk);
  check_values(config_json_object);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigureTestFailParameterInvalidError) {
  std::list<std::string> parameters = {
      MAX_PREDICTIONS_PROP, INPUT_WIDTH_PROP,     INPUT_HEIGHT_PROP,
      MAX_PREDICTIONS_PROP, SCORE_THRESHOLD_PROP, OUTPUT_WIDTH_PROP,
      OUTPUT_HEIGHT_PROP,   IOU_THRESHOLD_PROP,   NMS_RADIUS_PROP,
      HEATMAP_INDEX_PRO,    OFFSET_INDEX_PRO,     FD_INDEX_PRO};
  for (const std::string &parameter : parameters)
    json_object_dotremove(config_json_object, parameter.c_str());

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  EXPECT_TRUE(out_value != NULL);
  extern DataProcessorCustomParam g_posenet_param;
  JSON_Object *json = json_object(out_value);
  EXPECT_EQ(json_object_dotget_number(json, MAX_PREDICTIONS_PROP),
            g_posenet_param.max_pose_detections);
  EXPECT_EQ(CST_POSENET_MAX_POSE_DETECTIONS,
            g_posenet_param.max_pose_detections);
  json_free_serialized_string((char *)config_mod);
  json_value_free(out_value);
  free(output);
}

/* -------------------------------------------------------- */
/*          Analyze                                         */
/* -------------------------------------------------------- */
TEST_F(ConfigureAnalyzeFixtureTests, ConfigureTestCorrect) {
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  check_values(config_json_object);
  EXPECT_EQ(res, kDataProcessorOk);
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle = {};
  SensorStreamGetProperty(
      0, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY, &ai_model_bundle,
      sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty));
  ASSERT_EQ(strncmp(ai_model_bundle.ai_model_bundle_id, network_id,
                    strlen(ai_model_bundle.ai_model_bundle_id)),
            0);
}

TEST_F(ConfigureAnalyzeFixtureTests, AnalyzeTestFailNullTensor) {
  uint32_t in_size;
  char *p_out_buf = nullptr;
  uint32_t p_out_size = 0;
  float *output_tensor = NULL;
  DataProcessorResultCode res =
      DataProcessorAnalyze(output_tensor, in_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
}

TEST_F(ConfigureAnalyzeFixtureTests, AnalyzeTestSuccess) {
  uint32_t in_size;
  char *p_out_buf = nullptr;
  uint32_t p_out_size = 0;
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  free(p_out_buf);
}

TEST_F(ConfigureAnalyzeFixtureTests, AIModelsNotNullTest) {
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

TEST_F(ConfigureAnalyzeFixtureTests, AiModelBundleIdNotNullTest) {
  JSON_Status stat = json_object_dotremove(
      config_json_object, "ai_models.posenet.ai_model_bundle_id");
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

TEST_F(ConfigureAnalyzeFixtureTests, CorrectAnalyzeJsonTest) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf = nullptr;
  uint32_t p_out_size = 0;

  printf("out_size=%d\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  ASSERT_NE(p_out_buf, nullptr);
  ASSERT_GT(p_out_size, 0);

  JSON_Value *out_json = json_parse_string(p_out_buf);

  free(output);

  json_value_free(out_json);
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
}
