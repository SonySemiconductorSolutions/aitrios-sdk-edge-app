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

#include <pybind11/pybind11.h>
#include <stdlib.h>

#include <string>

#include "data_export.h"
#include "data_processor_api.hpp"
#include "log.h"
#include "log_internal.h"
#include "send_data.h"
#include "sensor.h"
#include "sm.h"
#include "sm_utils.hpp"

#define PORTNAME_META "metadata"
#define PORTNAME_INPUT "input"

#define DATA_EXPORT_AWAIT_TIMEOUT 10000

EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;
char *state_topic = NULL;

namespace py = pybind11;

using namespace EdgeAppLib;

static struct {
  std::string stream_key;
  py::object instance;
  py::object on_create;
  py::object on_configure;
  py::object on_iterate;
  py::object on_stop;
  py::object on_start;
  py::object on_destroy;

  void init(const py::type &edge_app_cls,
            const std::optional<py::str> &stream_key) {
    if (stream_key.has_value() && stream_key.value()) {
      this->stream_key = stream_key->cast<std::string>();
    } else {
      this->stream_key = AITRIOS_SENSOR_STREAM_KEY_DEFAULT;
    }
    LOG_INFO("Using stream key '%s'", this->stream_key.c_str());

    // TODO: find some better way to replace the hardcoded stream key in
    // applying.cpp
    extern std::string s_py_stream_key;  // from states/applying.cpp
    s_py_stream_key = this->stream_key;

    instance = edge_app_cls();

#define LOOKUP_CALLBACK(name)         \
  if (py::hasattr(instance, #name)) { \
    name = instance.attr(#name);      \
  } else {                            \
    name = py::none();                \
  }

    LOOKUP_CALLBACK(on_create)
    LOOKUP_CALLBACK(on_configure)
    LOOKUP_CALLBACK(on_iterate)
    LOOKUP_CALLBACK(on_stop)
    LOOKUP_CALLBACK(on_start)
    LOOKUP_CALLBACK(on_destroy)

#undef LOOKUP_CALLBACK
  }

  void reset() {
    on_create = py::none();
    on_configure = py::none();
    on_iterate = py::none();
    on_stop = py::none();
    on_start = py::none();
    on_destroy = py::none();
    instance = py::none();
  }
} g_py_edge_app;

/**
 * @brief Sends the Input Tensor to the cloud synchronously.
 *
 * This function sends the input tensor data from the provided frame to the
 * cloud.
 *
 * @param frame Pointer to the current sensor frame.
 */
void sendInputTensorSync(EdgeAppLibSensorFrame *frame) {
  LOG_TRACE("Inside sendInputTensor.");

  EdgeAppLibSensorChannel channel = 0;
  int32_t ret = -1;
  if ((ret = SensorFrameGetChannelFromChannelId(
           *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE,
           &channel)) != 0) {
    LOG_WARN(
        "EdgeAppLibSensorFrameGetChannelFromChannelId : ret=%d. Skipping "
        "sending "
        "input tensor.",
        ret);
    return;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "EdgeAppLibSensorChannelGetRawData : ret=%d. Skipping sending input "
        "tensor.",
        ret);
    return;
  }

  /*
   * Handle subframe properties for large input tensors:
   * - When the input tensor is large, it is divided into subframes.
   * - Retrieve subframe properties using GetProperty.
   */
  struct EdgeAppLibSensorSubFrameProperty subframe = {0};
  if ((ret = SensorChannelGetProperty(
           channel, AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY, &subframe,
           sizeof(struct EdgeAppLibSensorSubFrameProperty))) < 0) {
    LOG_WARN("SensorChannelGetProperty - SubFrame: ret=%d", ret);
  } else {
    LOG_INFO("SensorChannelGetProperty - SubFrame: current=%d, division=%d",
             subframe.current_num, subframe.division_num);
    if (subframe.current_num == 0 && subframe.division_num == 0) {
      /*
       * If both current and division are 0, the data is invalid:
       * - No timestamp is associated.
       * - Data size is 0 bytes.
       */
      return;
    } else {
      /*
       * Includes current_num and division_num for processing valid subframes.
       */
      // EdgeAppLibDataExportFuture *future =
      //     DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
      //                        data.address, data.size, data.timestamp,
      //                        subframe.current_num, subframe.division_num);
      // DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
      // DataExportCleanup(future);
      return;
    }
  }

  // TODO: got segmentation fault here when running from RaspberryPi
  // EdgeAppLibDataExportFuture *future =
  //     DataExportSendData((char *)PORTNAME_INPUT, EdgeAppLibDataExportRaw,
  //                        data.address, data.size, data.timestamp);
  // DataExportAwait(future, DATA_EXPORT_AWAIT_TIMEOUT);
  // DataExportCleanup(future);

  // TODO: since we are using mock EVP, EVP_blobOperation doesn't send the data
  // anywhere, so write it to a file here
  struct stat st;
  if (stat("./images", &st) == -1) {
    mkdir("./images", 0700);
  }
  char fn[512] = {0};
  snprintf(fn, sizeof(fn), "./images/%zu.jpg", (size_t)data.timestamp);
  FILE *f = fopen(fn, "wb");
  fwrite(data.address, data.size, 1, f);
  fclose(f);
}

/**
 * @brief Sends the Metadata to the cloud synchronously.
 *
 * This function sends the post-processed output tensor (metadata) from the
 * provided sensor frame to the cloud.
 *
 * @param frame Pointer to the current sensor frame.
 */
void sendMetadata(EdgeAppLibSensorFrame *frame) {
  LOG_TRACE("Inside sendMetadata.");

  // TODO: when using a webcam stream, this channel ID actually maps to the
  // image channel
  //       so sendMetadata is actually sending the image
  EdgeAppLibSensorChannel channel;
  int32_t ret = -1;
  if ((ret = SensorFrameGetChannelFromChannelId(
           *frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel)) < 0) {
    LOG_WARN(
        "EdgeAppLibSensorFrameGetChannelFromChannelId : ret=%d. Skipping "
        "sending "
        "metadata.",
        ret);
    return;
  }

  struct EdgeAppLibSensorRawData data = {0};
  if ((ret = SensorChannelGetRawData(channel, &data)) < 0) {
    LOG_WARN(
        "EdgeAppLibSensorChannelGetRawData : ret=%d. Skipping sending "
        "metadata.",
        ret);
    return;
  }

  LOG_INFO(
      "output_raw_data.address:%p\noutput_raw_data.size:%zu\noutput_raw_data."
      "timestamp:%llu\noutput_raw_data.type:%s",
      data.address, data.size, data.timestamp, data.type);

  /*
   * Retrieve subframe properties:
   * - Subframe properties indicate whether the input tensor is divided into
   *   smaller parts.
   * - Each part, including metadata, is processed based on its subframe number.
   */
  struct EdgeAppLibSensorSubFrameProperty subframe = {0};
  if ((ret = SensorChannelGetProperty(
           channel, AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY, &subframe,
           sizeof(struct EdgeAppLibSensorSubFrameProperty))) < 0) {
    LOG_WARN("SensorChannelGetProperty - SubFrame: ret=%d", ret);
  } else {
    LOG_INFO("SensorChannelGetProperty - SubFrame: current=%d, division=%d",
             subframe.current_num, subframe.division_num);
    /*
     * - Only if current_num is 1, the data includes valid metadata.
     */
    if (subframe.current_num != 1) return;

    /*
     * Proceed with processing metadata:
     * - Metadata is not divided, so no need to append current_num
     *   and division_num.
     */
  }

  EdgeAppLibSendDataResult send_data_res =
      SendDataSyncMeta(data.address, data.size, EdgeAppLibSendDataBase64,
                       data.timestamp, DATA_EXPORT_AWAIT_TIMEOUT);
  if (send_data_res != EdgeAppLibSendDataResultSuccess) {
    LOG_ERR("SendDataSyncMeta failed with EdgeAppLibSendDataResult: %d",
            send_data_res);
  }

  // TODO: since we are using mock EVP, EVP_blobOperation doesn't send the data
  // anywhere, so write it to a file here
  struct stat st;
  if (stat("./metadata", &st) == -1) {
    mkdir("./metadata", 0700);
  }
  char fn[512] = {0};
  snprintf(fn, sizeof(fn), "./metadata/%zu.bin", (size_t)data.timestamp);
  FILE *f = fopen(fn, "wb");
  fwrite(data.address, data.size, 1, f);
  fclose(f);
}

int onCreate() {
  LOG_TRACE("Inside onCreate.");
  int32_t ret = -1;
  if ((ret = SensorCoreInit(&s_core)) < 0) {
    LOG_ERR("SensorCoreInit : ret=%d", ret);
    return -1;
  }

  if ((ret = SensorCoreOpenStream(s_core, g_py_edge_app.stream_key.c_str(),
                                  &s_stream)) < 0) {
    LOG_ERR("SensorCoreOpenStream : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  py::gil_scoped_acquire acquire;
  if (g_py_edge_app.on_create) {
    return g_py_edge_app.on_create().cast<int>();
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
  state_topic = topic;
  if ((res = DataProcessorConfigure((char *)value, &output)) !=
      kDataProcessorOk) {
    DataExportSendState(topic, (void *)output, strlen(output));
    free(value);
    return (res == kDataProcessorInvalidParam) ? 0 : -1;
  }
  DataExportSendState(topic, value, valuesize);

  py::gil_scoped_acquire acquire;
  if (g_py_edge_app.on_configure) {
    return g_py_edge_app.on_configure().cast<int>();
  }

  return 0;
}

int onIterate() {
  LOG_TRACE("Inside onIterate.");

  py::gil_scoped_acquire acquire;
  if (g_py_edge_app.on_iterate) {
    return g_py_edge_app.on_iterate().cast<int>();
  }

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

  py::gil_scoped_acquire acquire;
  if (g_py_edge_app.on_stop) {
    return g_py_edge_app.on_stop().cast<int>();
  }

  return 0;
}

int onStart() {
  LOG_TRACE("Inside onStart.");
  int32_t ret = -1;
  if ((ret = SensorStart(s_stream)) < 0) {
    LOG_ERR("SensorStart : ret=%d", ret);
    PrintSensorError();
    return -1;
  }

  py::gil_scoped_acquire acquire;
  if (g_py_edge_app.on_start) {
    return g_py_edge_app.on_start().cast<int>();
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

  py::gil_scoped_acquire acquire;
  if (g_py_edge_app.on_destroy) {
    return g_py_edge_app.on_destroy().cast<int>();
  }

  return 0;
}

int run_sm(py::type edge_app_class, std::optional<py::str> stream_key) {
  SetLogLevel(kTraceLevel);

  {
    auto edge_app_class_str = py::repr(edge_app_class).cast<std::string>();
    LOG_INFO("Running state machine with Python class '%s'",
             edge_app_class_str.c_str());
  }

  g_py_edge_app.init(edge_app_class, stream_key);

  py::gil_scoped_release release(true);
  int main(int argc, char *argv[]);  // from libs/sm/src/main.cpp
  int result = main(0, nullptr);

  py::gil_scoped_acquire acquire;
  g_py_edge_app.reset();

  return result;
}
