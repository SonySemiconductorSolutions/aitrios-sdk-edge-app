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
#include "detection_utils.hpp"
#include "flatbuffers/flatbuffers.h"
#include "objectdetection_generated.h"
#include "parson.h"
#include "sensor.h"
#include "testing_utils.hpp"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;

EdgeAppLibSensorStream s_stream = 0;

#define MAX_PREDICTIONS_PROP "ai_models.detection.parameters.max_detections"
#define THRESHOLD_PROP "ai_models.detection.parameters.threshold"
#define INPUT_WIDTH_PROP "ai_models.detection.parameters.input_width"
#define INPUT_HEIGHT_PROP "ai_models.detection.parameters.input_height"

#define EPSILON 1e-4
#define MODEL_ID "ModelID"
#define DEVICE_ID "DeviceID"
#define BUF_IMAGE "Image"
#define BUF_TIME "T"
#define BUF_OUTPUT "O"
#define BUF_INFERENCE "Inferences"

class ConfigureAnalyzeFixtureTests : public ::testing::Test {
 protected:
  void SetUp() override {
    config_json_val =
        json_parse_file("../../../test_data/custom_parameter.json");
    nanoseconds = 1726161043914069133;
    config_json_object = json_object(config_json_val);
    generate_random_uuid(network_id);
    json_object_dotset_string(config_json_object,
                              "ai_models.detection.ai_model_bundle_id",
                              network_id);

    config = json_serialize_to_string(config_json_val);

    JSON_Value *output_tensor_val =
        json_parse_file_with_comments("../../../test_data/output_tensor.jsonc");

    char *output_tensor = json_serialize_to_string(output_tensor_val);

    num_array_elements = 0;
    out_data = StringToFloatArray((char *)output_tensor, &num_array_elements);
    out_size = num_array_elements * sizeof(float);

    free(output_tensor);
    json_value_free(output_tensor_val);
  }

  void TearDown() override {
    json_value_free(config_json_val);
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
  JSON_Object *config_json_object = nullptr;
  char *config = nullptr;
  char *output_tensor = nullptr;
  uint32_t num_array_elements = 0;
  uint32_t out_size = 0;
  float *out_data = nullptr;
  uint64_t nanoseconds = 0;
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
};

void check_values(JSON_Object *json) {
  // extract parameters
  int max_detections = json_object_dotget_number(json, MAX_PREDICTIONS_PROP);
  float threshold = json_object_dotget_number(json, THRESHOLD_PROP);
  int input_width = json_object_dotget_number(json, INPUT_WIDTH_PROP);
  int input_height = json_object_dotget_number(json, INPUT_HEIGHT_PROP);
  extern DataProcessorCustomParam ssd_param;
  // assert equalities
  EXPECT_EQ(max_detections, ssd_param.max_detections);
  EXPECT_EQ(threshold, ssd_param.threshold);
  EXPECT_EQ(input_width, ssd_param.input_width);
  EXPECT_EQ(input_height, ssd_param.input_height);
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
  check_values(config_json_object);
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
      config_json_object, "ai_models.detection.parameters.threshold", 1.5);
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  extern DataProcessorCustomParam ssd_param;
  EXPECT_EQ((float)DEFAULT_THRESHOLD, ssd_param.threshold);
  EXPECT_EQ(res, kDataProcessorOutOfRange);
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
  for (const std::string &parameter : parameters)
    json_object_dotremove(config_json_object, parameter.c_str());
  char *config_mod = json_serialize_to_string(config_json_val);
  extern DataProcessorCustomParam ssd_param;
  ssd_param = {
      .max_detections = DEFAULT_MAX_DETECTIONS + 1,
      .threshold = DEFAULT_THRESHOLD + 1,
      .input_width = DEFAULT_SSD_INPUT_TENSOR_WIDTH + 1,
      .input_height = DEFAULT_SSD_INPUT_TENSOR_HEIGHT + 1,
  };
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure(config_mod, &output);
  json_free_serialized_string(config_mod);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  EXPECT_NEAR(DEFAULT_THRESHOLD, ssd_param.threshold, EPSILON);
  EXPECT_EQ(DEFAULT_MAX_DETECTIONS, ssd_param.max_detections);
  EXPECT_EQ(DEFAULT_SSD_INPUT_TENSOR_WIDTH, ssd_param.input_width);
  EXPECT_EQ(DEFAULT_SSD_INPUT_TENSOR_HEIGHT, ssd_param.input_height);
  JSON_Value *out_value = json_parse_string(output);
  free(output);
  EXPECT_TRUE(out_value != NULL);
  JSON_Object *json = json_object(out_value);
  EXPECT_NEAR(json_object_dotget_number(json, THRESHOLD_PROP),
              ssd_param.threshold, EPSILON);
  EXPECT_EQ(json_object_dotget_number(json, MAX_PREDICTIONS_PROP),
            ssd_param.max_detections);
  EXPECT_EQ(json_object_dotget_number(json, INPUT_WIDTH_PROP),
            ssd_param.input_width);
  EXPECT_EQ(json_object_dotget_number(json, INPUT_HEIGHT_PROP),
            ssd_param.input_height);
  json_value_free(out_value);
}

TEST_F(ConfigureAnalyzeFixtureTests, HeaderIdFailTest) {
  JSON_Status stat = json_object_remove(config_json_object, "header");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOk);
  json_free_serialized_string((char *)config_mod);
}

TEST_F(ConfigureAnalyzeFixtureTests, CorrectAnalyzeJsonTest) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%d\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bbox": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bbox": {"left": 105, "top": 90, "right": 224, "bottom": 209}
    }
  ])");
  JSON_Value *expected_json = json_parse_string(expected_json_str);

  ASSERT_NE(p_out_buf, nullptr);
  ASSERT_GT(p_out_size, 0);

  JSON_Value *out_json = json_parse_string(p_out_buf);
  ASSERT_TRUE(json_value_equals(out_json, expected_json))
      << "  Actual JSON: " << p_out_buf << '\n'
      << "Expected JSON: " << expected_json_str;

  json_value_free(expected_json);
  json_value_free(out_json);
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
}

TEST_F(ConfigureAnalyzeFixtureTests, CorrectAnalyzeFlatbufferTest) {
  char *output = NULL;
  DataProcessorConfigure((char *)config, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  auto object_detection_top = SmartCamera::GetObjectDetectionTop(p_out_buf);

  std::vector<uint32_t> expected_class = {235, 95};

  std::vector<float> expected_score = {0.8, 0.6};

  int expected_bbox[8] = {45, 30, 164, 150, 105, 90, 224, 209};

  auto obj_detection_data =
      object_detection_top->perception()->object_detection_list();
  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);

    auto bbox = general_object->bounding_box_as_BoundingBox2d();

    EXPECT_EQ(general_object->class_id(), expected_class[i]);
    EXPECT_EQ(general_object->score(), expected_score[i]);
    EXPECT_EQ(bbox->left(), expected_bbox[i * 4]);
    EXPECT_EQ(bbox->top(), expected_bbox[i * 4 + 1]);
    EXPECT_EQ(bbox->right(), expected_bbox[i * 4 + 2]);
    EXPECT_EQ(bbox->bottom(), expected_bbox[i * 4 + 3]);
  }
  free(p_out_buf);
  EXPECT_EQ(152, p_out_size);
}

TEST_F(ConfigureAnalyzeFixtureTests, NullTensorAnalyzeTest) {
  uint32_t in_size;
  char *p_out_buf;
  uint32_t p_out_size;
  float *output_tensor = NULL;
  DataProcessorResultCode res =
      DataProcessorAnalyze(output_tensor, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
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
      config_json_object, "ai_models.detection.ai_model_bundle_id");
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

TEST_F(ConfigureAnalyzeFixtureTests, DataProcessorGetDataTypeBase64) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 0);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  EdgeAppLibSendDataType res = DataProcessorGetDataType();
  EXPECT_EQ(res, EdgeAppLibSendDataBase64);

  free(output);
  json_free_serialized_string((char *)config_mod);
}

TEST_F(ConfigureAnalyzeFixtureTests, DataProcessorGetDataTypeJson) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  EdgeAppLibSendDataType res = DataProcessorGetDataType();
  EXPECT_EQ(res, EdgeAppLibSendDataJson);

  free(output);
  json_free_serialized_string((char *)config_mod);
}
