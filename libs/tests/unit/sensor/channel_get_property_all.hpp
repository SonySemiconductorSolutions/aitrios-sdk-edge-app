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

int32_t ChannelGetPropertyAll(EdgeAppLibSensorChannel channel,
                              int32_t expected_ret = 0) {
  int32_t ret = 0;

  {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property2;
    ret = SensorChannelGetProperty(channel, key, &property2, sizeof(property2));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY;
    EdgeAppLibSensorImageCropProperty property2;
    ret = SensorChannelGetProperty(channel, key, &property2, sizeof(property2));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_IMAGE_PROPERTY_KEY;
    EdgeAppLibSensorImageProperty property2;
    ret = SensorChannelGetProperty(channel, key, &property2, sizeof(property2));
    EXPECT_EQ(expected_ret, ret);
  }

  {
    const char *key = AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY;
    EdgeAppLibSensorTensorShapesProperty property2;
    ret = SensorChannelGetProperty(channel, key, &property2, sizeof(property2));
    EXPECT_EQ(expected_ret, ret);
  }

  return ret;
}

}  // namespace aitrios_sensor_ut
