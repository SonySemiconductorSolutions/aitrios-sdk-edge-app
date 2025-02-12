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

#include <iostream>
#include <sstream>
#include <vector>

#include "data_processor_api.hpp"
#include "data_processor_utils.hpp"
#include "parson.h"
#include "sm_utils.hpp"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;

class DataProcessorUtilsTes : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    mock_string_numbers = "[[1.23,4.56,7.89]]";
    json_param_number = "{\"param_1\": 3}";
    header = "{\"header\": {\"id\": \"00\", \"version\": \"01.01.00\"}}";
  }

  void TearDown() override { SensorCoreExit(0); }

  const char *mock_string_numbers = nullptr;
  const char *json_param_number = nullptr;
  const char *header = nullptr;
};

TEST_F(DataProcessorUtilsTes, GetValueNumberTest) {
  JSON_Value *root_value = json_parse_string(json_param_number);
  JSON_Object *json = json_object(root_value);
  double array = -1;
  EXPECT_EQ(GetValueNumber(json, "param_1", &array), 0);
  EXPECT_EQ(array, 3);
  EXPECT_EQ(GetValueNumber(json, "param_2", &array), 1);
  EXPECT_EQ(GetValueNumber(json, "param_2", nullptr), -1);
  json_value_free(root_value);
}

TEST_P(DataProcessorUtilsTes, SetEdgeAppLibNetwork) {
  // network_id is a six digits hexadecimal value string: from "000000" to
  // "FFFFFF"
  char network_id[32] = {0};
  char tmpbuf[65] = {0};
  char buf[256];
  for (int i = 0; i < 32; ++i) {
    network_id[i] = (rand() % 1000000) & 0xFF;
    sprintf(tmpbuf + (i * 2), "%02x", network_id[i]);
  }
  snprintf(buf, sizeof(buf), "{\"ai_model_bundle_id\":\"%s\"}", tmpbuf);
  // Parse JSON and handle errors
  JSON_Value *value = json_parse_string(buf);
  if (value == nullptr) {
    printf("Failed to parse JSON");
    return;
  }

  JSON_Object *object = json_object(value);
  // Call SetEdgeAppLibNetwork and check for leaks
  if (SetEdgeAppLibNetwork(0, object) < 0) {
    printf("SetEdgeAppLibNetwork failed");
    json_value_free(value);
    return;
  }

  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle {};

  if (SensorStreamGetProperty(
          0, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY, &ai_model_bundle,
          sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty)) < 0) {
    printf("Failed to get AI model bundle ID property");
    json_value_free(value);
    return;
  }

  ASSERT_TRUE(
      strncmp(ai_model_bundle.ai_model_bundle_id, tmpbuf, strlen(tmpbuf)) == 0);

  // Free the JSON_Value to avoid memory leaks
  json_value_free(value);
}

INSTANTIATE_TEST_CASE_P(DataProcessorUtilsTesP, DataProcessorUtilsTes,
                        ::testing::Range(0, 100));
