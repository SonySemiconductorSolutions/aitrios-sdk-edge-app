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

#include <cstdlib>

#include "dtdl_model/objects/common_settings.hpp"
#include "dtdl_model/properties.h"
#include "log.h"
#include "parson.h"
#include "sensor.h"
#include "sensor/mock_sensor.hpp"
#include "sm_context.hpp"
#include "states/idle.hpp"
#include "states/running.hpp"

#define DIGITAL_ZOOM "digital_zoom"
#define EV_COMPENSATION "ev_compensation"
#define EXPOSURE_MODE "exposure_mode"
#define AE_ANTI_FLICKER_MODE "ae_anti_flicker_mode"
#define WHITE_BALANCE_MODE "white_balance_mode"
#define IMAGE_ROTATION "image_rotation"

#define TEST_INPUT \
  "{            \
  \"camera_image_size\": {},             \
  \"camera_image_flip\": {},             \
  \"frame_rate\": {\"num\":2997,\"denom\":100},  \
  \"digital_zoom\": 0.25,                \
  \"exposure_mode\": 3,                  \
  \"auto_exposure\": {},                 \
  \"auto_exposure_metering\": {},        \
  \"ev_compensation\": 0.6,              \
  \"ae_anti_flicker_mode\": 1,           \
  \"manual_exposure\": {},               \
  \"white_balance_mode\": 1,            \
  \"auto_white_balance\": {},            \
  \"manual_white_balance_preset\": {},   \
  \"image_cropping\": {},                \
  \"image_rotation\": 2}"

using namespace EdgeAppLib;

class MockFrameRate : public FrameRate {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockCameraImageSize : public CameraImageSize {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockCameraImageFlip : public CameraImageFlip {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockAutoWhiteBalance : public AutoWhiteBalance {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockManualWhiteBalancePreset : public ManualWhiteBalancePreset {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockImageCropping : public ImageCropping {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockAutoExposure : public AutoExposure {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockAutoExposureMetering : public AutoExposureMetering {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockManualExposure : public ManualExposure {
 public:
  int Verify(JSON_Object *obj) override { return 0; }
  int Apply(JSON_Object *obj) override { return 0; }
};

class MockPqSettings : public PqSettings {
 public:
  FrameRate *GetFrameRate() { return &frameRate; }
  CameraImageSize *GetCameraImageSize() { return &camera_image_size; }
  CameraImageFlip *GetCameraImageFlip() { return &camera_image_flip; }
  MockAutoWhiteBalance *GetAutoWhiteBalance() { return &auto_white_balance; }
  MockManualWhiteBalancePreset *GetManualWhiteBalancePreset() {
    return &manual_white_balance_preset;
  }
  MockImageCropping *GetImageCropping() { return &image_cropping; };
  AutoExposure *GetAutoExposure() { return &auto_exposure; }
  AutoExposureMetering *GetAutoExposureMetering() {
    return &auto_exposure_metering;
  }
  ManualExposure *GetManualExposure() { return &manual_exposure; }

 private:
  MockFrameRate frameRate;
  MockCameraImageSize camera_image_size;
  MockCameraImageFlip camera_image_flip;
  MockAutoWhiteBalance auto_white_balance;
  MockManualWhiteBalancePreset manual_white_balance_preset;
  MockImageCropping image_cropping;
  MockAutoExposure auto_exposure;
  MockAutoExposureMetering auto_exposure_metering;
  MockManualExposure manual_exposure;
};

class PqSettingsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    state = StateFactory::Create(STATE_RUNNING);
    context = StateMachineContext::GetInstance(state);
    json_value = json_parse_string(TEST_INPUT);
    json_obj = json_object(json_value);
  }

  void TearDown() override {
    json_value_free(json_value);
    context->Delete();
  }

  State *state;
  StateMachineContext *context;
  JSON_Value *json_value;
  JSON_Object *json_obj;
};

TEST_F(PqSettingsTest, Parse) {
  MockPqSettings pq_settings;
  ASSERT_EQ(pq_settings.Verify(json_obj), 0);
}

TEST(PqSettings, InitializeValues) {
  PqSettings obj;

  JSON_Object *json_pre = obj.GetJsonObject();
  char *pre_init_str =
      json_serialize_to_string(json_object_get_wrapping_value(json_pre));
  obj.InitializeValues();
  JSON_Object *json_post = obj.GetJsonObject();
  char *post_init_str =
      json_serialize_to_string(json_object_get_wrapping_value(json_post));

  EXPECT_FALSE(strcmp(pre_init_str, post_init_str) == 0);

  obj.Delete();
  free(pre_init_str);
  free(post_init_str);
}

TEST_F(PqSettingsTest, CheckNotificationDigitalZoom) {
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraDigitalZoomProperty digital_zoom_prop;

  PqSettings *pq_settings =
      context->GetDtdlModel()->GetCommonSettings()->GetPqSettings();
  pq_settings->Apply(json_obj);
  context->ClearNotification();

  json_object_set_number(json_obj, "digital_zoom", 4.5);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY,
                          &digital_zoom_prop, sizeof(digital_zoom_prop));
  ASSERT_LT(abs(digital_zoom_prop.magnification - 4.5), TOLERANCE);

  context->ClearNotification();
  pq_settings->Apply(json_obj);
  ASSERT_FALSE(context->IsPendingNotification());

  context->ClearNotification();
  json_object_set_number(json_obj, "digital_zoom", 1.5);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY,
                          &digital_zoom_prop, sizeof(digital_zoom_prop));
  ASSERT_LT(abs(digital_zoom_prop.magnification - 1.5), TOLERANCE);

  context->ClearNotification();
  json_object_set_number(json_obj, "digital_zoom", 0.5);
  setEdgeAppLibSensorStreamSetPropertyFail();
  pq_settings->Apply(json_obj);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY,
                          &digital_zoom_prop, sizeof(digital_zoom_prop));
  ASSERT_LT(abs(digital_zoom_prop.magnification - 1.5), TOLERANCE);
}

TEST_F(PqSettingsTest, CheckNotificationExposureMode) {
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraExposureModeProperty exposure_mode = {
      .mode = AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO};

  PqSettings *pq_settings =
      context->GetDtdlModel()->GetCommonSettings()->GetPqSettings();
  pq_settings->Apply(json_obj);
  context->ClearNotification();

  json_object_set_number(json_obj, "exposure_mode", 0);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY,
                          &exposure_mode, sizeof(exposure_mode));
  ASSERT_EQ(exposure_mode.mode, (EdgeAppLibSensorCameraExposureMode)0);

  context->ClearNotification();
  json_object_set_number(json_obj, "exposure_mode", 3);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY,
                          &exposure_mode, sizeof(exposure_mode));
  ASSERT_EQ(exposure_mode.mode, (EdgeAppLibSensorCameraExposureMode)3);

  context->ClearNotification();
  json_object_set_number(json_obj, "exposure_mode", 0);
  setEdgeAppLibSensorStreamSetPropertyFail();
  pq_settings->Apply(json_obj);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY,
                          &exposure_mode, sizeof(exposure_mode));
  ASSERT_EQ(exposure_mode.mode, (EdgeAppLibSensorCameraExposureMode)3);
}

TEST_F(PqSettingsTest, CheckNotificationImageRotation) {
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorImageRotationProperty image_rotation_property;

  PqSettings *pq_settings =
      context->GetDtdlModel()->GetCommonSettings()->GetPqSettings();
  pq_settings->Apply(json_obj);
  context->ClearNotification();

  json_object_set_number(json_obj, "image_rotation", 3);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  context->ClearNotification();
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY,
                          &image_rotation_property,
                          sizeof(image_rotation_property));
  ASSERT_EQ(image_rotation_property.rotation_angle,
            AITRIOS_SENSOR_ROTATION_ANGLE_270_DEG);

  pq_settings->Apply(json_obj);
  ASSERT_FALSE(context->IsPendingNotification());

  context->ClearNotification();
  json_object_set_number(json_obj, "image_rotation", 1);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  context->ClearNotification();
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY,
                          &image_rotation_property,
                          sizeof(image_rotation_property));
  ASSERT_EQ(image_rotation_property.rotation_angle,
            AITRIOS_SENSOR_ROTATION_ANGLE_90_DEG);
  json_object_set_number(json_obj, "image_rotation", 2);
  setEdgeAppLibSensorStreamSetPropertyFail();
  pq_settings->Apply(json_obj);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY,
                          &image_rotation_property,
                          sizeof(image_rotation_property));
  ASSERT_EQ(image_rotation_property.rotation_angle,
            AITRIOS_SENSOR_ROTATION_ANGLE_90_DEG);
}

TEST_F(PqSettingsTest, CheckNotificationEVCompensation) {
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraEvCompensationProperty ev_compensation_prop = {
      .ev_compensation = 0.1};

  PqSettings *pq_settings =
      context->GetDtdlModel()->GetCommonSettings()->GetPqSettings();
  pq_settings->Apply(json_obj);
  context->ClearNotification();

  json_object_set_number(json_obj, "ev_compensation", 0.2);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY,
                          &ev_compensation_prop, sizeof(ev_compensation_prop));
  ASSERT_LT(abs(ev_compensation_prop.ev_compensation - (float)0.2), TOLERANCE);

  context->ClearNotification();
  json_object_set_number(json_obj, "ev_compensation", 0.1);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY,
                          &ev_compensation_prop, sizeof(ev_compensation_prop));
  ASSERT_LT(abs(ev_compensation_prop.ev_compensation - (float)0.1), TOLERANCE);

  context->ClearNotification();
  json_object_set_number(json_obj, "ev_compensation", 0.2);
  setEdgeAppLibSensorStreamSetPropertyFail();
  pq_settings->Apply(json_obj);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY,
                          &ev_compensation_prop, sizeof(ev_compensation_prop));
  ASSERT_LT(abs(ev_compensation_prop.ev_compensation - (float)0.1), TOLERANCE);
}

TEST_F(PqSettingsTest, CheckNotificationAEAntiFlickerMode) {
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraAntiFlickerModeProperty ae_anti_flicker_mode_prop = {
      .anti_flicker_mode = AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_AUTO};

  PqSettings *pq_settings =
      context->GetDtdlModel()->GetCommonSettings()->GetPqSettings();
  pq_settings->Apply(json_obj);
  context->ClearNotification();

  json_object_set_number(json_obj, "ae_anti_flicker_mode", 0);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY,
      &ae_anti_flicker_mode_prop, sizeof(ae_anti_flicker_mode_prop));
  ASSERT_EQ(ae_anti_flicker_mode_prop.anti_flicker_mode,
            (EdgeAppLibSensorCameraAntiFlickerMode)0);

  context->ClearNotification();
  json_object_set_number(json_obj, "ae_anti_flicker_mode", 1);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY,
      &ae_anti_flicker_mode_prop, sizeof(ae_anti_flicker_mode_prop));
  ASSERT_EQ(ae_anti_flicker_mode_prop.anti_flicker_mode,
            (EdgeAppLibSensorCameraAntiFlickerMode)1);

  context->ClearNotification();
  json_object_set_number(json_obj, "ae_anti_flicker_mode", 0);
  setEdgeAppLibSensorStreamSetPropertyFail();
  pq_settings->Apply(json_obj);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY,
      &ae_anti_flicker_mode_prop, sizeof(ae_anti_flicker_mode_prop));
  ASSERT_EQ(ae_anti_flicker_mode_prop.anti_flicker_mode,
            (EdgeAppLibSensorCameraAntiFlickerMode)1);
}

TEST_F(PqSettingsTest, CheckNotificationWBMode) {
  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorWhiteBalanceModeProperty white_balance_mode_prop = {
      .mode = AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_AUTO};

  PqSettings *pq_settings =
      context->GetDtdlModel()->GetCommonSettings()->GetPqSettings();
  pq_settings->Apply(json_obj);
  context->ClearNotification();

  json_object_set_number(json_obj, "white_balance_mode", 0);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY,
      &white_balance_mode_prop, sizeof(white_balance_mode_prop));
  ASSERT_EQ(white_balance_mode_prop.mode,
            (EdgeAppLibSensorInferenceWhiteBalanceMode)0);

  context->ClearNotification();
  json_object_set_number(json_obj, "white_balance_mode", 3);
  pq_settings->Apply(json_obj);
  ASSERT_TRUE(context->IsPendingNotification());

  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY,
      &white_balance_mode_prop, sizeof(white_balance_mode_prop));
  ASSERT_EQ(white_balance_mode_prop.mode,
            (EdgeAppLibSensorInferenceWhiteBalanceMode)3);

  context->ClearNotification();
  json_object_set_number(json_obj, "white_balance_mode", 0);
  setEdgeAppLibSensorStreamSetPropertyFail();
  pq_settings->Apply(json_obj);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  ASSERT_TRUE(context->IsPendingNotification());
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY,
      &white_balance_mode_prop, sizeof(white_balance_mode_prop));
  ASSERT_EQ(white_balance_mode_prop.mode,
            (EdgeAppLibSensorInferenceWhiteBalanceMode)3);
}
