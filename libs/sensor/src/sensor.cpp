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

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "edge_app/senscord.h"
#include "memory_manager.hpp"
#include "process_format.hpp"
#include "sensor_def.h"
#include "sm_api.hpp"

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif
extern int8_t mapped_flag;
int32_t SensorCoreInit(EdgeAppLibSensorCore *core) {
  LOG_TRACE("SensorCoreInit start");
  int32_t result = senscord_core_init(core);
  if (result != 0) {
    LOG_ERR("senscord_core_init %d", result);
  }

  LOG_TRACE("SensorCoreInit end");
  return result;
}

/**
 * The same as senscord_core_exit
 */
int32_t SensorCoreExit(EdgeAppLibSensorCore core) {
  LOG_TRACE("SensorCoreExit start");

  if (core == 0) {
    LOG_ERR("core is NULL");
    return -1;
  }
  int32_t result = senscord_core_exit(core);
  if (result != 0) {
    LOG_ERR("senscord_core_exit %d", result);
  }

  LOG_TRACE("SensorCoreExit end");
  return result;
}

/**
 * The same as senscord_core_open_stream
 */
int32_t SensorCoreOpenStream(EdgeAppLibSensorCore core, const char *stream_key,
                             EdgeAppLibSensorStream *stream) {
  LOG_TRACE("SensorCoreOpenStream start");
  if (core == 0) {
    LOG_ERR("core is NULL");
    return -1;
  }
  int32_t result = senscord_core_open_stream(core, stream_key, stream);
  if (result != 0) {
    LOG_ERR("senscord_core_open_stream %d", result);
    return result;
  }
  LOG_TRACE("SensorCoreOpenStream end");
  return result;
}

/**
 * The same as senscord_core_close_stream
 */
int32_t SensorCoreCloseStream(EdgeAppLibSensorCore core,
                              EdgeAppLibSensorStream stream) {
  LOG_TRACE("SensorCoreCloseStream start");

  if (core == 0 || stream == 0) {
    LOG_ERR("stream is NULL");
    return -1;
  }
  int32_t result = senscord_core_close_stream(core, stream);
  if (result != 0) {
    LOG_ERR("senscord_core_close_stream %d", result);
  }

  LOG_TRACE("SensorCoreCloseStream end");
  return result;
}

/**
 * The same as senscord_stream_get_frame
 */
int32_t SensorGetFrame(EdgeAppLibSensorStream stream,
                       EdgeAppLibSensorFrame *frame, int32_t timeout_msec) {
  LOG_TRACE("SensorGetFrame start");

  if (stream == 0) {
    LOG_ERR("stream is NULL");
    return -1;
  }
  if (frame == nullptr) {
    LOG_ERR("frame is NULL");
    return -1;
  }
  int32_t result = senscord_stream_get_frame(stream, frame, timeout_msec);
  if (result != 0) {
    LOG_ERR("senscord_stream_get_frame %d", result);
  }

  LOG_TRACE("SensorGetFrame end");
  return result;
}

/**
 * The same as senscord_stream_release_frame
 */
int32_t SensorReleaseFrame(EdgeAppLibSensorStream stream,
                           EdgeAppLibSensorFrame frame) {
  LOG_TRACE("SensorReleaseFrame start");
  if (stream == 0 || frame == 0) {
    LOG_ERR("stream or frame is NULL");
    return -1;
  }
  int32_t result = senscord_stream_release_frame(stream, frame);
  if (result != 0) {
    LOG_INFO("senscord_stream_release_frame %d %08x", result, frame);

    // Get detailed error information
    EdgeAppLibSensorErrorLevel level = SensorGetLastErrorLevel();
    EdgeAppLibSensorErrorCause cause = SensorGetLastErrorCause();

    char error_buffer[256] = {0};
    uint32_t buffer_length = sizeof(error_buffer);
    if (SensorGetLastErrorString(AITRIOS_SENSOR_STATUS_PARAM_MESSAGE,
                                 error_buffer, &buffer_length) == 0) {
      LOG_DBG("Detailed error: Level=%d, Cause=%d, Message=%s", level, cause,
              error_buffer);
    } else {
      LOG_DBG("Detailed error: Level=%d, Cause=%d (Failed to get error string)",
              level, cause);
    }

    // Handle "not managed frame" error - likely indicates core was already
    // exited
    if (strstr(error_buffer, "not managed frame") != NULL) {
      LOG_INFO("Frame is no longer managed at host side");
      // Don't treat this as a critical error since cleanup may have already
      // occurred
      result = 0;
    }
  }

  LOG_TRACE("SensorReleaseFrame end");
  return result;
}

int32_t SensorLatencySetMode(bool is_enable, uint32_t backlog) {
  LOG_TRACE("SensorLatencySetMode start");

  int32_t result = EsfSensorLatencySetMode(is_enable, backlog);
  if (result != 0) {
    LOG_ERR("EsfSensorLatencySetMode %d", result);
  }
  return result;
}

/**
 * Get the latency timestamps of specified sequence number's frame
 */
int32_t SensorGetFrameLatency(EdgeAppLibSensorFrame frame,
                              uint64_t *sequence_number,
                              EdgeAppLibLatencyTimestamps *info) {
  LOG_TRACE("SensorGetFrameLatency start");

  if (frame == 0 || sequence_number == nullptr || info == nullptr) {
    LOG_ERR("frame, sequence_number or info is NULL");
    return -1;
  }

  int32_t result = senscord_frame_get_sequence_number(frame, sequence_number);
  if (result != 0) {
    LOG_ERR("senscord_frame_get_sequence_number %d", result);
    return result;
  }

  result = EsfSensorLatencyGetTimestamps(*sequence_number,
                                         (EsfSensorLatencyTimestamps *)info);
  if (result != 0) {
    LOG_ERR("EsfSensorLatencyGetTimestamps %d", result);
  }

  LOG_TRACE("SensorGetFrameLatency end");
  return result;
}

/**
 * Retrieves raw data handle from the specified channel and maps it.
 *
 * This function uses the senscord_channel_get_raw_data_handle API to fetch the
 * raw data handle The function populates the provided raw_data structure with
 * the handle's details (address, size, timestamp). If the handle retrieval
 * fails, an error is logged and the error code is returned.
 *
 * @param channel Pointer to the channel from which the raw data handle is to be
 * retrieved.
 * @param raw_data Pointer to a structure where the raw data handle details will
 * be stored.
 * @param mapped_flag Flag to indicate whether the raw data handle should be
 * @return int32_t Returns 0 on success, or an error code on failure.
 */
static int32_t MemoryRefAccess(EdgeAppLibSensorChannel channel,
                               struct EdgeAppLibSensorRawData *raw_data,
                               int8_t mapped_flag) {
  int ret = -1;
  if (mapped_flag == 0) {  // For memory constrained case
    struct senscord_raw_data_handle_t raw_data_handle = {0};
    ret = senscord_channel_get_raw_data_handle(channel, &raw_data_handle);
    if (ret != 0) {
      LOG_ERR("senscord_channel_get_raw_data_handle %d", ret);
      return -1;
    }
    EdgeAppLibSensorRawMemoryRef raw_data_tmp = {0};

    raw_data_tmp.address.type = MEMORY_MANAGER_FILE_TYPE;
    raw_data_tmp.address.u.esf_handle = raw_data_handle.address;
    raw_data_tmp.size = raw_data_handle.size;
    raw_data_tmp.timestamp = raw_data_handle.timestamp;

    // Get codec settings for processing the raw data
    JSON_Object *json_object = getCodecSettings();
    int codec_number = json_object_get_number(json_object, "format");

    void *codec_buffer = NULL;
    int32_t codec_size = 0;

    // Retrieve Channel properties for image processing
    EdgeAppLibSensorImageProperty image_property = {0};
    ret = senscord_channel_get_property(
        channel, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY, &image_property,
        sizeof(image_property));

    // Process the raw data based on the specified format
    ProcessFormatResult process_format_ret =
        ProcessFormatInput(raw_data_tmp.address, raw_data_tmp.size,
                           (ProcessFormatImageType)codec_number,
                           (EdgeAppLibImageProperty *)&image_property,
                           raw_data_tmp.timestamp, &codec_buffer, &codec_size);
    if (process_format_ret != kProcessFormatResultOk) {
      LOG_ERR("ProcessFormatInput failed. Exit with return %d.",
              process_format_ret);
      return -1;
    }

    // Populate the raw data structure with the processed data
    raw_data->address =
        codec_buffer;  // Assuming address is a union with a pointer
    raw_data->size = codec_size;
    raw_data->timestamp = raw_data_tmp.timestamp;
  } else {
    struct senscord_raw_data_t raw_data_tmp = {0};
    ret = senscord_channel_get_raw_data(channel, &raw_data_tmp);
    if (ret != 0) {
      LOG_ERR("senscord_channel_get_raw_data %d", ret);
      return -1;
    }
    raw_data->address = raw_data_tmp.address;
    raw_data->size = raw_data_tmp.size;
    raw_data->timestamp = raw_data_tmp.timestamp;
  }
  return ret;
}

/**
 * Retrieves raw data directly from the specified channel.
 *
 * This function uses the senscord_channel_get_raw_data API to fetch the raw
 * data If the data retrieval fails, an error is logged.
 *
 * @param channel The channel from which raw data is to be retrieved.
 * @param raw_data Pointer to a structure where the retrieved raw data will be
 * stored.
 * @return int32_t Returns 0 on success, or an error code on failure.
 */
static int32_t DataAccess(EdgeAppLibSensorChannel channel,
                          struct EdgeAppLibSensorRawData *raw_data) {
  int32_t result = senscord_channel_get_raw_data(
      channel, reinterpret_cast<senscord_raw_data_t *>(raw_data));
  return result;
}

static bool IsInferenceMetaChannel(uint32_t channel_id) {
  return channel_id == AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT ||
         channel_id == AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_RAW_IMAGE;
}

/**
 * The same as senscord_frame_get_channel_from_channel_id
 */
int32_t SensorFrameGetChannelFromChannelId(EdgeAppLibSensorFrame frame,
                                           uint32_t channel_id,
                                           EdgeAppLibSensorChannel *channel) {
  LOG_TRACE("SensorFrameGetChannelFromChannelId start");

  int32_t result =
      senscord_frame_get_channel_from_channel_id(frame, channel_id, channel);
  if (result != 0) {
    LOG_ERR("senscord_frame_get_channel_from_channel_id %d", result);
    return result;
  }
  LOG_TRACE("SensorFrameGetChannelFromChannelId end");
  return result;
}

/**
 * The same as senscord_channel_get_raw_data
 */
int32_t SensorChannelGetRawData(EdgeAppLibSensorChannel channel,
                                struct EdgeAppLibSensorRawData *raw_data) {
  LOG_TRACE("SensorChannelGetRawData start");
  int result = -1;

  // Get the channel ID from the channel handle
  uint32_t channel_id;
  int32_t ret = senscord_channel_get_channel_id(channel, &channel_id);
  if (ret != 0) {
    LOG_ERR("senscord_channel_get_channel_id failed with %" PRId32 ".", ret);
    return -1;
  }

  LOG_WARN("mapped_flag: %d, channel_id: %u", mapped_flag, channel_id);

  if (mapped_flag != -1) {
    if (IsInferenceMetaChannel(channel_id)) {
      /* Metadata should be accessed by address based */
      return DataAccess(channel, raw_data);
    } else {
      return MemoryRefAccess(channel, raw_data, mapped_flag);
    }
  } else {
    return DataAccess(channel, raw_data);
  }
}

/**
 * Helper function to modify EdgeAppLibSensorInputDataTypeProperty.
 */
int32_t SensorInputDataTypeEnableChannel(
    EdgeAppLibSensorInputDataTypeProperty *property, uint32_t channel_id,
    bool enable) {
  if (property == NULL) {
    LOG_ERR("SensorInputDataTypeEnableChannel");
    return -1;
  }

  // Search for the channel in case it is already enabled
  uint32_t i = 0;
  for (; i < property->count; i++) {
    if (property->channels[i] == channel_id) {
      break;
    }
  }

  if (enable) {
    if (i == property->count) {
      // Channel not found, add it
      if (property->count < AITRIOS_SENSOR_CHANNEL_LIST_MAX) {
        property->channels[property->count++] = channel_id;
      } else {
        LOG_ERR(
            "SensorInputDataTypeEnableChannel too many channels "
            "enabled");
        return -1;
      }
    }
  } else {  // disable
    if (i != property->count) {
      // Channel found, remove it
      while (++i < property->count) {
        property->channels[i - 1] = property->channels[i];
      }
      --property->count;
    }
  }

  return 0;
}
#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib
