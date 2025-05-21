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

#include <stdlib.h>

#include <string>

#include "log.h"
#include "log_internal.h"
#include "py_edge_app.hpp"
#include "sm.h"
#include "sm_utils.hpp"

EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;
char *state_topic = NULL;

using namespace EdgeAppLib;

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

  py::gil_scoped_acquire acquire;
  if (g_py_edge_app.on_configure) {
    std::string config_json(static_cast<char *>(value), valuesize);
    std::string topic_str(topic);
    return g_py_edge_app.on_configure(topic_str, config_json).cast<int>();
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
