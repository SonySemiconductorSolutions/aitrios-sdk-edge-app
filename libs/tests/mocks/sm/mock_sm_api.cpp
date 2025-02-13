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
#include "mock_sm_api.hpp"

#include <stdio.h>

#include "sm_api.hpp"

void updateProperty(EdgeAppLibSensorStream stream, const char *property_key,
                    const void *value, size_t value_size) {}

void updateCustomSettings(void *state, int statelen) {}

static JSON_Value *test_value = nullptr;
static JSON_Value *test_value1 = nullptr;
static int32_t num_of_inf = 0;
static EdgeAppLibSensorStream mock_stream;

void setPortSettings(int method) {
  if (test_value != nullptr) json_value_free(test_value);
  char *test_port_settings_template = R"({
        "metadata": {
            "method": %d,
            "storage_name": "metadatastoragename",
            "endpoint": "metadataendpoint",
            "path": "metadatapath",
            "enabled": true
        },
        "input_tensor": {
            "method": %d,
            "storage_name": "inputtensorstoragename",
            "endpoint": "inputtensorendpoint",
            "path": "inputtensorpath",
            "enabled": true
        }
    })";

  char test_port_settings[1024];
  snprintf(test_port_settings, sizeof(test_port_settings),
           test_port_settings_template, method, method);

  test_value = json_parse_string(test_port_settings);
}

void setPortSettingsNoInputTensor(int method) {
  setPortSettings(method);
  json_object_dotremove(json_object(test_value), "input_tensor");
}
void setPortSettingsNoInputTensorEnabled(void) {
  setPortSettings(2);
  json_object_dotremove(json_object(test_value), "input_tensor.enabled");
}

void setPortSettingsNoMetadata(void) {
  setPortSettings(2);
  json_object_dotremove(json_object(test_value), "metadata");
}

void setPortSettingsNoMetadataEndpoint(void) {
  setPortSettings(2);
  json_object_dotremove(json_object(test_value), "metadata.endpoint");
}

void setPortSettingsMetadataEndpoint(const char *endpoint, const char *path) {
  setPortSettings(2);
  JSON_Object *test_object = json_object(test_value);
  json_object_dotset_string(test_object, "metadata.endpoint", endpoint);
  json_object_dotset_string(test_object, "metadata.path", path);
}

void setPortSettingsInputTensorEndpoint(const char *endpoint,
                                        const char *path) {
  setPortSettings(2);
  JSON_Object *test_object = json_object(test_value);
  json_object_dotset_string(test_object, "input_tensor.endpoint", endpoint);
  json_object_dotset_string(test_object, "input_tensor.path", path);
}

void setPortSettingsMetadataDisabled(void) {
  setPortSettings(2);
  json_object_dotset_boolean(json_object(test_value), "metadata.enabled",
                             false);
}

void setPortSettingsInputTensorDisabled(void) {
  setPortSettings(2);
  json_object_dotset_boolean(json_object(test_value), "input_tensor.enabled",
                             false);
}

void resetPortSettings(void) { setPortSettings(2); }

void freePortSettingsValue(void) {
  json_value_free(test_value);
  test_value = nullptr;
}

JSON_Object *getPortSettings(void) {
  if (test_value == nullptr) setPortSettings(2);
  JSON_Object *test_ojbect = json_object(test_value);
  return test_ojbect;
}
void setCodecSettingsFull(void) {
  if (test_value1 != nullptr) json_value_free(test_value1);
  const char *test_codec_settings = R"({
   "format": 1
  })";
  test_value1 = json_parse_string(test_codec_settings);
}
JSON_Object *getCodecSettings(void) {
  if (test_value1 == nullptr) setCodecSettingsFull();
  JSON_Object *test_ojbect = json_object(test_value1);
  return test_ojbect;
}
void resetCodecSettings(void) { setCodecSettingsFull(); }
void freeCodecSettingsValue(void) {
  json_value_free(test_value1);
  test_value1 = nullptr;
}
void setCodecSettingsFormatValue(int num) {
  setCodecSettingsFull();
  JSON_Object *test_object = json_object(test_value1);
  json_object_set_number(test_object, "format", num);
}

void setNumOfInfPerMsg(int num) { num_of_inf = num; }
uint32_t getNumOfInfPerMsg(void) { return num_of_inf; }

EdgeAppLibSensorStream GetSensorStream(void) { return mock_stream; }
