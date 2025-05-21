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

#include "sm.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_export.h"
#include "data_processor_api.hpp"
#include "log.h"
#include "send_data.h"
#include "sensor.h"
#include "sm_utils.hpp"

#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"
#define DATA_EXPORT_AWAIT_TIMEOUT -1
#define SENSOR_GET_FRAME_TIMEOUT 5000

using namespace EdgeAppLib;

EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;

static double time_ms() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000.0;
}

/**
 * @brief Defines how many frames to measure before uploading the times.
 */
#define NUM_FRAMES_PER_FLUSH 10
#define NUM_LATENCY_INSIDE_SENSOR_MAX 5
/**
 * @brief Defines the different parts of the code we are going to measure during
 * one iteration.
 */
typedef enum {
  FB_INSIDE_SENSOR_0,
  FB_INSIDE_SENSOR_1,
  FB_INSIDE_SENSOR_2,
  FB_INSIDE_SENSOR_3,
  FB_INSIDE_SENSOR_4,
  FB_ON_ITERATE,
  FB_GET_FRAME,
  FB_RELEASE_FRAME,
  FB_IT_GET_RAW_DATA,
  FB_IT_UPLOAD,
  FB_OT_GET_RAW_DATA,
  FB_OT_UPLOAD,
  NUM_FRAME_BLOCKS,
} FrameBlockID;

static const char *s_frame_block_id_str[NUM_FRAME_BLOCKS] = {
    // clang-format off
  "InsideSensor0",
  "InsideSensor1",
  "InsideSensor2",
  "InsideSensor3",
  "InsideSensor4",
  "onIterate",
  "GetFrame",
  "ReleaseFrame",
  "IT_GetRawData",
  "IT_Upload",
  "OT_GetRawData",
  "OT_Upload",
    // clang-format on
};

/**
 * @brief Stores the frame time measurements and uploads them once it is full.
 */
class FrameTimesCollector {
  double measurements_[NUM_FRAMES_PER_FLUSH][NUM_FRAME_BLOCKS] = {};
  double timestamps_[NUM_FRAME_BLOCKS] = {};
  int frame_ = 0;

  void Flush() {
    // Build output JSON with this schema:
    //   [
    //     { "onIterate": 1.23, "GetFrame": 4.56, ... },
    //     ...
    //   ]
    //
    JSON_Value *times_array_value = json_value_init_array();
    JSON_Array *times_array = json_array(times_array_value);
    for (int frame = 0; frame < NUM_FRAMES_PER_FLUSH; frame++) {
      JSON_Value *frame_obj_value = json_value_init_object();
      JSON_Object *frame_obj = json_object(frame_obj_value);
      for (int id = 0; id < NUM_FRAME_BLOCKS; id++) {
        json_object_set_number(frame_obj, s_frame_block_id_str[id],
                               measurements_[frame][id]);
      }
      json_array_append_value(times_array, frame_obj_value);
    }
    char *json_str = json_serialize_to_string(times_array_value);
    json_value_free(times_array_value);

    // Upload the JSON
    EdgeAppLibDataExportFuture *future = DataExportSendData(
        (char *)PORTNAME_META, EdgeAppLibDataExportMetadata, json_str,
        strlen(json_str), 0);  // timestamp 0 tells the server this is the JSON
                               // with the frame times
    if (future) {
      DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
      DataExportCleanup(future);
    }

    json_free_serialized_string(json_str);

    // Reset collector
    memset(measurements_, 0, sizeof(measurements_));
    memset(timestamps_, 0, sizeof(timestamps_));
    frame_ = 0;
  }

 public:
  void EndFrame() {
    frame_++;

    if (frame_ >= NUM_FRAMES_PER_FLUSH) {
      Flush();
    }
  }

  void Collect(FrameBlockID id, double duration) {
    measurements_[frame_][id] = duration;
  }

  void Attach(FrameBlockID id, double value) {
    timestamps_[id] = value;
    if (id == 0)
      measurements_[frame_][id] = 0;
    else
      measurements_[frame_][id] =
          (timestamps_[id] - timestamps_[id - 1]) / (1000 * 1000);
  }
};

static FrameTimesCollector s_frame_times_collector;

/**
 * @brief RAII class used to measure the time taken to execute a code block.
 */
template <FrameBlockID ID>
class FrameScopedTimer {
  double start_time_;

 public:
  FrameScopedTimer() : start_time_(time_ms()) {}
  ~FrameScopedTimer() {
    double dur = time_ms() - start_time_;
    s_frame_times_collector.Collect(ID, dur);
  }
};

/**
 * @brief Measures the time taken to execute the current code block.
 */
#define FRAME_TIMER(id) \
  FrameScopedTimer<id> frame_scoped_timer__##id {}

#define FRAME_TIMER_APPEND(id, elapsed_time) \
  s_frame_times_collector.Attach(id, elapsed_time);

/**
 * @brief Sends the Input Tensor to the cloud asynchronously.
 *
 * This function sends the input tensor data from the provided frame to the
 * cloud. It returns a future object representing the asynchronous operation.
 *
 * By returning a future, this function allows for non-blocking execution.
 * The caller can await this future after sending the output tensor, ensuring
 * that both awaits are done consecutively without blocking the sending of the
 * rest of the data.
 *
 * @param frame Pointer to the current sensor frame.
 * @return A future representing the asynchronous operation of sending the input
 * tensor.
 */
static void sendInputTensor(EdgeAppLibSensorFrame *frame) {
  LOG_TRACE("Inside sendInputTensor.");

  struct EdgeAppLibSensorRawData data = {0};
  {
    FRAME_TIMER(FB_IT_GET_RAW_DATA);
    EdgeAppLibSensorChannel channel = 0;
    int32_t ret = -1;
    if ((ret = SensorFrameGetChannelFromChannelId(
             *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE,
             &channel)) != 0) {
      LOG_WARN(
          "SensorFrameGetChannelFromChannelId : ret=%d. Skipping "
          "sending "
          "input tensor.",
          ret);
      return;
    }

    if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
      LOG_WARN(
          "SensorChannelGetRawData : ret=%d. Skipping sending input "
          "tensor.",
          ret);
      return;
    }
  }

  {
    FRAME_TIMER(FB_IT_UPLOAD);
    EdgeAppLibDataExportFuture *future =
        DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
                           data.address, data.size, data.timestamp);
    if (future) {
      DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
      DataExportCleanup(future);
    }
  }
}

/**
 * @brief Sends the Metadata to the cloud synchronously.
 *
 * This function sends the post-processed output tensor (metadata) from the
 * provided sensor frame to the cloud.
 *
 * @param frame Pointer to the current sensor frame.
 */
static void sendMetadata(EdgeAppLibSensorFrame *frame) {
  LOG_TRACE("Inside sendMetadata.");

  struct EdgeAppLibSensorRawData data = {0};
  {
    FRAME_TIMER(FB_OT_GET_RAW_DATA);
    EdgeAppLibSensorChannel channel;
    int32_t ret = -1;
    if ((ret = SensorFrameGetChannelFromChannelId(
             *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel)) <
        0) {
      LOG_WARN(
          "SensorFrameGetChannelFromChannelId : ret=%d. Skipping "
          "sending "
          "metadata.",
          ret);
      return;
    }

    if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
      LOG_WARN(
          "SensorChannelGetRawData : ret=%d. Skipping sending "
          "metadata.",
          ret);
      return;
    }
  }

  LOG_INFO(
      "output_raw_data.address:%p\noutput_raw_data.size:%zu\noutput_raw_data."
      "timestamp:%llu\noutput_raw_data.type:%s",
      data.address, data.size, data.timestamp, data.type);

  {
    FRAME_TIMER(FB_OT_UPLOAD);
    EdgeAppLibSendDataResult send_data_res =
        SendDataSyncMeta(data.address, data.size, EdgeAppLibSendDataBase64,
                         data.timestamp, DATA_EXPORT_AWAIT_TIMEOUT);
    if (send_data_res != EdgeAppLibSendDataResultSuccess &&
        send_data_res != EdgeAppLibSendDataResultEnqueued) {
      LOG_ERR("SendDataSyncMeta failed with EdgeAppLibSendDataResult: %d",
              send_data_res);
    }
  }
}

int onCreate() {
  LOG_TRACE("Inside onCreate. Using a pseudo stream key.");
  int32_t ret = -1;
  if ((ret = SensorCoreInit(&s_core)) < 0) {
    LOG_ERR("SensorCoreInit : ret=%d", ret);
    return -1;
  }
  const char *stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
  if ((ret = SensorCoreOpenStream(s_core, stream_key, &s_stream)) < 0) {
    LOG_ERR("SensorCoreOpenStream : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  return 0;
}

int onConfigure(char *topic, void *value, int valuesize) {
  LOG_TRACE("Inside onConfigure.");
  if (value == NULL) {
    LOG_ERR("[onConfigure] Invalid param : value=NULL");
    return -1;
  }
  LOG_INFO("[onConfigure] topic:%s\nvalue:%s\nvaluesize:%i\n", topic,
           (char *)value, valuesize);

  char *output = NULL;
  DataProcessorResultCode res;
  if ((res = DataProcessorConfigure((char *)value, &output)) !=
      kDataProcessorOk) {
    DataExportSendState(topic, (void *)output, strlen(output));
    free(value);
    return (res == kDataProcessorInvalidParam) ? 0 : -1;
  }
  DataExportSendState(topic, value, valuesize);
  return 0;
}

int onIterate() {
  {
    FRAME_TIMER(FB_ON_ITERATE);

    bool inputTensorEnabled = DataExportIsEnabled(EdgeAppLibDataExportRaw);
    bool metadataEnabled = DataExportIsEnabled(EdgeAppLibDataExportMetadata);
    if (!inputTensorEnabled && !metadataEnabled) {
      // Early exit to avoid doing unnecessary work when DataExport is disabled
      return 0;
    }

    EdgeAppLibSensorFrame frame;
    int32_t ret = -1;
    {
      FRAME_TIMER(FB_GET_FRAME);
      if ((ret = SensorGetFrame(s_stream, &frame, SENSOR_GET_FRAME_TIMEOUT)) <
          0) {
        LOG_ERR("SensorGetFrame : ret=%d", ret);
        PrintSensorError();
        return SensorGetLastErrorCause() == AITRIOS_SENSOR_ERROR_TIMEOUT ? 0
                                                                         : -1;
      }

      uint64_t sequence_number = 0;
      EdgeAppLibLatencyTimestamps info = {0};
      if ((ret = SensorGetFrameLatency(frame, &sequence_number, &info)) == 0) {
        LOG_DBG("Frame sequence number: %llu", sequence_number);
        for (size_t i = 0; i < NUM_LATENCY_INSIDE_SENSOR_MAX; ++i) {
          if (info.points[i] != 0) {
            FRAME_TIMER_APPEND(
                static_cast<FrameBlockID>(FB_INSIDE_SENSOR_0 + i),
                info.points[i]);
            LOG_INFO("Latency %s: %llu", s_frame_block_id_str[i],
                     info.points[i]);
          }
        }
      }
    }

    if (inputTensorEnabled) {
      sendInputTensor(&frame);
    }
    if (metadataEnabled) {
      sendMetadata(&frame);
    }

    {
      FRAME_TIMER(FB_RELEASE_FRAME);
      if ((ret = SensorReleaseFrame(s_stream, frame)) < 0) {
        LOG_ERR("SensorReleaseFrame : ret= %d", ret);
        PrintSensorError();
        return -1;
      }
    }
  }

  s_frame_times_collector.EndFrame();
  return 0;
}

int onStop() {
  LOG_TRACE("Inside onStop.");
  int32_t ret = -1;
  if ((ret = SensorStop(s_stream)) < 0) {
    LOG_ERR("SensorStop : ret=%d", ret);
    PrintSensorError();
    return -1;
  }
  SensorLatencySetMode(false, NUM_LATENCY_INSIDE_SENSOR_MAX);
  return 0;
}

int onStart() {
  LOG_TRACE("Inside onStart.");
  SensorLatencySetMode(true, NUM_LATENCY_INSIDE_SENSOR_MAX);
  int32_t ret = -1;
  if ((ret = SensorStart(s_stream)) < 0) {
    LOG_ERR("SensorStart : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  return 0;
}

int onDestroy() {
  LOG_TRACE("Inside onDestroy.");
  int32_t ret = -1;
  if ((ret = SensorCoreCloseStream(s_core, s_stream)) < 0) {
    LOG_ERR("SensorCoreCloseStream : ret=%d", ret);
    PrintSensorError();
    return -1;
  }
  if ((ret = SensorCoreExit(s_core)) < 0) {
    LOG_ERR("SensorCoreExit : ret=%d", ret);
    PrintSensorError();
    return -1;
  }
  return 0;
}
