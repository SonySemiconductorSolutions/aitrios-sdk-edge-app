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

#include "bindings.hpp"
#include "exceptions.hpp"
#include "log.h"
#include "py_sensor_types.hpp"
#include "sensor.h"

namespace py = pybind11;

using namespace EdgeAppLib;

py::class_<struct PySensorStream> bind_sensor_stream(py::module_ &m) {
  auto cls_stream = py::class_<struct PySensorStream>(m, "SensorStream");

  cls_stream
      .def(
          "get_frame",
          [](PySensorStream stream, int32_t timeout_msec) -> PySensorFrame {
            EdgeAppLibSensorFrame frame;
            int32_t ret = SensorGetFrame(stream.handle, &frame, timeout_msec);
            if (ret != 0) {
              throw PyEdgeAppError("SensorGetFrame", ret);
            }

            return PySensorFrame{frame};
          },
          R"(
        Get sensor frame. Returns the oldest unobtained handle of frame.

        :param timeout_msec: timeout value (0: timeout immediately, -1: not timeout infinitely)
        :raises EdgeAppError:
        )",
          py::arg("timeout_msec").none(false))

      .def(
          "release_frame",
          [](PySensorStream stream, PySensorFrame frame) {
            int32_t ret = SensorReleaseFrame(stream.handle, frame.handle);
            if (ret != 0) {
              throw PyEdgeAppError("SensorReleaseFrame", ret);
            }
          },
          R"(
        Release sensor frame.

        :param frame: Handle of frame to be released.
        :raises EdgeAppError:
        )",
          py::arg("frame").none(false))
      .def_readonly("handle", &PySensorStream::handle)
      .def("__repr__", [](PySensorStream stream) {
        return "<edge_app_sdk.SensorStream handle=" +
               std::to_string(stream.handle) + ">";
      });

  return cls_stream;
}
