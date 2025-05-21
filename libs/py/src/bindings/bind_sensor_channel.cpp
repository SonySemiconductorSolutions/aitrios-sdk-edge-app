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

#include "bindings.hpp"
#include "exceptions.hpp"
#include "log.h"
#include "py_sensor_types.hpp"
#include "sensor.h"

namespace py = pybind11;

using namespace EdgeAppLib;

void bind_sensor_channel(py::module_ &m) {
  py::class_<PySensorChannel>(m, "SensorChannel")
      .def_property_readonly(
          "raw_data",
          [](PySensorChannel channel) -> py::memoryview {
            EdgeAppLibSensorRawData data{};
            int32_t ret = SensorChannelGetRawData(channel.handle, &data);
            if (ret != 0) {
              throw PyEdgeAppError("SensorChannelGetRawData", ret);
            }

            return py::memoryview::from_memory(data.address, data.size);
          },
          "Access the channel data as a raw memory view.")

      .def_property_readonly(
          "data",
          [](PySensorChannel channel)
              -> py::array_t<float, py::array::c_style> {
            EdgeAppLibSensorRawData data{};
            int32_t ret = SensorChannelGetRawData(channel.handle, &data);
            if (ret != 0) {
              throw PyEdgeAppError("SensorChannelGetRawData", ret);
            }

            // TODO: this asssumes the raw data is always an array of floats,
            // which is not always true We would want to change the type and
            // shape based on the channel format, e.g. uint8 array with shape
            // (width, height, 3) for an RGB image channel like the input tensor
            return py::array_t<float, py::array::c_style>(
                static_cast<size_t>(data.size) / sizeof(float),
                reinterpret_cast<float *>(data.address));
          },
          "Access the channel data as a typed numpy array.")

      .def_readonly("handle", &PySensorChannel::handle)
      .def("__repr__", [](PySensorChannel channel) {
        return "<edge_app_sdk.SensorChannel handle=" +
               std::to_string(channel.handle) + ">";
      });
}
