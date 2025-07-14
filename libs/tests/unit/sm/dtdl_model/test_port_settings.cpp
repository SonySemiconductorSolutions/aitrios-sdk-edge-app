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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dtdl_model/objects/common_settings.hpp"
#include "dtdl_model/properties.h"
#include "fixtures/common_fixture.hpp"
#include "fixtures/state_fixture.cpp"
#include "mock_sensor.hpp"
#include "parson.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "states/state_factory.hpp"

using ::testing::Contains;

#define CONFIG(method, storage_name, endpoint, path, enabled)  \
  "\"method\": " #method ", \"storage_name\": \"" storage_name \
  "\", \"endpoint\": \"" endpoint                              \
  "\", "                                                       \
  "\"path\": \"" path "\", \"enabled\": " #enabled

#define CONFIG_1 CONFIG(2, "mystoragename", "myendpoint", "mypath", true)
#define CONFIG_2 CONFIG(2, "mystoragename2", "myendpoint2", "mypath2", false)

#define TEST_PORT_SETTINGS(input_config, metadata_config)                  \
  "{\"input_tensor\": {" input_config "}, \"metadata\": {" metadata_config \
  "}"                                                                      \
  "}"

#define TEST_PORT_SETTINGS_11 TEST_PORT_SETTINGS(CONFIG_1, CONFIG_1)
#define TEST_PORT_SETTINGS_12 TEST_PORT_SETTINGS(CONFIG_1, CONFIG_2)
#define TEST_PORT_SETTINGS_21 TEST_PORT_SETTINGS(CONFIG_2, CONFIG_1)
#define TEST_PORT_SETTINGS_22 TEST_PORT_SETTINGS(CONFIG_2, CONFIG_2)

class PortSettingsTest : public CommonTest {
 public:
  void AssertStreamChannels(bool metadata, bool input_tensor) {
    EdgeAppLibSensorStream stream = context->GetSensorStream();
    EdgeAppLibSensorInputDataTypeProperty enabled = {};
    EdgeAppLib::SensorStreamGetProperty(
        stream, AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY, &enabled,
        sizeof(enabled));

    uint32_t expected_count = 0;
    if (metadata) expected_count++;
    if (input_tensor) expected_count++;
    ASSERT_EQ(enabled.count, expected_count);

    std::vector enabled_channels(enabled.channels,
                                 enabled.channels + enabled.count);
    if (metadata)
      ASSERT_THAT(enabled_channels,
                  Contains(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT));
    if (input_tensor)
      ASSERT_THAT(enabled_channels,
                  Contains(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE));
  }

  void AssertStreamChannelsOT() { AssertStreamChannels(true, false); }
  void AssertStreamChannelsIT() { AssertStreamChannels(false, true); }
  void AssertStreamChannelsITOT() { AssertStreamChannels(true, true); }
};

TEST_F(PortSettingsTest, Parse) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_11);
  JSON_Object *object = json_object(value);

  PortSettings ps;
  ASSERT_EQ(ps.Verify(object), 0);
  ps.Delete();
  json_value_free(value);
}

TEST_F(PortSettingsTest, EmptyJson) {
  PortSettings obj;
  JSON_Value *value = json_parse_string("{}");
  obj.Apply(json_object(value));
  json_value_free(value);
}

TEST_F(PortSettingsTest, PortSettingsSetMode0) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_12);
  JSON_Object *object = json_object(value);

  PortSettings ps;
  ps.Apply(object);

  // input tensor only enabled -> 0
  AssertStreamChannelsIT();

  ps.Delete();
  json_value_free(value);
}

TEST_F(PortSettingsTest, PortSettingsSetMode1) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_11);
  JSON_Object *object = json_object(value);

  PortSettings ps;
  ps.Apply(object);

  // both enabled -> 1
  AssertStreamChannelsITOT();

  ps.Delete();
  json_value_free(value);
}

TEST_F(PortSettingsTest, PortSettingsSetMode2) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_21);
  JSON_Object *object = json_object(value);

  PortSettings ps;
  ps.Apply(object);

  // input "metadata only enabled -> 2
  AssertStreamChannelsOT();

  ps.Delete();
  json_value_free(value);
}

TEST_F(PortSettingsTest, PortSettingsSetModeError) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_22);
  JSON_Object *object = json_object(value);

  PortSettings ps;
  ps.Apply(object);

  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(),
               "Neither input tensor or metadata are enabled");

  ps.Delete();
  json_value_free(value);
}

TEST_F(PortSettingsTest, PortSettingsSetModeSensorInputDisabled) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_11);
  JSON_Object *object = json_object(value);

  PortSettings ps;
  ps.Apply(object);

  // both enabled -> 1
  AssertStreamChannelsITOT();

  // Disable sensor input data type property
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  EdgeAppLib::SensorInputDataTypeEnableChannel(
      &enabled, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, true);
  EdgeAppLib::SensorInputDataTypeEnableChannel(
      &enabled, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, false);
  int32_t result = EdgeAppLib::SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY, &enabled,
      sizeof(enabled));
  ASSERT_EQ(result, 0);

  AssertStreamChannelsOT();

  ps.Apply(object);
  // both enabled -> 1
  AssertStreamChannelsITOT();

  ps.Delete();
  json_value_free(value);
}

TEST_F(PortSettingsTest, PortSettingsSetModeSensorMetadataDisabled) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_11);
  JSON_Object *object = json_object(value);

  PortSettings ps;
  ps.Apply(object);

  // both enabled -> 1
  AssertStreamChannelsITOT();

  // Disable sensor metadata type property
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  EdgeAppLib::SensorInputDataTypeEnableChannel(
      &enabled, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, false);
  EdgeAppLib::SensorInputDataTypeEnableChannel(
      &enabled, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, true);
  int32_t result = EdgeAppLib::SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY, &enabled,
      sizeof(enabled));
  ASSERT_EQ(result, 0);

  AssertStreamChannelsIT();

  ps.Apply(object);
  // both enabled -> 1
  AssertStreamChannelsITOT();

  ps.Delete();
  json_value_free(value);
}

TEST_F(PortSettingsTest, ApplyStreamChannelsError) {
  JSON_Value *value = json_parse_string(TEST_PORT_SETTINGS_21);
  JSON_Object *object = json_object(value);
  setEdgeAppLibSensorStreamSetPropertyFail();
  PortSettings ps;
  ps.Apply(object);

  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
  ASSERT_EQ(dtdl->GetResInfo()->GetCode(), CODE_INVALID_ARGUMENT);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(),
               "Input Data Type property failed to be set.");

  ps.Delete();
  json_value_free(value);
}
