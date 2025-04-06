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

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "edge_app/senscord.h"
#include "memory_manager.hpp"
#include "sensor.h"
#include "sensor_def.h"
#include "sm_api.hpp"

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Determine if the file-based interface or memory mapping (Map) is recommended
 * for accessing high-memory (himem).
 * 0: Map, 1: File
 */
int8_t mapped_flag = -1;
pthread_mutex_t flag_mutex = PTHREAD_MUTEX_INITIALIZER;
/*
 * Helper function to check if the memory is mapped or if file access is better.
 * Returns:
 *   0 if memory mapping (Map) is recommended,
 *   1 if file-based access is recommended,
 *  -1 in case of an error.
 */
static int8_t IsMappedMemory(EdgeAppLibSensorStream stream) {
  EdgeAppLibSensorFrame frame;
  int8_t mapped = -1;

  // Retrieve a frame from the stream
  int32_t result = senscord_stream_get_frame(stream, &frame, -1);
  if (result != 0) {
    LOG_ERR("senscord_stream_get_frame failed with error: %d", result);
    return -1;
  }

  // Retrieve the first channel from the frame
  EdgeAppLibSensorChannel channel;
  result = senscord_frame_get_channel_from_channel_id(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel);
  if (result != 0) {
    result = senscord_frame_get_channel_from_channel_id(
        frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);
    if (result != 0) {
      LOG_ERR(
          "senscord_frame_get_channel_from_channel_id failed with error: %d",
          result);
      senscord_stream_release_frame(stream, frame);
      return -1;
    }
  }

  // Get the raw data handle for the channel
  struct senscord_raw_data_handle_t raw_data_handle;
  result = senscord_channel_get_raw_data_handle(channel, &raw_data_handle);
  if (result != 0) {
    LOG_ERR("senscord_channel_get_raw_data_handle failed with error: %d",
            result);
    senscord_stream_release_frame(stream, frame);
    return -1;
  }

  // Test file-based access using EsfMemoryManagerPread
  uint8_t test = 0;
  size_t size = 1;
  EsfMemoryManagerResult mem_err =
      EsfMemoryManagerPread(raw_data_handle.address, &test, 1, 0, &size);

  if (mem_err == kEsfMemoryManagerResultSuccess) {
    LOG_INFO("Pread succeeded: File-based access is recommended.");
    mapped = 0;
  } else {
    LOG_INFO("Pread failed: Memory mapping (Map) is recommended.");
    mapped = 1;
  }

  // Release the frame after processing
  result = senscord_stream_release_frame(stream, frame);
  if (result != 0) {
    LOG_ERR("senscord_stream_release_frame failed with error: %d", result);
    return -1;
  }

  return mapped;
}

/**
 * Wrapper function to start the sensor stream.
 * Internally checks the memory condition (file vs mapped access) only once.
 *
 * Returns:
 *   0 on success,
 *  Non-zero on failure.
 */
int32_t SensorStart(EdgeAppLibSensorStream stream) {
  LOG_TRACE("SensorStart initiated.");

  if (stream == 0) {
    LOG_ERR("stream is NULL");
    return -1;
  }
  // Start the sensor stream
  int32_t result = senscord_stream_start(stream);
  if (result != 0) {
    LOG_ERR("senscord_stream_start failed with error: %d", result);
    return result;
  }

  // Check memory access mode only once
  pthread_mutex_lock(&flag_mutex);
  if (mapped_flag == -1) {
    mapped_flag = IsMappedMemory(stream);
  }
  pthread_mutex_unlock(&flag_mutex);

  LOG_TRACE("SensorStart completed with result: %d", result);
  return result;
}

/**
 * Sensor Wrapper implementation API (to call senscord_stream_stop internally).
 */
int32_t SensorStop(EdgeAppLibSensorStream stream) {
  LOG_TRACE("EdgeAppLibSensorStop start");

  if (stream == 0) {
    LOG_ERR("stream is NULL");
    return -1;
  }
  // request stream stop to senscord
  int32_t result = senscord_stream_stop(stream);
  if (result != 0) {
    LOG_ERR("senscord_stream_stop result %d", result);
    return result;
  }

  LOG_TRACE("EdgeAppLibSensorStop end");
  return result;
}

/**
 * Sensor Wrapper implementation API (to call
 * senscord_stream_get_property internally).
 */
int32_t SensorStreamGetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, void *value,
                                size_t value_size) {
  LOG_TRACE("EdgeAppLibSensorStreamGetProperty start");

  if (stream == 0 || property_key == nullptr || value == nullptr ||
      value_size == 0) {
    LOG_ERR("stream, property_key, value or value_size is NULL");
    return -1;
  }
  // call senscord function
  int32_t result =
      senscord_stream_get_property(stream, property_key, value, value_size);
  LOG_TRACE("senscord_stream_get_property %s %d", property_key, result);
  return result;
}

/**
 * Sensor Wrapper implementation API (to call senscord_stream_set_property
 * internally).
 */
int32_t SensorStreamSetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, const void *value,
                                size_t value_size) {
  LOG_TRACE("EdgeAppLibSensorStreamSetProperty start");

  if (stream == 0 || property_key == nullptr || value == nullptr ||
      value_size == 0) {
    LOG_ERR("stream, property_key, value or value_size is NULL");
    return -1;
  }

  // call senscord function
  int32_t result =
      senscord_stream_set_property(stream, property_key, value, value_size);
  LOG_TRACE("senscord_stream_set_property %s %d", property_key, result);

  if (result == 0) {
    // property has been correctly set => update DTDL
    updateProperty(stream, property_key, value, value_size);
  }

  return result;
}

/**
 * Sensor Wrapper implementation API (to call senscord_channel_get_property
 * internally).
 */
int32_t SensorChannelGetProperty(EdgeAppLibSensorChannel channel,
                                 const char *property_key, void *value,
                                 size_t value_size) {
  LOG_TRACE("EdgeAppLibSensorChannelGetProperty start");

  if (channel == 0 || property_key == nullptr || value == nullptr ||
      value_size == 0) {
    LOG_ERR("channel, property_key, value or value_size is NULL");
    return -1;
  }
  // call senscord function
  int32_t result =
      senscord_channel_get_property(channel, property_key, value, value_size);
  LOG_TRACE("senscord_channel_get_property %s %d", property_key, result);
  return result;
}
#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib
