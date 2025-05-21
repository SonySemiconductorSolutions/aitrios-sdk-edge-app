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

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>
#include <string>

#include "bindings.hpp"
#include "exceptions.hpp"
#include "log.h"
#include "py_sensor_types.hpp"
#include "sensor.h"

namespace py = pybind11;

using namespace EdgeAppLib;

int run_sm(py::type edge_app_class, std::optional<py::str> stream_key);

PYBIND11_MODULE(_edge_app_sdk, m) {
  bind_sensor_channel(m);
  bind_sensor_frame(m);
  auto cls_stream = bind_sensor_stream(m);
  bind_stream_properties(m, cls_stream);
  bind_enums(m);
  bind_data_export(m);
  bind_sensor_error(m);

  py::register_exception<PyEdgeAppError>(m, "EdgeAppError");

  m.def(
       "stream",
       []() {
         extern EdgeAppLibSensorStream s_stream;  // from py_sm.cpp
         return PySensorStream{s_stream};
       },
       "Gets the current sensor stream.")

      .def("run_sm", &run_sm,
           R"(
           Run the Edge App state machine. Blocks until the edge app is destroyed.

           :param edge_app_class: Class that implements the edge app event functions.
           :param stream_key: Key of the stream to open in the edge app.
           :return: Edge app exit code.
           )",
           py::arg("edge_app_class").none(false),
           py::arg("stream_key").none(true) = py::none());
}
