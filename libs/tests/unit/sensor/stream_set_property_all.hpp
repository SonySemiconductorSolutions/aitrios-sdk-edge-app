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

#include "sensor.h"
#include "sensor_unit_test.h"
#include "sensor_unit_test_mock.h"
// #include "sensor_unit_test.h"
// #include "sensor_unit_test_mock.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
// #include "parson/parson.h"
// #include "edge_app/senscord.h"
#include "sensor_unsupported.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;

using namespace EdgeAppLib;

namespace aitrios_sensor_ut {

int32_t StreamSetPropertyAll(EdgeAppLibSensorStream stream,
                             int32_t expected_ret = 0) {
  int32_t ret = 0;

  uint32_t ai_model_bundle_ids[] = {0x900100, 0x000100, 0x0, 0xffffff};
  for (int i = 0;
       i < sizeof(ai_model_bundle_ids) / sizeof(ai_model_bundle_ids[0]); i++) {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property = {};
    property.ai_model_bundle_id[0] = ai_model_bundle_ids[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY;
    EdgeAppLibSensorImageCropProperty property = {};
    property.top = 10;
    property.left = 20;
    property.height = 2000;
    property.width = 3000;
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY;
    EdgeAppLibSensorPostProcessParameterProperty property = {};
    memset(property.param, 0, AITRIOS_SENSOR_INFERENCE_POST_PROCESS_PARAM_SIZE);
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY;
    EdgeAppLibSensorPostProcessAvailableProperty property = {};
    property.is_available = 0;
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorRotationAngle angles[] = {
      AITRIOS_SENSOR_ROTATION_ANGLE_0_DEG,
      AITRIOS_SENSOR_ROTATION_ANGLE_90_DEG,
      AITRIOS_SENSOR_ROTATION_ANGLE_180_DEG,
      AITRIOS_SENSOR_ROTATION_ANGLE_270_DEG,
  };
  for (int i = 0; i < sizeof(angles) / sizeof(angles[0]); i++) {
    const char *key = AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY;
    EdgeAppLibSensorImageRotationProperty property = {};
    property.rotation_angle = angles[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  int angles_err[] = {
      -1,
      4,
  };
  for (int i = 0; i < sizeof(angles_err) / sizeof(angles_err[0]); i++) {
    const char *key = AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY;
    EdgeAppLibSensorImageRotationProperty property = {};
    property.rotation_angle = (EdgeAppLibSensorRotationAngle)angles_err[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraFrameRateProperty frameRates[] = {
      {99, 100},   {499, 100},  {999, 100},  {1248, 100},
      {1498, 100}, {1998, 100}, {2497, 100}, {2997, 100}};
  for (int i = 0; i < sizeof(frameRates) / sizeof(frameRates[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY;
    EdgeAppLibSensorCameraFrameRateProperty property = frameRates[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraFrameRateProperty frameRates_err[] = {
      {99, 0},
  };
  for (int i = 0; i < sizeof(frameRates_err) / sizeof(frameRates_err[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY;
    EdgeAppLibSensorCameraFrameRateProperty property = frameRates_err[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraExposureMode exposureModes[] = {
      AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO,
      AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_MANUAL};
  for (int i = 0; i < sizeof(exposureModes) / sizeof(exposureModes[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorCameraExposureModeProperty property = {};
    property.mode = exposureModes[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraExposureModeUnsupported exposureModes_unsupported[] = {
      AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_GAIN_FIX_UNSUPPORTED,
      AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_TIME_FIX_UNSUPPORTED,
      AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_HOLD_UNSUPPORTED};
  for (int i = 0; i < sizeof(exposureModes_unsupported) /
                          sizeof(exposureModes_unsupported[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorCameraExposureModeProperty property = {};
    property.mode =
        (EdgeAppLibSensorCameraExposureMode)exposureModes_unsupported[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  int exposureModes_err[] = {-1, 5};
  for (int i = 0; i < sizeof(exposureModes_err) / sizeof(exposureModes_err[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorCameraExposureModeProperty property = {};
    property.mode = (EdgeAppLibSensorCameraExposureMode)exposureModes_err[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraAutoExposureProperty autoExposures[] = {
      {0, 2000 - 1, 69.0, 1},
      {200000 + 1, 2000 - 1, 3.0, 69},
  };
  for (int i = 0; i < sizeof(autoExposures) / sizeof(autoExposures[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY;
    EdgeAppLibSensorCameraAutoExposureProperty property =
        autoExposures[i];  // {};
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraAutoExposureProperty autoExposures_err[] = {
      {0, 2000, -1.0, 1},
  };
  for (int i = 0; i < sizeof(autoExposures_err) / sizeof(autoExposures_err[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY;
    EdgeAppLibSensorCameraAutoExposureProperty property =
        autoExposures_err[i];  // {};
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  float evCompensations[] = {
      1.50,  1.25,  1.00,  0.75,  0.50,  0.25,  0,
      -0.25, -0.50, -0.75, -1.00, -1.25, -1.50,
  };
  for (int i = 0; i < sizeof(evCompensations) / sizeof(evCompensations[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY;
    EdgeAppLibSensorCameraEvCompensationProperty property = {};
    property.ev_compensation = evCompensations[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraAntiFlickerMode flickerModes[] = {
      AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_OFF,
      AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_AUTO,
      AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_FORCE_50HZ,
      AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_FORCE_60HZ,
  };
  for (int i = 0; i < sizeof(flickerModes) / sizeof(flickerModes[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY;
    EdgeAppLibSensorCameraAntiFlickerModeProperty property = {};
    property.anti_flicker_mode = flickerModes[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  int flickerModes_err[] = {
      -1,
      4,
  };
  for (int i = 0; i < sizeof(flickerModes_err) / sizeof(flickerModes_err[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY;
    EdgeAppLibSensorCameraAntiFlickerModeProperty property = {};
    property.anti_flicker_mode =
        (EdgeAppLibSensorCameraAntiFlickerMode)flickerModes_err[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraManualExposureProperty manualExposures[] = {
      {0, 69.0},
      {200000, 3.0},
      {167, 69.0},
  };
  for (int i = 0; i < sizeof(manualExposures) / sizeof(manualExposures[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY;
    EdgeAppLibSensorCameraManualExposureProperty property =
        manualExposures[i];  // {};
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraManualExposureProperty manualExposures_err[] = {
      {66667, -1.0},
  };
  for (int i = 0;
       i < sizeof(manualExposures_err) / sizeof(manualExposures_err[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY;
    EdgeAppLibSensorCameraManualExposureProperty property =
        manualExposures_err[i];  // {};
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorInferenceWhiteBalanceMode whiteBalanceModes[] = {
      AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_AUTO,
      AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_MANUAL_PRESET};
  for (int i = 0; i < sizeof(whiteBalanceModes) / sizeof(whiteBalanceModes[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorWhiteBalanceModeProperty property = {};
    property.mode = whiteBalanceModes[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorInferenceWhiteBalanceModeUnsupported
      whiteBalanceModes_unsupported[] = {
          AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_MANUAL_GAIN_UNSUPPORTED,
          AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_HOLD_UNSUPPORTED};
  for (int i = 0; i < sizeof(whiteBalanceModes_unsupported) /
                          sizeof(whiteBalanceModes_unsupported[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorWhiteBalanceModeProperty property = {};
    property.mode = (EdgeAppLibSensorInferenceWhiteBalanceMode)
        whiteBalanceModes_unsupported[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  int whiteBalanceModes_err[] = {-1, 4};
  for (int i = 0;
       i < sizeof(whiteBalanceModes_err) / sizeof(whiteBalanceModes_err[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorWhiteBalanceModeProperty property = {};
    property.mode =
        (EdgeAppLibSensorInferenceWhiteBalanceMode)whiteBalanceModes_err[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  uint32_t convergenceSpeeds[] = {0, 15, 30, 60, 90};
  for (int i = 0; i < sizeof(convergenceSpeeds) / sizeof(convergenceSpeeds[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY;
    EdgeAppLibSensorAutoWhiteBalanceProperty property = {};
    property.convergence_speed = convergenceSpeeds[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  uint32_t colorTempratures[] = {3200, 4300, 5600, 6500};
  for (int i = 0; i < sizeof(colorTempratures) / sizeof(colorTempratures[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY;
    EdgeAppLibSensorManualWhiteBalancePresetProperty property = {};
    property.color_temperature = colorTempratures[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  uint32_t colorTempratures_err[] = {0, 1000, 7000};
  for (int i = 0;
       i < sizeof(colorTempratures_err) / sizeof(colorTempratures_err[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY;
    EdgeAppLibSensorManualWhiteBalancePresetProperty property = {};
    property.color_temperature = colorTempratures_err[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  // {
  //   const char *key =
  //   AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_GAIN_PROPERTY_KEY;
  //   EdgeAppLibSensorManualWhiteBalanceGainProperty property = {};
  //   property.gains.red = 30.0;
  //   property.gains.blue = 30.0;
  //   ret = EdgeAppLibSensorStreamSetProperty(stream, key, &property,
  //                                        sizeof(property));
  //   EXPECT_EQ(expected_ret, ret);
  // }

  bool imageFlips[] = {false, true};
  for (int i = 0; i < sizeof(imageFlips) / sizeof(imageFlips[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY;
    EdgeAppLibSensorCameraImageFlipProperty property = {};
    property.flip_vertical = imageFlips[i];
    property.flip_horizontal = imageFlips[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  EdgeAppLibSensorCameraImageSizeProperty imageSizes_err[] = {
      {2028 - 1, 1520, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_RESOLUTION},
      {2028, 1520 - 1, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_SENSITIVITY},
      {4056 + 1, 3040, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_RESOLUTION},
      {4056, 3040 + 1, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_SENSITIVITY}};
  for (int i = 0; i < sizeof(imageSizes_err) / sizeof(imageSizes_err[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY;
    EdgeAppLibSensorCameraImageSizeProperty property =
        imageSizes_err[i];  // {};
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(
        expected_ret,
        ret);  // success when set property. error will occur when stream start.
  }

  EdgeAppLibSensorCameraImageSizeProperty imageSizes[] = {
      {2028, 1520, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_RESOLUTION},
      {2028, 1520, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_SENSITIVITY},
      {4056, 3040, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_RESOLUTION},
      {4056, 3040, AITRIOS_SENSOR_CAMERA_SCALING_POLICY_SENSITIVITY}};
  for (int i = 0; i < sizeof(imageSizes) / sizeof(imageSizes[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY;
    EdgeAppLibSensorCameraImageSizeProperty property = imageSizes[i];  // {};
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  float zooms_err[] = {-1.0, 0.0, 1.5, 3.0};
  for (int i = 0; i < sizeof(zooms_err) / sizeof(zooms_err[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY;
    EdgeAppLibSensorCameraDigitalZoomProperty property = {};
    property.magnification = zooms_err[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(
        expected_ret,
        ret);  // success when set property. error will occur when stream start.
  }

  float zooms[] = {2.0, 1.0};
  for (int i = 0; i < sizeof(zooms) / sizeof(zooms[0]); i++) {
    const char *key = AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY;
    EdgeAppLibSensorCameraDigitalZoomProperty property = {};
    property.magnification = zooms[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  bool temperatureEnable[] = {false, true};
  for (int i = 0; i < sizeof(temperatureEnable) / sizeof(temperatureEnable[0]);
       i++) {
    const char *key = AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY;
    EdgeAppLibSensorTemperatureEnableProperty property = {};
    property.count = 1;
    property.temperatures[0].sensor_id = 0x00000000;
    property.temperatures[0].enable = temperatureEnable[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  uint64_t data_64[] = {0x0, 0xffffffff};
  for (int i = 0; i < sizeof(data_64) / sizeof(data_64[0]); i++) {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess64Property property = {};
    property.id = 1;
    property.address = 0x00000000;
    property.data = data_64[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  uint32_t data_32[] = {0x0, 0xffff};
  for (int i = 0; i < sizeof(data_32) / sizeof(data_32[0]); i++) {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess32Property property = {};
    property.id = 1;
    property.address = 0x00000000;
    property.data = data_32[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  uint16_t data_16[] = {0x0, 0xff};
  for (int i = 0; i < sizeof(data_16) / sizeof(data_16[0]); i++) {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess16Property property = {};
    property.id = 1;
    property.address = 0x00000000;
    property.data = data_16[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  uint8_t data_8[] = {0x0, 0xf};
  for (int i = 0; i < sizeof(data_8) / sizeof(data_8[0]); i++) {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess8Property property = {};
    property.id = 1;
    property.address = 0x00000000;
    property.data = data_8[i];
    ret = SensorStreamSetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  return ret;
}

}  // namespace aitrios_sensor_ut
