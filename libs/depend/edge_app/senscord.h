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

#ifndef _SENSCORD_H_
#define _SENSCORD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define SENSCORD_STREAM_KEY "inference_stream"
#define SENSCORD_STREAM_TYPE_INFERENCE_STREAM "inference"
#define SENSCORD_CHANNEL_ID_INFERENCE (0x00000000)
#define SENSCORD_CHANNEL_ID_IMAGE (0x00000001)
#define SENSCORD_RAW_DATA_TYPE_INFERENCE "inference_data"
#define SENSCORD_RAW_DATA_TYPE_IMAGE "image_data"
#define SENSCORD_IMAGE_CROP_PROPERTY_KEY "image_crop_property"
#define SENSCORD_AI_MODEL_BUNDLE_ID_PROPERTY_KEY "ai_model_bundle_id_property"
#define SENSCORD_AI_MODEL_INDEX_PROPERTY_KEY "ai_model_index_property"
#define SENSCORD_POST_PROCESS_AVAILABLE_PROPERTY_KEY \
  "post_process_available_property"
#define SENSCORD_POST_PROCESS_PARAMETER_PROPERTY_KEY \
  "post_process_parameter_property"
#define SENSCORD_INFERENCE_POST_PROCESS_PARAM_SIZE (256)

enum senscord_error_level_t {
  SENSCORD_LEVEL_UNDEFINED = 0,
  SENSCORD_LEVEL_FAIL,
  SENSCORD_LEVEL_FATAL,
};
enum senscord_error_cause_t {
  SENSCORD_ERROR_NONE = 0,
  SENSCORD_ERROR_NOT_FOUND,
  SENSCORD_ERROR_INVALID_ARGUMENT,
  SENSCORD_ERROR_RESOURCE_EXHAUSTED,
  SENSCORD_ERROR_PERMISSION_DENIED,
  SENSCORD_ERROR_BUSY,
  SENSCORD_ERROR_TIMEOUT,
  SENSCORD_ERROR_CANCELLED,
  SENSCORD_ERROR_ABORTED,
  SENSCORD_ERROR_ALREADY_EXISTS,
  SENSCORD_ERROR_INVALID_OPERATION,
  SENSCORD_ERROR_OUT_OF_RANGE,
  SENSCORD_ERROR_DATA_LOSS,
  SENSCORD_ERROR_HARDWARE_ERROR,
  SENSCORD_ERROR_NOT_SUPPORTED,
  SENSCORD_ERROR_UNKNOWN,
  /* extended error cause */
  SENSCORD_ERROR_INVALID_CAMERA_OPERATION_PARAMETER,
};

typedef uint64_t senscord_handle_t;
typedef senscord_handle_t senscord_core_t;
typedef senscord_handle_t senscord_stream_t;
typedef senscord_handle_t senscord_frame_t;
typedef senscord_handle_t senscord_channel_t;

struct senscord_image_crop_property_t {
  uint32_t left;   /* start xpoint */
  uint32_t top;    /* start ypoint */
  uint32_t width;  /* width */
  uint32_t height; /* height */
};
struct senscord_ai_model_bundle_id_property_t {
  uint32_t ai_model_bundle_id;
};
struct senscord_ai_model_index_property_t {
  uint32_t ai_model_index;
};
struct senscord_post_process_available_property_t {
  bool is_aveilable;
};
struct senscord_post_process_parameter_property_t {
  uint8_t param[SENSCORD_INFERENCE_POST_PROCESS_PARAM_SIZE];
};
struct senscord_raw_data_t {
  void *address;
  size_t size;
  char *type;
  uint64_t timestamp; /* nanoseconds timestamp captured by the device */
};

/**
 * @brief Raw data informations.
 * @see senscord::Channel::RawData
 */
struct senscord_raw_data_handle_t {
  uint64_t address;   /**< virtual address */
  uint64_t size;      /**< data size */
  const char *type;   /**< data type*/
  uint64_t timestamp; /**< nanoseconds timestamp captured by the device */
};
struct senscord_status_t {
  enum senscord_error_level_t level;
  enum senscord_error_cause_t cause;
  const char *message;
  /* -------------------------------------------------------- */
  /* Note: message : decode specification                     */
  /*   all 10 digits : exam...0x0123456789                    */
  /*   01   : FileID.                                         */
  /*   2345 : File Line Number                                */
  /*   67   : ret (signed)                                    */
  /*   89   : errno                                           */
  /* -------------------------------------------------------- */
  const char *block; /* internal block from where the error has occurred */
};
enum senscord_status_param_t {
  /** Error message. */
  SENSCORD_STATUS_PARAM_MESSAGE,
  /** Where the error occurred. */
  SENSCORD_STATUS_PARAM_BLOCK,
  /** Trace information. */
  SENSCORD_STATUS_PARAM_TRACE,
};

#define ESF_SENSOR_LATENCY_POINTS_MAX (uint32_t)8

typedef struct {
  uint64_t points[ESF_SENSOR_LATENCY_POINTS_MAX];
} EsfSensorLatencyTimestamps;

#define SENSCORD_REGISTER_ACCESS_64_PROPERTY_KEY "register_access_64_property"
struct senscord_register_access_64_property_t {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint64_t data;    /**< Writing data or read data. */
};
#define SENSCORD_REGISTER_ACCESS_32_PROPERTY_KEY "register_access_32_property"
struct senscord_register_access_32_property_t {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint32_t data;    /**< Writing data or read data. */
};
#define SENSCORD_REGISTER_ACCESS_16_PROPERTY_KEY "register_access_16_property"
struct senscord_register_access_16_property_t {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint16_t data;    /**< Writing data or read data. */
};
#define SENSCORD_REGISTER_ACCESS_8_PROPERTY_KEY "register_access_8_property"
struct senscord_register_access_8_property_t {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint8_t data;     /**< Writing data or read data. */
};

int32_t senscord_core_init(senscord_core_t *core);
int32_t senscord_core_exit(senscord_core_t core);
int32_t senscord_core_open_stream(senscord_core_t core, const char *stream_key,
                                  senscord_stream_t *stream);
int32_t senscord_core_close_stream(senscord_core_t core,
                                   senscord_stream_t stream);
int32_t senscord_stream_start(senscord_stream_t stream);
int32_t senscord_stream_stop(senscord_stream_t stream);
int32_t senscord_stream_get_frame(senscord_stream_t stream,
                                  senscord_frame_t *frame,
                                  int32_t timeout_msec);
int32_t senscord_stream_release_frame(senscord_stream_t stream,
                                      senscord_frame_t frame);
int32_t senscord_stream_get_property(senscord_stream_t stream,
                                     const char *property_key, void *value,
                                     size_t value_size);
int32_t senscord_stream_set_property(senscord_stream_t stream,
                                     const char *property_key,
                                     const void *value, size_t value_size);
int32_t senscord_frame_get_channel_from_channel_id(senscord_frame_t frame,
                                                   uint32_t channel_id,
                                                   senscord_channel_t *channel);
int32_t senscord_channel_get_raw_data(senscord_channel_t channel,
                                      struct senscord_raw_data_t *raw_data);
int32_t senscord_channel_get_raw_data_handle(
    senscord_channel_t channel, struct senscord_raw_data_handle_t *raw_data);
int32_t senscord_channel_get_property(senscord_channel_t channel,
                                      const char *property_key, void *value,
                                      size_t value_size);
enum senscord_error_level_t senscord_get_last_error_level(void);
enum senscord_error_cause_t senscord_get_last_error_cause(void);
int32_t senscord_get_last_error_string(enum senscord_status_param_t param,
                                       char *buffer, uint32_t *length);

int32_t senscord_frame_get_sequence_number(senscord_frame_t frame,
                                           uint64_t *frame_number);
int32_t EsfSensorLatencySetMode(bool is_enable, uint32_t backlog);

int32_t EsfSensorLatencyGetTimestamps(uint64_t sequence_number,
                                      EsfSensorLatencyTimestamps *timestamps);

#ifdef __cplusplus
}
#endif

#endif
