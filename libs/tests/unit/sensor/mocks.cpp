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

#include "memory_manager.hpp"
#include "sensor_unit_test.h"
#include "sensor_unit_test_mock.h"

/**
 * Mock Functions (should be outside namespace)
 */
#ifdef __cplusplus
extern "C" {
#endif
int32_t senscord_core_init(senscord_core_t *core) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_->senscord_core_init(
      core);
}

int32_t senscord_stream_get_frame(senscord_stream_t stream,
                                  senscord_frame_t *frame,
                                  int32_t timeout_msec) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_stream_get_frame(stream, frame, timeout_msec);
}
senscord_error_cause_t senscord_get_last_error_cause(void) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_get_last_error_cause();
}

senscord_error_level_t senscord_get_last_error_level(void) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_get_last_error_level();
}

int32_t senscord_get_last_error_string(enum senscord_status_param_t param,
                                       char *buffer, uint32_t *length) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_get_last_error_string(param, buffer, length);
}

int32_t senscord_core_open_stream(senscord_core_t core, const char *stream_key,
                                  senscord_stream_t *stream) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_core_open_stream(core, stream_key, stream);
}

int32_t senscord_stream_start(senscord_stream_t stream) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_stream_start(stream);
}

int32_t senscord_stream_stop(senscord_stream_t stream) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_stream_stop(stream);
}

int32_t senscord_core_close_stream(senscord_core_t core,
                                   senscord_stream_t stream) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_core_close_stream(core, stream);
}

int32_t senscord_core_exit(senscord_core_t core) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_->senscord_core_exit(
      core);
}

int32_t senscord_frame_get_channel_from_channel_id(
    senscord_frame_t frame, uint32_t channel_id, senscord_channel_t *channel) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_frame_get_channel_from_channel_id(frame, channel_id, channel);
}

int32_t senscord_channel_get_channel_id(senscord_channel_t channel,
                                        uint32_t *channel_id) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_channel_get_channel_id(channel, channel_id);
}

int32_t senscord_channel_get_raw_data(senscord_channel_t channel,
                                      struct senscord_raw_data_t *raw_data) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_channel_get_raw_data(channel, raw_data);
}

int32_t senscord_channel_get_raw_data_handle(
    senscord_channel_t channel, struct senscord_raw_data_handle_t *raw_data) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_channel_get_raw_data_handle(channel, raw_data);
}

int32_t senscord_channel_get_property(senscord_channel_t channel,
                                      const char *property_key, void *value,
                                      size_t value_size) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_channel_get_property(channel, property_key, value, value_size);
}

int32_t senscord_stream_set_property(senscord_stream_t stream,
                                     const char *property_key,
                                     const void *value, size_t value_size) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_stream_set_property(stream, property_key, value, value_size);
}

int32_t senscord_stream_get_property(senscord_stream_t stream,
                                     const char *property_key, void *value,
                                     size_t value_size) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_stream_get_property(stream, property_key, value, value_size);
}

int32_t senscord_stream_release_frame(senscord_stream_t stream,
                                      senscord_frame_t frame) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_stream_release_frame(stream, frame);
}

int32_t senscord_frame_get_sequence_number(senscord_frame_t frame,
                                           uint64_t *frame_number) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->senscord_frame_get_sequence_number(frame, frame_number);
}

int32_t EsfSensorLatencySetMode(bool is_enable, uint32_t backlog) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->EsfSensorLatencySetMode(is_enable, backlog);
}

int32_t EsfSensorLatencyGetTimestamps(uint64_t sequence_number,
                                      EsfSensorLatencyTimestamps *timestamps) {
  return aitrios_sensor_ut::EdgeAppLibSensorUnitTest::mock_
      ->EsfSensorLatencyGetTimestamps(sequence_number, timestamps);
}

#ifdef __cplusplus
}
#endif
