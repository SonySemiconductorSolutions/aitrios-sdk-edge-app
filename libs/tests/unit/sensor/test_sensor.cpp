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

#include <execinfo.h>
#include <stdint.h>
#include <string.h>

#include "edge_app/senscord.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "parson/parson.h"
#include "sensor.h"
#include "sensor_unit_test.h"
#include "sensor_unit_test_mock.h"

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

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreInit_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = 0L;
  int32_t ret = SensorCoreInit(&core);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreInit_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EXPECT_CALL(*mock_, senscord_core_init(_))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorCore core = 0L;
  int32_t ret = SensorCoreInit(&core);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreExit_NormalSuccess) {
  EdgeAppLibSensorCore core = DUMMY_HANDLE_CORE;
  int32_t ret = SensorCoreExit(core);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreExit_Abnormal_Error) {
  EXPECT_CALL(*mock_, senscord_core_exit(_))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorCore core = DUMMY_HANDLE_CORE;
  int32_t ret = SensorCoreExit(core);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreExit_NullCore) {
  EdgeAppLibSensorCore core = 0;
  int32_t ret = SensorCoreExit(core);
  ASSERT_EQ(ret, -1);
  EXPECT_CALL(*mock_, senscord_core_exit(_)).Times(0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreOpenStream_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = DUMMY_HANDLE_CORE;
  const char *key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
  EdgeAppLibSensorStream stream = 0L;
  int32_t ret = SensorCoreOpenStream(core, key, &stream);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorCoreOpenStream_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = DUMMY_HANDLE_CORE;
  EdgeAppLibSensorStream stream = 0L;

  EXPECT_CALL(*mock_, senscord_core_open_stream(_, _, _))
      .WillRepeatedly(testing::Return(-1));
  const char *key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
  int32_t ret = SensorCoreOpenStream(core, key, &stream);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreOpenStream_BoundaryCheck) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = DUMMY_HANDLE_CORE;
  EdgeAppLibSensorStream stream = 0L;
  int32_t ret = 0;

  /* inference_stream */
  ret = SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
  ASSERT_EQ(ret, 0);

  EXPECT_CALL(*mock_, senscord_core_open_stream(_, _, _))
      .WillRepeatedly(testing::Return(-1));

  /* Wrong StreamKey */
  ret = SensorCoreOpenStream(core, "xyz", &stream);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreOpenStream_CoreNull) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream = 0L;
  int32_t ret = 0;

  /* inference_stream */
  ret = SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
  ASSERT_EQ(ret, -1);

  EXPECT_CALL(*mock_, senscord_core_open_stream(_, _, _)).Times(0);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorCoreCloseStream_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = DUMMY_HANDLE_CORE;
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorCoreCloseStream(core, stream);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorCoreCloseStream_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = DUMMY_HANDLE_CORE;

  EXPECT_CALL(*mock_, senscord_core_close_stream(_, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  int32_t ret = SensorCoreCloseStream(core, stream);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorCoreCloseStream_StreamNull) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream = 0;
  int32_t ret = SensorCoreCloseStream(core, stream);
  ASSERT_EQ(ret, -1);

  EXPECT_CALL(*mock_, senscord_core_close_stream(_, _)).Times(0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorGetFrame_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  EdgeAppLibSensorFrame frame = 0L;
  int32_t ret = SensorGetFrame(stream, &frame, -1);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorGetFrame_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;

  EXPECT_CALL(*mock_, senscord_stream_get_frame(_, _, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorFrame frame = 0L;
  int32_t ret = SensorGetFrame(stream, &frame, -1);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorGetFrame_BoundaryCheck) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  EdgeAppLibSensorFrame frame = 0L;

  /* -1 <= timeout_msec */
  int32_t ret = SensorGetFrame(stream, &frame, 1);
  ASSERT_EQ(ret, 0);

  EXPECT_CALL(*mock_, senscord_stream_get_frame(_, _, _))
      .WillRepeatedly(testing::Return(-1));

  /* timeout_msec < -1 */
  ret = SensorGetFrame(stream, &frame, -10);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorGetFrame_NullStream) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = 0;
  EdgeAppLibSensorFrame frame = 0L;

  /* -1 <= timeout_msec */
  int32_t ret = SensorGetFrame(stream, &frame, 1);
  ASSERT_EQ(ret, -1);

  EXPECT_CALL(*mock_, senscord_stream_get_frame(_, _, _)).Times(0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorReleaseFrame_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorReleaseFrame(stream, frame);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorGetFrame_NullFrame) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;

  /* -1 <= timeout_msec */
  int32_t ret = SensorGetFrame(stream, NULL, 1);
  ASSERT_EQ(ret, -1);

  EXPECT_CALL(*mock_, senscord_stream_get_frame(_, _, _)).Times(0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorReleaseFrame_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;

  EXPECT_CALL(*mock_, senscord_stream_release_frame(_, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorReleaseFrame(stream, frame);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorReleaseFrame_NullStream) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = 0;
  EdgeAppLibSensorFrame frame = 0;
  int32_t ret = SensorReleaseFrame(stream, frame);
  ASSERT_EQ(ret, -1);
  EXPECT_CALL(*mock_, senscord_stream_release_frame(_, _)).Times(0);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorFrameGetChannelFromChannelId_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  EdgeAppLibSensorChannel channel = 0L;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorFrameGetChannelFromChannelId_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;

  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, _, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorChannel channel = 0L;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorFrameGetChannelFromChannelId_BoundaryCheck) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorStream stream = DUMMY_HANDLE_STREAM;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  EdgeAppLibSensorChannel channel = 0L;

  /* ChannelID: 0 */
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel);
  ASSERT_EQ(ret, 0);

  /* ChannelID: 1 */
  ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  ASSERT_EQ(ret, 0);

  EXPECT_CALL(*mock_, senscord_frame_get_channel_from_channel_id(_, _, _))
      .WillRepeatedly(testing::Return(-1));

  /* ChannnelID: 2 */
  uint32_t channel_id = 0x00000002;
  ret = SensorFrameGetChannelFromChannelId(frame, channel_id, &channel);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetRawData_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  mapped_flag = -1;
  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  ASSERT_EQ(ret, 0);
  EdgeAppLibSensorRawData raw_data = {};
  EXPECT_CALL(*mock_, senscord_channel_get_channel_id(_, _))
      .WillOnce(DoAll(
          SetArgPointee<1>(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE),
          Return(0)));
  ret = SensorChannelGetRawData(channel, &raw_data);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetRawData_NormalSuccess_FileIO) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  mapped_flag = 0;
  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  ASSERT_EQ(ret, 0);
  EdgeAppLibSensorRawData raw_data = {};
  EXPECT_CALL(*mock_, senscord_channel_get_channel_id(_, _))
      .WillOnce(DoAll(
          SetArgPointee<1>(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE),
          Return(0)));
  ret = SensorChannelGetRawData(channel, &raw_data);
  free(raw_data.address);  // this is the restriction of FileIO mock
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorChannelGetRawDataFailFileIO) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_CALL(*mock_, senscord_channel_get_raw_data_handle(_, _))
      .WillRepeatedly(testing::Return(-1));
  mapped_flag = 0;
  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  ASSERT_EQ(ret, 0);
  EdgeAppLibSensorRawData raw_data = {};
  EXPECT_CALL(*mock_, senscord_channel_get_channel_id(_, _))
      .WillOnce(DoAll(
          SetArgPointee<1>(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE),
          Return(0)));
  ret = SensorChannelGetRawData(channel, &raw_data);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetRawData_NormalSuccess_FileIOMeta) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  mapped_flag = 0;
  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel);
  ASSERT_EQ(ret, 0);
  EdgeAppLibSensorRawData raw_data = {};
  EXPECT_CALL(*mock_, senscord_channel_get_channel_id(_, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT),
                Return(0)));
  ret = SensorChannelGetRawData(channel, &raw_data);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetRawData_NormalSuccess_MapIO) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  mapped_flag = 1;
  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  ASSERT_EQ(ret, 0);
  EdgeAppLibSensorRawData raw_data = {};
  EXPECT_CALL(*mock_, senscord_channel_get_channel_id(_, _))
      .WillOnce(DoAll(
          SetArgPointee<1>(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE),
          Return(0)));
  ret = SensorChannelGetRawData(channel, &raw_data);
  ASSERT_EQ(ret, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, EdgeAppLibSensorChannelGetRawData_Fail_MapIO) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_CALL(*mock_, senscord_channel_get_raw_data(_, _))
      .WillRepeatedly(testing::Return(-1));
  mapped_flag = 1;
  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  ASSERT_EQ(ret, 0);
  EdgeAppLibSensorRawData raw_data = {};
  EXPECT_CALL(*mock_, senscord_channel_get_channel_id(_, _))
      .WillOnce(DoAll(
          SetArgPointee<1>(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE),
          Return(0)));
  ret = SensorChannelGetRawData(channel, &raw_data);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorChannelGetRawData_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorChannel channel = DUMMY_HANDLE_CHANNEL;
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  int32_t ret = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
  ASSERT_EQ(ret, 0);
  EXPECT_CALL(*mock_, senscord_channel_get_raw_data(_, _))
      .WillRepeatedly(testing::Return(-1));
  EdgeAppLibSensorRawData raw_data = {};
  EXPECT_CALL(*mock_, senscord_channel_get_channel_id(_, _))
      .WillOnce(DoAll(
          SetArgPointee<1>(AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE),
          Return(0)));
  ret = SensorChannelGetRawData(channel, &raw_data);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorInputDataTypeEnableChannel_NormalSuccess) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  int32_t ret = SensorInputDataTypeEnableChannel(&enabled, 0, true);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(enabled.count, 1);
  ASSERT_EQ(enabled.channels[0], 0);

  ret = SensorInputDataTypeEnableChannel(&enabled, 1, true);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(enabled.count, 2);
  ASSERT_EQ(enabled.channels[0], 0);
  ASSERT_EQ(enabled.channels[1], 1);

  ret = SensorInputDataTypeEnableChannel(&enabled, 0, false);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(enabled.count, 1);
  ASSERT_EQ(enabled.channels[0], 1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorInputDataTypeEnableChannel_Abnormal_Error) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  int32_t ret = SensorInputDataTypeEnableChannel(nullptr, 0, true);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorInputDataTypeEnableChannel_BoundaryCheck) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  int32_t ret = 0;

  // Enable the max number of channels
  for (uint32_t i = 0; i < AITRIOS_SENSOR_CHANNEL_LIST_MAX; i++) {
    ret = SensorInputDataTypeEnableChannel(&enabled, i, true);
    ASSERT_EQ(ret, 0);
  }

  // Any channel enabled after should fail
  ret = SensorInputDataTypeEnableChannel(&enabled,
                                         AITRIOS_SENSOR_CHANNEL_LIST_MAX, true);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, SensorGetFrameLatencyTest_SuccessCase) {
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  uint64_t sequence_number = 0;
  EdgeAppLibLatencyTimestamps info;

  EXPECT_CALL(*mock_, senscord_frame_get_sequence_number)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(12345),
                                 ::testing::Return(0)));

  EXPECT_CALL(*mock_, EsfSensorLatencyGetTimestamps)
      .WillOnce(::testing::DoAll(
          ::testing::Invoke(
              [](uint64_t, EsfSensorLatencyTimestamps *timestamps) {
                for (size_t i = 0; i < ESF_SENSOR_LATENCY_POINTS_MAX; ++i) {
                  timestamps->points[i] = i * 100;
                }
              }),
          ::testing::Return(0)));

  int32_t result = SensorGetFrameLatency(frame, &sequence_number, &info);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(sequence_number, 12345);
  for (size_t i = 0; i < ESF_SENSOR_LATENCY_POINTS_MAX; ++i) {
    EXPECT_EQ(info.points[i], i * 100);
  }
}

TEST_F(EdgeAppLibSensorUnitTest, SensorGetFrameLatencyTest_FailureCase) {
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  uint64_t sequence_number = 0;
  EdgeAppLibLatencyTimestamps info;

  EXPECT_CALL(*mock_, EsfSensorLatencyGetTimestamps)
      .WillOnce(::testing::Return(-1));

  int32_t result = SensorGetFrameLatency(frame, &sequence_number, &info);

  EXPECT_EQ(result, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, SensorGetFrameLatencyTestSequenceNumber) {
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  uint64_t sequence_number = 0;
  EdgeAppLibLatencyTimestamps info;

  EXPECT_CALL(*mock_, senscord_frame_get_sequence_number)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(12345),
                                 ::testing::Return(-1)));
  int32_t result = SensorGetFrameLatency(frame, &sequence_number, &info);

  EXPECT_EQ(result, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, SensorGetFrameLatencyTest_TimestampsFailure) {
  EdgeAppLibSensorFrame frame = DUMMY_HANDLE_FRAME;
  uint64_t sequence_number = 0;
  EdgeAppLibLatencyTimestamps info = {};

  EXPECT_CALL(*mock_, senscord_frame_get_sequence_number)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(12345),
                                 ::testing::Return(0)));

  EXPECT_CALL(*mock_, EsfSensorLatencyGetTimestamps)
      .WillOnce(::testing::Return(-1));

  int32_t result = SensorGetFrameLatency(frame, &sequence_number, &info);

  EXPECT_EQ(result, -1);
  EXPECT_EQ(sequence_number, 12345);
  for (size_t i = 0; i < ESF_SENSOR_LATENCY_POINTS_MAX; ++i) {
    EXPECT_EQ(info.points[i], 0);
  }
}

TEST_F(EdgeAppLibSensorUnitTest, SensorGetFrameLatencyTest_NullFrame) {
  EdgeAppLibSensorFrame frame = 0;
  uint64_t sequence_number = 0;
  EdgeAppLibLatencyTimestamps info = {};

  int32_t result = SensorGetFrameLatency(frame, &sequence_number, &info);

  EXPECT_EQ(result, -1);
  EXPECT_EQ(sequence_number, 0);
  EXPECT_CALL(*mock_, senscord_frame_get_sequence_number).Times(0);
  EXPECT_CALL(*mock_, EsfSensorLatencyGetTimestamps).Times(0);
}

TEST_F(EdgeAppLibSensorUnitTest, SensorLatencySetModeTest_SuccessCase) {
  EXPECT_CALL(*mock_, EsfSensorLatencySetMode).WillOnce(::testing::Return(0));

  int32_t result = SensorLatencySetMode(true, 10);

  EXPECT_EQ(result, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, SensorLatencySetModeTest_FailureCase) {
  EXPECT_CALL(*mock_, EsfSensorLatencySetMode).WillOnce(::testing::Return(-1));

  int32_t result = SensorLatencySetMode(true, 10);

  EXPECT_EQ(result, -1);
}

TEST_F(EdgeAppLibSensorUnitTest, SensorLatencySetModeTest_Disable) {
  EXPECT_CALL(*mock_, EsfSensorLatencySetMode).WillOnce(::testing::Return(0));

  int32_t result = SensorLatencySetMode(false, 10);

  EXPECT_EQ(result, 0);
}

TEST_F(EdgeAppLibSensorUnitTest, SensorLatencySetModeTest_DisableFailure) {
  EXPECT_CALL(*mock_, EsfSensorLatencySetMode).WillOnce(::testing::Return(-1));

  int32_t result = SensorLatencySetMode(false, 10);

  EXPECT_EQ(result, -1);
}

}  // namespace aitrios_sensor_ut
