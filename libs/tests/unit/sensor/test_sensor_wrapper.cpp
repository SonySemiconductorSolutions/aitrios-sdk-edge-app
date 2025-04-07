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

#include "channel_get_property_all.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mock_device.hpp"
#include "sensor.h"
#include "sensor_unit_test.h"
#include "sensor_unit_test_mock.h"
#include "sensor_unsupported.h"
#include "stream_get_property_all.hpp"
#include "stream_set_property_all.hpp"

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

const uint64_t DUMMY_HANDLE_CORE = 0x1111;
const uint64_t DUMMY_HANDLE_STREAM = 0x2222;
const uint64_t DUMMY_HANDLE_FRAME = 0x3333;
const uint64_t DUMMY_HANDLE_CHANNEL = 0x4444;

const char *g_unsupported_keys[] = {
    AITRIOS_SENSOR_AI_MODEL_INDEX_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_GAIN_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_CHANNEL_INFO_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_CHANNEL_MASK_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_CURRENT_FRAME_NUM_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_FRAME_RATE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_IMAGE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_INFERENCE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_TEMPERATURE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY_UNSUPPORTED,
};

const char *g_channel_unsupported_keys[] = {
    AITRIOS_SENSOR_AI_MODEL_INDEX_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_IMAGE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_INFERENCE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY_UNSUPPORTED};

const char *g_error_channel_unsupported_keys[] = {
    AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY,
    AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY,
    AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY,
    AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_GAIN_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY,
    AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY,
    AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY,
    AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY,

    AITRIOS_SENSOR_CHANNEL_INFO_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_CHANNEL_MASK_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_CURRENT_FRAME_NUM_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_FRAME_RATE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_TEMPERATURE_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY_UNSUPPORTED,
    AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY_UNSUPPORTED,
};

// -------- //
TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorStart_Normal_Success) {
  EXPECT_CALL(*mock_, senscord_stream_start(_)).WillOnce(testing::Return(0));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStop_SensCordStartUninitialized) {
  EdgeAppLibSensorStream stream = 0;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(-1, ret);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorStart_SensCordError_Error) {
  EXPECT_CALL(*mock_, senscord_stream_start(_)).WillOnce(testing::Return(-1));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(-1, ret);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorStop_Normal_Success) {
  EXPECT_CALL(*mock_, senscord_stream_stop(_)).WillOnce(testing::Return(0));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStop(stream);
  EXPECT_EQ(0, ret);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorStop_SensCordStopFail_Error) {
  EXPECT_CALL(*mock_, senscord_stream_stop(_)).WillOnce(testing::Return(-1));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStop(stream);
  EXPECT_EQ(-1, ret);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStop_SensCordStopUninitialized) {
  EdgeAppLibSensorStream stream = 0;
  int32_t ret = SensorStop(stream);
  EXPECT_EQ(-1, ret);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStreamSetProperty_UpdateProperty) {
  int32_t ret = 0;
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  // using image crop as representative property
  // TODO: test it for every property
  EdgeAppLibSensorImageCropProperty image_crop_property = {
      .left = 1, .top = 2, .width = 3, .height = 4};
  ret = SensorStreamSetProperty(stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                                &image_crop_property,
                                sizeof(image_crop_property));
  EXPECT_EQ(0, ret);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStreamSetProperty_UpdateProperty_with_StreamNull) {
  int32_t ret = 0;
  EdgeAppLibSensorStream stream = 0;
  // using image crop as representative property
  // TODO: test it for every property
  EdgeAppLibSensorImageCropProperty image_crop_property = {
      .left = 1, .top = 2, .width = 3, .height = 4};
  ret = SensorStreamSetProperty(stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
                                &image_crop_property,
                                sizeof(image_crop_property));
  EXPECT_EQ(-1, ret);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStreamSetProperty_Normal_Success) {
  EXPECT_CALL(*mock_, senscord_stream_set_property(_, _, _, _))
      .WillRepeatedly(testing::Return(0));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  StreamSetPropertyAll(stream);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStreamSetProperty_SensCordError_Error) {
  EXPECT_CALL(*mock_, senscord_stream_set_property(_, _, _, _))
      .WillRepeatedly(testing::Return(-1));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  StreamSetPropertyAll(stream, -1);
}

// -------- //

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStreamGetProperty_Normal_Success) {
  EXPECT_CALL(*mock_, senscord_stream_get_property(_, _, _, _))
      .WillRepeatedly(testing::Return(0));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;

  StreamGetPropertyAll(stream);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStreamGetProperty_SensCordError_Error) {
  EXPECT_CALL(*mock_, senscord_stream_get_property(_, _, _, _))
      .WillRepeatedly(testing::Return(-1));

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;

  StreamGetPropertyAll(stream, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorStreamGetProperty_SensCordErrorStreamNonInitialized) {
  EdgeAppLibSensorStream stream = 0;

  StreamGetPropertyAll(stream, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetProperty_Normal_Success) {
  EXPECT_CALL(*mock_, senscord_channel_get_property(_, _, _, _))
      .WillRepeatedly(testing::Return(0));

  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;

  ChannelGetPropertyAll(channel);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetProperty_SensCordError_Error) {
  EXPECT_CALL(*mock_, senscord_channel_get_property(_, _, _, _))
      .WillRepeatedly(testing::Return(-1));

  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;

  ChannelGetPropertyAll(channel, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetProperty_SensCordErrorChannelNonInitialized) {
  EdgeAppLibSensorChannel channel = 0;

  ChannelGetPropertyAll(channel, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryFileIO) {
  mapped_flag = -1;
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(0, mapped_flag);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryWithInputTensorOnly) {
  mapped_flag = -1;
  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, 0, _))
      .WillRepeatedly(testing::Return(-1));
  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, 1, _))
      .WillRepeatedly(testing::Return(0));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(0, mapped_flag);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryWithOutputTensorOnly) {
  mapped_flag = -1;
  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, 1, _))
      .WillRepeatedly(testing::Return(-1));
  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, 0, _))
      .WillRepeatedly(testing::Return(0));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(0, mapped_flag);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryWithNoChannel) {
  mapped_flag = -1;
  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, 1, _))
      .WillRepeatedly(testing::Return(-1));
  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, 0, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(-1, mapped_flag);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryMap) {
  mapped_flag = -1;
  setEsfMemoryManagerPreadFail();
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(1, mapped_flag);
  resetEsfMemoryManagerPreadSuccess();
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryFailFrame) {
  mapped_flag = -1;
  EXPECT_CALL(*mock_, senscord_stream_get_frame(_, _, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(-1, mapped_flag);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryFailChannel) {
  mapped_flag = -1;
  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, _, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(-1, mapped_flag);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryFailRawDataTest) {
  mapped_flag = -1;
  EXPECT_CALL(*mock_, senscord_channel_get_raw_data_handle(_, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(-1, mapped_flag);
}

TEST_F(EdgeAppLibSensorUnitTest, IsMappedMemoryFailReleaseFrame) {
  mapped_flag = -1;
  EXPECT_CALL(*mock_, senscord_stream_release_frame(_, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorStart(stream);
  EXPECT_EQ(0, ret);
  EXPECT_EQ(-1, mapped_flag);
}

}  // namespace aitrios_sensor_ut
