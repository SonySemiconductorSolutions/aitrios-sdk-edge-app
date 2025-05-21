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
#include <pybind11/pytypes.h>

#include "bindings.hpp"
#include "exceptions.hpp"
#include "sensor.h"

namespace py = pybind11;

using namespace EdgeAppLib;

void bind_sensor_error(py::module_ &m) {
  py::enum_<EdgeAppLibSensorStatusParam>(m, "EdgeAppLibSensorStatusParam")
      .value("MESSAGE", AITRIOS_SENSOR_STATUS_PARAM_MESSAGE)
      .value("TRACE", AITRIOS_SENSOR_STATUS_PARAM_TRACE)
      .export_values();

  m.def(
      "get_last_error_string",
      [](EdgeAppLibSensorStatusParam param) -> std::string {
        char buffer[256];
        uint32_t length = sizeof(buffer);
        int32_t ret = SensorGetLastErrorString(param, buffer, &length);
        if (ret != 0) {
          throw PyEdgeAppError("SensorGetLastErrorString", ret);
        }
        return std::string(buffer, length);
      },
      "Get the last error string from the sensor", py::arg("param"));
}
