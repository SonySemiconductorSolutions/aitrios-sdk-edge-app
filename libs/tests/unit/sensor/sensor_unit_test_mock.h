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

#ifndef _AITRIOS_SENSOR_UNIT_TEST_MOCK_H_
#define _AITRIOS_SENSOR_UNIT_TEST_MOCK_H_

#include "gmock/gmock.h"
#include "gtest/gtest.h"
// #include "parson/parson.h"

#include "edge_app/senscord.h"
#include "sensor.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;

namespace aitrios_sensor_ut {

class EdgeAppLibSensorUnitTestMock {
 public:
  MOCK_METHOD(int32_t, senscord_core_init, (senscord_core_t * core));

  MOCK_METHOD(int32_t, senscord_stream_get_frame,
              (senscord_stream_t stream, senscord_frame_t *frame,
               int32_t timeout_msec));

  MOCK_METHOD(int32_t, senscord_core_exit, (senscord_core_t core));

  MOCK_METHOD(senscord_error_cause_t, senscord_get_last_error_cause, ());
  MOCK_METHOD(senscord_error_level_t, senscord_get_last_error_level, ());
  MOCK_METHOD(int32_t, senscord_get_last_error_string,
              (enum senscord_status_param_t param, char *buffer,
               uint32_t *length));

  MOCK_METHOD(int32_t, senscord_core_open_stream,
              (senscord_core_t core, const char *stream_key,
               senscord_stream_t *stream));

  MOCK_METHOD(int32_t, senscord_frame_get_channel_from_channel_id,
              (senscord_frame_t frame, uint32_t channel_id,
               senscord_channel_t *channel));

  MOCK_METHOD(int32_t, senscord_channel_get_raw_data,
              (senscord_channel_t channel,
               struct senscord_raw_data_t *raw_data));

  MOCK_METHOD(int32_t, senscord_channel_get_raw_data_handle,
              (senscord_channel_t channel,
               struct senscord_raw_data_handle_t *raw_data));

  MOCK_METHOD(int32_t, senscord_channel_get_property,
              (senscord_channel_t channel, const char *property_key,
               void *value, size_t value_size));

  MOCK_METHOD(int32_t, senscord_stream_get_property,
              (senscord_stream_t stream, const char *property_key, void *value,
               size_t value_size));

  MOCK_METHOD(int32_t, senscord_stream_set_property,
              (senscord_stream_t stream, const char *property_key,
               const void *value, size_t value_size));

  MOCK_METHOD(int32_t, senscord_stream_release_frame,
              (senscord_stream_t stream, senscord_frame_t frame));

  MOCK_METHOD(int32_t, senscord_stream_start, (senscord_stream_t stream));

  MOCK_METHOD(int32_t, senscord_stream_stop, (senscord_stream_t stream));

  MOCK_METHOD(int32_t, senscord_core_close_stream,
              (senscord_core_t core, senscord_stream_t stream));

  MOCK_METHOD(void, updateProperty,
              (EdgeAppLibSensorStream stream, const char *property_key,
               const void *value, size_t value_size));
  MOCK_METHOD(int32_t, senscord_frame_get_sequence_number,
              (senscord_frame_t frame, uint64_t *frame_number));

  MOCK_METHOD(int32_t, EsfSensorLatencySetMode,
              (bool is_enable, uint32_t backlog));

  MOCK_METHOD(int32_t, EsfSensorLatencyGetTimestamps,
              (uint64_t sequence_number,
               EsfSensorLatencyTimestamps *timestamps));
};

}  // namespace aitrios_sensor_ut

#endif  // _AITRIOS_SENSOR_UNIT_TEST_MOCK_H_
