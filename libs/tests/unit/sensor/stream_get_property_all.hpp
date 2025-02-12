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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "sensor.h"
#include "sensor_unit_test.h"
#include "sensor_unit_test_mock.h"
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

int32_t StreamGetPropertyAll(EdgeAppLibSensorStream stream,
                             int32_t expected_ret = 0) {
  int32_t ret = 0;

  {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property2;
    ret = SensorStreamGetProperty(stream, key, &property2, sizeof(property2));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY;
    EdgeAppLibSensorImageCropProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY;
    EdgeAppLibSensorPostProcessParameterProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY;
    EdgeAppLibSensorPostProcessAvailableProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY;
    EdgeAppLibSensorImageRotationProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY;
    EdgeAppLibSensorCameraFrameRateProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorCameraExposureModeProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY;
    EdgeAppLibSensorCameraAutoExposureProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY;
    EdgeAppLibSensorCameraEvCompensationProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY;
    EdgeAppLibSensorCameraAntiFlickerModeProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY;
    EdgeAppLibSensorCameraManualExposureProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY;
    EdgeAppLibSensorWhiteBalanceModeProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY;
    EdgeAppLibSensorAutoWhiteBalanceProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY;
    EdgeAppLibSensorManualWhiteBalancePresetProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  // {
  //   const char *key =
  //   AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_GAIN_PROPERTY_KEY;
  //   EdgeAppLibSensorManualWhiteBalanceGainProperty property = {};
  //   ret = EdgeAppLibSensorStreamGetProperty(stream, key, &property,
  //                                        sizeof(property));
  //   EXPECT_EQ(expected_ret, ret);
  // }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY;
    EdgeAppLibSensorCameraImageFlipProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY;
    EdgeAppLibSensorCameraImageSizeProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY;
    EdgeAppLibSensorCameraDigitalZoomProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY;
    EdgeAppLibSensorInputDataTypeProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_IMAGE_PROPERTY_KEY;
    EdgeAppLibSensorImageProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CURRENT_FRAME_NUM_PROPERTY_KEY;
    EdgeAppLibSensorCurrentFrameNumProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_CHANNEL_INFO_PROPERTY_KEY;
    EdgeAppLibSensorChannelInfoProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY;
    EdgeAppLibSensorTemperatureEnableProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_INFERENCE_PROPERTY_KEY;
    EdgeAppLibSensorInferenceProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY;
    EdgeAppLibSensorInfoStringProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_FRAME_RATE_PROPERTY_KEY;
    EdgeAppLibSensorFrameRateProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess64Property property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess32Property property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess16Property property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY;
    EdgeAppLibSensorRegisterAccess8Property property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_TEMPERATURE_PROPERTY_KEY;
    EdgeAppLibSensorTemperatureProperty property = {};
    ret = SensorStreamGetProperty(stream, key, &property, sizeof(property));
    EXPECT_EQ(expected_ret, ret);
  }

  return ret;
}

}  // namespace aitrios_sensor_ut
