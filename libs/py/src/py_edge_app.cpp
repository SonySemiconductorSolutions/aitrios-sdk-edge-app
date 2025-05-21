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

#include "py_edge_app.hpp"

#include "log.h"
#include "sensor.h"

PyEdgeApp g_py_edge_app;

void PyEdgeApp::init(const py::type &edge_app_cls,
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

void PyEdgeApp::reset() {
  on_create = py::none();
  on_configure = py::none();
  on_iterate = py::none();
  on_stop = py::none();
  on_start = py::none();
  on_destroy = py::none();
  instance = py::none();
}
