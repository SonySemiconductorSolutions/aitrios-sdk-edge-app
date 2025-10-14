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
#define BBOX_ORDER_PROP "ai_models.detection.parameters.bbox_order"
#define CLASS_ORDER_PROP "ai_models.detection.parameters.class_score_order"
#define BBOX_NORM_PROP "ai_models.detection.parameters.bbox_normalization"

#define EPSILON 1e-4
#define MODEL_ID "ModelID"
#define DEVICE_ID "DeviceID"
#define BUF_IMAGE "Image"
#define BUF_TIME "T"
#define BUF_OUTPUT "O"
#define BUF_INFERENCE "Inferences"

class ConfigureAnalyzeFixtureTests : public ::testing::Test {
 protected:
  const char *output_tensor_path = "../../../test_data/output_tensor.jsonc";
  const char *config_json_path = "../../../test_data/custom_parameter.json";
  float *out_data = nullptr;
  size_t out_size = 0;
  uint32_t num_array_elements = 0;
  JSON_Value *config_json_val = nullptr;
  JSON_Object *config_json_object = nullptr;
  char *config = nullptr;
  char network_id[37];  // UUID
  char area_config_str[1024];
  JSON_Value *area_config_json = nullptr;

  void LoadConfigJson() {
    config_json_val = json_parse_file(config_json_path);
    config_json_object = json_object(config_json_val);
    generate_random_uuid(network_id);
    json_object_dotset_string(config_json_object,
                              "ai_models.detection.ai_model_bundle_id",
                              network_id);
    config = json_serialize_to_string(config_json_val);
  }

  void LoadTensorData() {
    if (out_data) {
      free(out_data);
      out_data = nullptr;
    }
    JSON_Value *output_tensor_val =
        json_parse_file_with_comments(output_tensor_path);
    char *output_tensor = json_serialize_to_string(output_tensor_val);

    out_data = StringToFloatArray(output_tensor, &num_array_elements);
    out_size = num_array_elements * sizeof(float);

    free(output_tensor);
    json_value_free(output_tensor_val);
  }

  void SetUp() override {
    LoadConfigJson();
    LoadTensorData();

    snprintf(area_config_str, sizeof(area_config_str), R"(
    {
      "coordinates": {
          "left": 44,
          "top": 40,
          "right": 300,
          "bottom": 200
      },
      "overlap": 0.5,
      "class_id": [0, 95, 132, 235]
    })");
    area_config_json = json_parse_string(area_config_str);
  }

  void TearDown() override {
    json_value_free(config_json_val);
    json_free_serialized_string(output_tensor);
    json_free_serialized_string(config);
    free(out_data);
    SensorCoreExit(0);
    if (area_config_json != nullptr) {
      json_value_free(area_config_json);
    }
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

  void ValidateDetectionFlatbuffersData(char *p_out_buf) {
    auto object_detection_root = SmartCamera::GetObjectDetectionTop(p_out_buf);

    std::vector<uint32_t> expected_class = {235, 95};

    std::vector<float> expected_score = {0.8, 0.6};

    int expected_bbox[8] = {45, 30, 164, 150, 105, 90, 224, 209};
    size_t expected_num_of_detections = 2;

    auto obj_detection_data =
        object_detection_root->perception()->object_detection_list();
    ASSERT_EQ(obj_detection_data->size(), expected_num_of_detections);
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
  extern DataProcessorCustomParam detection_param;
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
  extern DataProcessorCustomParam detection_param;
  EXPECT_EQ((float)DEFAULT_THRESHOLD, detection_param.threshold);
  EXPECT_EQ(res, kDataProcessorOutOfRange);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, NumberOfClassIdsExceedsLimitTest) {
  json_object_set_value(config_json_object, "area", area_config_json);
  char class_id_str[512];
  snprintf(class_id_str, sizeof(class_id_str), R"(
    [0,1,2,3,4,5,6,7,8,9,10]
      )");
  JSON_Value *class_id_value = json_parse_string(class_id_str);
  JSON_Status stat = json_object_dotset_value(config_json_object,
                                              "area.class_id", class_id_value);
  area_config_json = nullptr;
  const char *config_mod = json_serialize_to_string_pretty(config_json_val);

  printf("[DEBUG]config_mod=%s\n", config_mod);

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
  for (const std::string &parameter : parameters)
    json_object_dotremove(config_json_object, parameter.c_str());
  char *config_mod = json_serialize_to_string(config_json_val);
  extern DataProcessorCustomParam detection_param;
  detection_param = {
      .max_detections = DEFAULT_MAX_DETECTIONS + 1,
      .threshold = DEFAULT_THRESHOLD + 1,
      .input_width = DEFAULT_INPUT_TENSOR_WIDTH + 1,
      .input_height = DEFAULT_INPUT_TENSOR_HEIGHT + 1,
  };
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure(config_mod, &output);
  json_free_serialized_string(config_mod);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  EXPECT_NEAR(DEFAULT_THRESHOLD, detection_param.threshold, EPSILON);
  EXPECT_EQ(DEFAULT_MAX_DETECTIONS, detection_param.max_detections);
  EXPECT_EQ(DEFAULT_INPUT_TENSOR_WIDTH, detection_param.input_width);
  EXPECT_EQ(DEFAULT_INPUT_TENSOR_HEIGHT, detection_param.input_height);
  JSON_Value *out_value = json_parse_string(output);
  free(output);
  EXPECT_TRUE(out_value != NULL);
  JSON_Object *json = json_object(out_value);
  EXPECT_NEAR(json_object_dotget_number(json, THRESHOLD_PROP),
              detection_param.threshold, EPSILON);
  EXPECT_EQ(json_object_dotget_number(json, MAX_PREDICTIONS_PROP),
            detection_param.max_detections);
  EXPECT_EQ(json_object_dotget_number(json, INPUT_WIDTH_PROP),
            detection_param.input_width);
  EXPECT_EQ(json_object_dotget_number(json, INPUT_HEIGHT_PROP),
            detection_param.input_height);
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

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

  ValidateDetectionFlatbuffersData(p_out_buf);

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

TEST_F(ConfigureAnalyzeFixtureTests, UndefinedDetectionFormatTest) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 10);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOk);
  char *p_out_buf = NULL;
  uint32_t p_out_size = 0;

  printf("out_size=%zu\n", out_size);
  res = DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  ASSERT_NE(p_out_buf, nullptr);
  ASSERT_GT(p_out_size, 0);

  ValidateDetectionFlatbuffersData(p_out_buf);

  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, UndefinedAreaCountFormatTest) {
  json_object_set_value(config_json_object, "area", area_config_json);
  area_config_json = nullptr;
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 10);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOk);
  char *p_out_buf = NULL;
  uint32_t p_out_size = 0;

  printf("out_size=%zu\n", out_size);
  res = DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  ASSERT_NE(p_out_buf, nullptr);
  ASSERT_GT(p_out_size, 0);

  ValidateDetectionFlatbuffersData(p_out_buf);

  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, CorrectAnalyzeAreaCountFlatbuffersTest) {
  json_object_set_value(config_json_object, "area", area_config_json);
  area_config_json = nullptr;

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  auto object_detection_root = SmartCamera::GetObjectDetectionTop(p_out_buf);

  std::vector<uint32_t> expected_class = {235, 95};

  AreaCount expected_area_count[2] = {{.class_id = 235, .count = 1},
                                      {.class_id = 95, .count = 1}};
  size_t expected_num_of_class = 2;
  size_t expected_num_of_detections = 2;

  auto obj_detection_data =
      object_detection_root->perception()->object_detection_list();
  ASSERT_EQ(obj_detection_data->size(), expected_num_of_detections);
  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);
    EXPECT_EQ(general_object->class_id(), expected_class[i]);
  }

  auto area_count_data = object_detection_root->area_count();
  ASSERT_EQ(area_count_data->size(), expected_num_of_class);
  for (int i = 0; i < area_count_data->size(); i++) {
    auto count_data = area_count_data->Get(i);
    EXPECT_EQ(count_data->class_id(), expected_area_count[i].class_id);
    EXPECT_EQ(count_data->count(), expected_area_count[i].count);
  }
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
  EXPECT_EQ(192, p_out_size);
}

TEST_F(ConfigureAnalyzeFixtureTests, CorrectAnalyzeAreaCountJsonTest) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_set_value(config_json_object, "area", area_config_json);
  area_config_json = nullptr;

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"(
    {
    "area_count":{
        "235":1,
        "95":1
      },
  "detections":[
        {
            "class_id": 235,
            "score": 0.8,
            "bounding_box": {
            "left": 45,
            "top": 30,
            "right": 164,
            "bottom": 150
            }
        },
        {
            "class_id": 95,
            "score": 0.6,
            "bounding_box": {
            "left": 105,
            "top": 90,
            "right": 224,
            "bottom": 209
            }
        }
       ]
  }
  )");
  JSON_Value *expected_json = json_parse_string(expected_json_str);

  ASSERT_NE(p_out_buf, nullptr);
  ASSERT_GT(p_out_size, 0);

  JSON_Value *out_json = json_parse_string(p_out_buf);
  EXPECT_TRUE(json_value_equals(out_json, expected_json))
      << "  Actual JSON: " << p_out_buf << '\n'
      << "Expected JSON: " << expected_json_str;

  json_value_free(expected_json);
  json_value_free(out_json);
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
}

TEST_F(ConfigureAnalyzeFixtureTests, EmptyClassIdJsonTest) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  JSON_Object *area_config_obj = json_value_get_object(area_config_json);
  JSON_Value *empty_array_value = json_value_init_array();
  JSON_Array *empty_array = json_value_get_array(empty_array_value);
  stat =
      json_object_dotset_value(area_config_obj, "class_id", empty_array_value);
  json_object_set_value(config_json_object, "area", area_config_json);
  area_config_json = nullptr;

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"(
    {
    "area_count":{
        "95":1,
        "235":1
      },
  "detections":[
        {
            "class_id": 235,
            "score": 0.8,
            "bounding_box": {
            "left": 45,
            "top": 30,
            "right": 164,
            "bottom": 150
            }
        },
        {
            "class_id": 95,
            "score": 0.6,
            "bounding_box": {
            "left": 105,
            "top": 90,
            "right": 224,
            "bottom": 209
            }
        }
       ]
  }
  )");
  JSON_Value *expected_json = json_parse_string(expected_json_str);

  ASSERT_NE(p_out_buf, nullptr);
  ASSERT_GT(p_out_size, 0);

  JSON_Value *out_json = json_parse_string(p_out_buf);
  EXPECT_TRUE(json_value_equals(out_json, expected_json))
      << "  Actual JSON: " << p_out_buf << '\n'
      << "Expected JSON: " << expected_json_str;

  json_value_free(expected_json);
  json_value_free(out_json);
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
}

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorYXYXN) {
  output_tensor_path = "../../../test_data/output_tensor_yxyxn_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXYXYN) {
  output_tensor_path = "../../../test_data/output_tensor_xyxyn_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xyxy");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXXYYN) {
  output_tensor_path = "../../../test_data/output_tensor_xxyyn_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xxyy");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXYWHN) {
  output_tensor_path = "../../../test_data/output_tensor_xywhn_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xywh");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, true);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "cls_score");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXYXY) {
  output_tensor_path = "../../../test_data/output_tensor_xyxy_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xyxy");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "cls_score");
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 300);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 300);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorYXYX) {
  output_tensor_path = "../../../test_data/output_tensor_yxyx_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "yxyx");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "cls_score");
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXYWH) {
  output_tensor_path = "../../../test_data/output_tensor_xywh_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xywh");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "cls_score");
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 300);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 300);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXXYY) {
  output_tensor_path = "../../../test_data/output_tensor_xxyy_cls_score.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xxyy");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "cls_score");
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 300);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 300);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXXYYScoreClass) {
  output_tensor_path = "../../../test_data/output_tensor_xxyy_score_cls.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xxyy");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "score_cls");
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 300);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 300);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorYXYXNScoreClass) {
  output_tensor_path = "../../../test_data/output_tensor_yxyxn_score_cls.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "yxyx");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, true);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "score_cls");
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 300);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 300);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 235,
        "score": 0.8,
        "bounding_box": {"left": 45, "top": 30, "right": 164, "bottom": 150}
    },
    {
        "class_id": 95,
        "score": 0.6,
        "bounding_box": {"left": 105, "top": 90, "right": 224, "bottom": 209}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXYXYScoreClass) {
  output_tensor_path = "../../../test_data/output_tensor_xyxy_score_cls.jsonc";
  LoadTensorData();

  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xyxy");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "score_cls");
  json_object_dotset_number(config_json_object, MAX_PREDICTIONS_PROP, 10);
  json_object_dotset_number(config_json_object, THRESHOLD_PROP, 0.06);
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 480);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 480);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 0,
        "score": 0.92,
        "bounding_box": {"left": 68,"top": 240,"right": 172,"bottom": 356}
    },
    {
        "class_id": 0,
        "score": 0.87,
        "bounding_box": {"left": 172, "top": 180, "right": 248, "bottom": 264}
    },
    {
        "class_id": 0,
        "score": 0.07,
        "bounding_box": {"left": 324, "top": 152, "right": 364, "bottom": 216}
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

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXYXYScoreClassAreaFbs) {
  output_tensor_path = "../../../test_data/output_tensor_xyxy_score_cls.jsonc";
  LoadTensorData();
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xyxy");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "score_cls");
  json_object_dotset_number(config_json_object, MAX_PREDICTIONS_PROP, 10);
  json_object_dotset_number(config_json_object, THRESHOLD_PROP, 0.06);
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 480);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 480);

  snprintf(area_config_str, sizeof(area_config_str), R"(
    {
      "coordinates": {
          "left": 15,
          "top": 10,
          "right": 470,
          "bottom": 470
      },
      "overlap": 0.5,
      "class_id": []
    })");
  json_value_free(area_config_json);
  area_config_json = json_parse_string(area_config_str);

  JSON_Object *area_config_obj = json_value_get_object(area_config_json);
  json_object_set_value(config_json_object, "area", area_config_json);
  area_config_json = nullptr;

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  auto object_detection_root = SmartCamera::GetObjectDetectionTop(p_out_buf);

  std::vector<uint32_t> expected_class = {0, 0, 0};

  AreaCount expected_area_count[1] = {{.class_id = 0, .count = 3}};
  size_t expected_num_of_class = 1;
  size_t expected_num_of_detections = 3;

  auto obj_detection_data =
      object_detection_root->perception()->object_detection_list();
  ASSERT_EQ(obj_detection_data->size(), expected_num_of_detections);
  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);

    EXPECT_EQ(general_object->class_id(), expected_class[i]);
  }

  auto area_count_data = object_detection_root->area_count();
  ASSERT_EQ(area_count_data->size(), expected_num_of_class);
  for (int i = 0; i < area_count_data->size(); i++) {
    auto count_data = area_count_data->Get(i);
    EXPECT_EQ(count_data->class_id(), expected_area_count[i].class_id);
    EXPECT_EQ(count_data->count(), expected_area_count[i].count);
  }
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
  EXPECT_EQ(212, p_out_size);
}

TEST_F(ConfigureAnalyzeFixtureTests, CustomTensorXYXYScoreClassAreaJson) {
  output_tensor_path = "../../../test_data/output_tensor_xyxy_score_cls.jsonc";
  LoadTensorData();
  json_object_dotset_number(config_json_object, "metadata_settings.format", 1);
  json_object_dotset_string(config_json_object, BBOX_ORDER_PROP, "xyxy");
  json_object_dotset_boolean(config_json_object, BBOX_NORM_PROP, false);
  json_object_dotset_string(config_json_object, CLASS_ORDER_PROP, "score_cls");
  json_object_dotset_number(config_json_object, MAX_PREDICTIONS_PROP, 10);
  json_object_dotset_number(config_json_object, THRESHOLD_PROP, 0.06);
  json_object_dotset_number(config_json_object, INPUT_HEIGHT_PROP, 480);
  json_object_dotset_number(config_json_object, INPUT_WIDTH_PROP, 480);

  snprintf(area_config_str, sizeof(area_config_str), R"(
    {
      "coordinates": {
          "left": 15,
          "top": 10,
          "right": 470,
          "bottom": 470
      },
      "overlap": 0.5,
      "class_id": []
    })");
  json_value_free(area_config_json);
  area_config_json = json_parse_string(area_config_str);

  JSON_Object *area_config_obj = json_value_get_object(area_config_json);
  json_object_set_value(config_json_object, "area", area_config_json);
  area_config_json = nullptr;

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"(
    {
    "area_count":{
        "0":3
      },
  "detections":[
      {
          "class_id": 0,
          "score": 0.92,
          "bounding_box": {"left": 68,"top": 240,"right": 172,"bottom": 356}
      },
      {
          "class_id": 0,
          "score": 0.87,
          "bounding_box": {"left": 172, "top": 180, "right": 248, "bottom": 264}
      },
      {
          "class_id": 0,
          "score": 0.07,
          "bounding_box": {"left": 324, "top": 152, "right": 364, "bottom": 216}
      }
          ]
  }
  )");
  JSON_Value *expected_json = json_parse_string(expected_json_str);

  ASSERT_NE(p_out_buf, nullptr);
  ASSERT_GT(p_out_size, 0);

  JSON_Value *out_json = json_parse_string(p_out_buf);
  EXPECT_TRUE(json_value_equals(out_json, expected_json))
      << "  Actual JSON: " << p_out_buf << '\n'
      << "Expected JSON: " << expected_json_str;

  json_value_free(expected_json);
  json_value_free(out_json);
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
}

TEST_F(ConfigureAnalyzeFixtureTests, EmptyClassIdFlatbuffersTest) {
  JSON_Object *area_config_obj = json_value_get_object(area_config_json);
  JSON_Value *empty_array_value = json_value_init_array();
  JSON_Array *empty_array = json_value_get_array(empty_array_value);
  JSON_Status stat =
      json_object_dotset_value(area_config_obj, "class_id", empty_array_value);
  json_object_set_value(config_json_object, "area", area_config_json);
  area_config_json = nullptr;

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%zu\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  auto object_detection_root = SmartCamera::GetObjectDetectionTop(p_out_buf);

  std::vector<uint32_t> expected_class = {235, 95};

  int expected_bbox[8] = {45, 30, 164, 150, 105, 90, 224, 209};

  AreaCount expected_area_count[2] = {{.class_id = 235, .count = 1},
                                      {.class_id = 95, .count = 1}};
  size_t expected_num_of_class = 2;
  size_t expected_num_of_detections = 2;

  auto obj_detection_data =
      object_detection_root->perception()->object_detection_list();

  ASSERT_EQ(obj_detection_data->size(), expected_num_of_detections);
  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);

    EXPECT_EQ(general_object->class_id(), expected_class[i]);
  }

  auto area_count_data = object_detection_root->area_count();
  ASSERT_EQ(area_count_data->size(), expected_num_of_class);
  for (int i = 0; i < area_count_data->size(); i++) {
    auto count_data = area_count_data->Get(i);
    EXPECT_EQ(count_data->class_id(), expected_area_count[i].class_id);
    EXPECT_EQ(count_data->count(), expected_area_count[i].count);
  }
  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
  EXPECT_EQ(192, p_out_size);
}
