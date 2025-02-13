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

#include "sensor.h"
#include "sm_api.hpp"
#include "sm_context.hpp"

using namespace EdgeAppLib;

class StateMachineApiTest : public ::testing::Test {
 protected:
  StateMachineContext *context = nullptr;
  EdgeAppLibSensorStream stream;
  JSON_Object *json_obj = nullptr;
  void SetUp() override {
    context = StateMachineContext::GetInstance(nullptr);
    stream = context->GetSensorStream();
    json_obj = context->GetDtdlModel()->GetJsonObject();
    SensorStart(stream);
  }

  void TearDown() override {
    context->Delete();
    SensorCoreExit(0);
  }
};

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraImageSizeProperty) {
  uint32_t width = 3;
  uint32_t height = 4;
  EdgeAppLibSensorCameraScalingPolicy scaling_policy =
      AITRIOS_SENSOR_CAMERA_SCALING_POLICY_SENSITIVITY;

  EdgeAppLibSensorCameraImageSizeProperty prop = {width, height,
                                                  scaling_policy};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(
      json_object_dotget_number(
          json_obj, "common_settings.pq_settings.camera_image_size.width"),
      width);
  ASSERT_EQ(
      json_object_dotget_number(
          json_obj, "common_settings.pq_settings.camera_image_size.height"),
      height);
  ASSERT_EQ(json_object_dotget_number(
                json_obj,
                "common_settings.pq_settings.camera_image_size.scaling_policy"),
            scaling_policy);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraImageFlipProperty) {
  bool flip_horizontal = 1;
  bool flip_vertical = 1;

  EdgeAppLibSensorCameraImageFlipProperty prop = {flip_horizontal,
                                                  flip_vertical};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(
      json_object_dotget_number(
          json_obj,
          "common_settings.pq_settings.camera_image_flip.flip_horizontal"),
      flip_horizontal);
  ASSERT_EQ(json_object_dotget_number(
                json_obj,
                "common_settings.pq_settings.camera_image_flip.flip_vertical"),
            flip_vertical);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraDigitalZoomProperty) {
  float magnification = 0.125;

  EdgeAppLibSensorCameraDigitalZoomProperty prop = {magnification};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.digital_zoom"),
            magnification);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraExposureModeProperty) {
  EdgeAppLibSensorCameraExposureMode mode =
      AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO;

  EdgeAppLibSensorCameraExposureModeProperty prop = {mode};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY,
                 &prop, sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.exposure_mode"),
            mode);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraAutoExposureProperty) {
  uint32_t max_exposure_time = 3;
  uint32_t min_exposure_time = 2;
  float max_gain = 1;
  uint32_t convergence_speed = 1;

  EdgeAppLibSensorCameraAutoExposureProperty prop = {
      max_exposure_time, min_exposure_time, max_gain, convergence_speed};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY,
                 &prop, sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj,
                "common_settings.pq_settings.auto_exposure.max_exposure_time"),
            max_exposure_time);
  ASSERT_EQ(json_object_dotget_number(
                json_obj,
                "common_settings.pq_settings.auto_exposure.min_exposure_time"),
            min_exposure_time);
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.auto_exposure.max_gain"),
            max_gain);
  ASSERT_EQ(json_object_dotget_number(
                json_obj,
                "common_settings.pq_settings.auto_exposure.convergence_speed"),
            convergence_speed);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraEvCompensationProperty) {
  float ev_compensation = 3;

  EdgeAppLibSensorCameraEvCompensationProperty prop = {ev_compensation};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY,
                 &prop, sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.ev_compensation"),
            ev_compensation);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraAntiFlickerModeProperty) {
  EdgeAppLibSensorCameraAntiFlickerMode anti_flicker_mode =
      AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_OFF;

  EdgeAppLibSensorCameraAntiFlickerModeProperty prop = {anti_flicker_mode};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY,
                 &prop, sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.ae_anti_flicker_mode"),
            anti_flicker_mode);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraManualExposureProperty) {
  uint32_t exposure_time = 1;
  float gain = 0.1;

  EdgeAppLibSensorCameraManualExposureProperty prop = {exposure_time, gain};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY,
                 &prop, sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj,
                "common_settings.pq_settings.manual_exposure.exposure_time"),
            exposure_time);
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.manual_exposure.gain"),
            gain);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorCameraFrameRateProperty) {
  uint32_t num = 2;
  uint32_t denom = 1;

  EdgeAppLibSensorCameraFrameRateProperty prop = {num, denom};
  updateProperty(stream, AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.frame_rate.num"),
            num);
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.frame_rate.denom"),
            denom);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorWhiteBalanceModeProperty) {
  EdgeAppLibSensorInferenceWhiteBalanceMode mode =
      AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_AUTO;

  EdgeAppLibSensorWhiteBalanceModeProperty prop = {mode};
  updateProperty(stream, AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.white_balance_mode"),
            mode);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorAutoWhiteBalanceProperty) {
  uint32_t convergence_speed = 3;

  EdgeAppLibSensorAutoWhiteBalanceProperty prop = {convergence_speed};
  updateProperty(stream, AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(
      json_object_dotget_number(
          json_obj,
          "common_settings.pq_settings.auto_white_balance.convergence_speed"),
      convergence_speed);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorManualWhiteBalancePresetProperty) {
  uint32_t color_temperature = 3200;
  uint32_t color_temperature_enum = 0;

  EdgeAppLibSensorManualWhiteBalancePresetProperty prop = {color_temperature};
  updateProperty(stream,
                 AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY, &prop,
                 sizeof(prop));

  ASSERT_EQ(json_object_dotget_number(json_obj,
                                      "common_settings.pq_settings.manual_"
                                      "white_balance_preset.color_temperature"),
            color_temperature_enum);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorImageCropProperty) {
  uint32_t left = 10;
  uint32_t top = 1;
  uint32_t width = 10;
  uint32_t height = 11;

  EdgeAppLibSensorImageCropProperty prop = {left, top, width, height};
  updateProperty(stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.image_cropping.left"),
            left);
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.image_cropping.top"),
            top);
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.image_cropping.width"),
            width);
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.image_cropping.height"),
            height);
}

TEST_F(StateMachineApiTest, EdgeAppLibSensorImageRotationProperty) {
  EdgeAppLibSensorRotationAngle rotation_angle =
      AITRIOS_SENSOR_ROTATION_ANGLE_90_DEG;

  EdgeAppLibSensorImageRotationProperty prop = {rotation_angle};
  updateProperty(stream, AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY, &prop,
                 sizeof(prop));
  ASSERT_EQ(json_object_dotget_number(
                json_obj, "common_settings.pq_settings.image_rotation"),
            rotation_angle);
}

TEST_F(StateMachineApiTest, UnknownProperty) {
  updateProperty(stream, "my-unknown-property", NULL, 0);
}

TEST_F(StateMachineApiTest, UpdateCustomSettings) {
  // TODO: improve custom setting creation
  const char *custom_settings = "{\"my-random-parameter\":3}";
  updateCustomSettings((void *)custom_settings, strlen(custom_settings));
  JSON_Object *json_object =
      context->GetDtdlModel()->GetCustomSettings()->GetJsonObject();
  char *custom_settings_act =
      json_serialize_to_string(json_object_get_wrapping_value(json_object));
  ASSERT_STREQ(custom_settings_act, custom_settings);
  json_free_serialized_string(custom_settings_act);
}

TEST_F(StateMachineApiTest, getPortSettings) {
  DtdlModel *dtdl = context->GetDtdlModel();
  const char *test_port_settings =
      "{\"metadata\":{"
      "\"method\":2,"
      "\"storage_name\":\"mystoragename\","
      "\"endpoint\":\"myendpoint\","
      "\"path\":\"mypath\","
      "\"enabled\":true"
      "},"
      "\"input_tensor\":{"
      "\"method\":2,"
      "\"storage_name\":\"mystoragename\","
      "\"endpoint\":\"myendpoint\","
      "\"path\":\"mypath\","
      "\"enabled\":true"
      "}}";
  JSON_Value *test_value = json_parse_string(test_port_settings);
  JSON_Object *test_ojbect = json_object(test_value);
  dtdl->GetCommonSettings()->GetPortSettings()->Apply(test_ojbect);

  JSON_Object *json_object = getPortSettings();
  char *port_settings =
      json_serialize_to_string(json_object_get_wrapping_value(json_object));

  ASSERT_STREQ(port_settings, test_port_settings);
  json_value_free(test_value);
  json_free_serialized_string(port_settings);
}
TEST_F(StateMachineApiTest, getCodecSettings) {
  DtdlModel *dtdl = context->GetDtdlModel();
  const char *test_codec_settings =
      "{"
      "\"format\":1"
      "}";
  JSON_Value *test_value = json_parse_string(test_codec_settings);
  JSON_Object *test_ojbect = json_object(test_value);
  dtdl->GetCommonSettings()->GetCodecSettings()->Apply(test_ojbect);
  JSON_Object *json_object = getCodecSettings();
  char *codec_settings =
      json_serialize_to_string(json_object_get_wrapping_value(json_object));
  ASSERT_STREQ(codec_settings, test_codec_settings);
  int format_value = json_object_get_number(json_object, "format");
  ASSERT_EQ(format_value, 1);
  json_value_free(test_value);
  json_free_serialized_string(codec_settings);
}

TEST_F(StateMachineApiTest, getNumberOfInferencePerMessages) {
  DtdlModel *dtdl = context->GetDtdlModel();
  int expected_value = 100;
  dtdl->GetCommonSettings()->SetInferencePerMessage(expected_value);
  ASSERT_EQ(expected_value, getNumOfInfPerMsg());
}

TEST_F(StateMachineApiTest, GetSensorStream) {
  EdgeAppLibSensorStream stream = 0;
  EdgeAppLibSensorStream stream_sm = 0x123456;
  context->SetSensorStream(stream_sm);
  stream = GetSensorStream();
  ASSERT_EQ(stream, 0x123456);
}
