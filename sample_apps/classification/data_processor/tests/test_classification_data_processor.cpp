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

#include "classification_utils.hpp"
#include "data_processor_api.hpp"
#include "parson.h"
#include "sensor.h"
#include "testing_utils.hpp"

#define MAX_PREDICTIONS_PROP \
  "ai_models.classification.parameters.max_predictions"

#define MODEL_ID "ModelID"
#define DEVICE_ID "DeviceID"
#define BUF_IMAGE "Image"
#define BUF_TIME "T"
#define BUF_OUTPUT "O"
#define BUF_INFERENCE "Inferences"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;

EdgeAppLibSensorStream s_stream = 0;

void check_values(JSON_Object *json) {
  int max_predictions = json_object_dotget_number(json, MAX_PREDICTIONS_PROP);
  extern DataProcessorCustomParam cls_param;
  EXPECT_EQ(max_predictions, cls_param.maxPredictions);
}

TEST(Initialize, InitializeTest) {
  DataProcessorResultCode res = DataProcessorInitialize();
  EXPECT_EQ(res, kDataProcessorOk);
  extern DataProcessorCustomParam cls_param;
  EXPECT_EQ(DEFAULT_MAX_PREDICTIONS, cls_param.maxPredictions);
}

TEST(ResetState, ResetStateTest) {
  DataProcessorResultCode res = DataProcessorResetState();
  EXPECT_EQ(res, kDataProcessorOk);
}

TEST(Finalize, FinalizeTest) {
  DataProcessorResultCode res = DataProcessorFinalize();
  EXPECT_EQ(res, kDataProcessorOk);
}

class ConfigureAnalyzeFixtureTests : public ::testing::Test {
 protected:
  void SetUp() override {
    generate_random_uuid(network_id);
    nanoseconds = 1726161043914069133;
    config_json_val =
        json_parse_file("../../../test_data/custom_parameter.json");
    config_json_object = json_object(config_json_val);
    json_object_dotset_string(config_json_object,
                              "ai_models.classification.ai_model_bundle_id",
                              network_id);
    config = json_serialize_to_string(config_json_val);

    output_tensor_val =
        json_parse_file_with_comments("../../../test_data/output_tensor.jsonc");
    output_tensor = json_serialize_to_string(output_tensor_val);

    uint32_t num_array_elements = 0;
    out_data = StringToFloatArray((char *)output_tensor, &num_array_elements);
    out_size = num_array_elements;
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
  JSON_Object *config_json_object = nullptr;
  JSON_Value *output_tensor_val = nullptr;
  char *output_tensor = nullptr;
  char *config = nullptr;
  float *out_data = nullptr;
  uint32_t out_size = 0;
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
  uint64_t nanoseconds = 0;
};

/* -------------------------------------------------------- */
/*                 Configure                                */
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

TEST_F(ConfigureAnalyzeFixtureTests,
       ConfigureTestCorrectMaxPredictionsOverwriteNegative) {
  JSON_Status stat =
      json_object_dotset_number(config_json_object, MAX_PREDICTIONS_PROP, -1);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorOutOfRange);
  json_free_serialized_string((char *)config_mod);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigureTestFailParameterInvalidError) {
  std::list<std::string> parameters = {MAX_PREDICTIONS_PROP};
  for (const std::string &parameter : parameters)
    json_object_dotremove(config_json_object, parameter.c_str());

  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  EXPECT_TRUE(out_value != NULL);
  extern DataProcessorCustomParam cls_param;
  JSON_Object *json = json_object(out_value);
  EXPECT_EQ(json_object_dotget_number(json, MAX_PREDICTIONS_PROP),
            cls_param.maxPredictions);
  EXPECT_EQ(DEFAULT_MAX_PREDICTIONS, cls_param.maxPredictions);
  json_free_serialized_string((char *)config_mod);
  json_value_free(out_value);
  free(output);
}

/* -------------------------------------------------------- */
/*          Analyze                                         */
/* -------------------------------------------------------- */

TEST_F(ConfigureAnalyzeFixtureTests, AnalyzeTestCorrect) {
  char *output = NULL;
  DataProcessorConfigure((char *)config, &output);
  char *p_out_buf = nullptr;
  uint32_t p_out_size = 0;
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);
  free(p_out_buf);
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
      config_json_object, "ai_models.classification.ai_model_bundle_id");
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
  char *p_out_buf;
  uint32_t p_out_size;

  printf("out_size=%d\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorOk);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"([
    {
        "class_id": 3,
        "score": 0.19531199336051941
    },
    {
        "class_id": 0,
        "score": 0.171875
    },
    {
        "class_id": 1,
        "score": 0.010742249898612499
    },
    {
        "class_id": 2,
        "score": 0.010742249898612499
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

TEST_F(ConfigureAnalyzeFixtureTests, UndefinedFormatTest) {
  JSON_Status stat = json_object_dotset_number(config_json_object,
                                               "metadata_settings.format", 10);
  const char *config_mod = json_serialize_to_string(config_json_val);
  char *output = NULL;
  DataProcessorConfigure((char *)config_mod, &output);
  char *p_out_buf = NULL;
  uint32_t p_out_size = 0;

  printf("out_size=%d\n", out_size);
  DataProcessorResultCode res =
      DataProcessorAnalyze(out_data, out_size, &p_out_buf, &p_out_size);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  EXPECT_EQ(p_out_buf, nullptr);
  EXPECT_EQ(p_out_size, 0);

  json_free_serialized_string((char *)config_mod);
  free(p_out_buf);
}

TEST_F(ConfigureAnalyzeFixtureTests,
       AnalyzeJsonMaxPredictionUnderElementsTest) {
  JSON_Status stat =
      json_object_dotset_number(config_json_object, MAX_PREDICTIONS_PROP, 2);
  stat = json_object_dotset_number(config_json_object,
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
        "class_id": 3,
        "score": 0.19531199336051941
    },
    {
        "class_id": 0,
        "score": 0.171875
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
