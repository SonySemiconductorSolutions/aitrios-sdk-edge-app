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
#include "mock_sensor.hpp"
#include "parson.h"
#include "sensor.h"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;
EdgeAppLibSensorStream s_stream = 0;
#define EPSILON 1e-4
#define MODEL_VERSION_ID "model_version_id"
#define DEVICE_ID "device_id"
#define BUF_IMAGE "image"
#define BUF_TIME "T"
#define BUF_OUTPUT "O"
#define BUF_INFERENCE "inferences"

class ConfigureAnalyzeFixtureTests : public ::testing::Test {
 protected:
  void SetUp() override {
    config_json_val =
        json_parse_file("../../../test_data/custom_parameter.json");
    nanoseconds = 1726161043914069133;
    config_json_object = json_object(config_json_val);
    generate_random_uuid(network_id);
    json_object_dotset_string(config_json_object,
                              "ai_models.passthrough.ai_model_bundle_id",
                              network_id);

    config = json_serialize_to_string(config_json_val);
  }

  void TearDown() override {
    json_value_free(config_json_val);
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

TEST_F(ConfigureAnalyzeFixtureTests, WrongAIModel) {
  char *output = NULL;
  const char *config_mod =
      R"({"ai_models" : {"test" : {"ai_model_bundle_id" : "000002"}}})";
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_FALSE(out_value == NULL);
  json_value_free(out_value);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, StreamSetPropertyFail) {
  setEdgeAppLibSensorStreamSetPropertyFail();
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  printf("config: %s\n", config);
  EXPECT_EQ(res, -1);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_FALSE(out_value == NULL);
  json_value_free(out_value);
  free(output);
}
