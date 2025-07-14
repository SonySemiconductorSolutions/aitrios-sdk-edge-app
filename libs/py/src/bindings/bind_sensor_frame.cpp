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

#include "bindings.hpp"
#include "exceptions.hpp"
#include "log.h"
#include "py_sensor_types.hpp"
#include "sensor.h"

namespace py = pybind11;

using namespace EdgeAppLib;

void bind_sensor_frame(py::module_ &m) {
  py::class_<PySensorFrame>(m, "SensorFrame")
      .def("get_inputs",
           [](PySensorFrame frame)
               -> std::tuple<py::array_t<uint8_t>, uint64_t> {
             EdgeAppLibSensorChannel channel = 0;
             int channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE;
             int32_t ret = SensorFrameGetChannelFromChannelId(
                 frame.handle, channel_id, &channel);
             if (ret != 0) {
               throw PyEdgeAppError("SensorFrameGetChannelFromChannelId", ret);
             }

             EdgeAppLibSensorRawData raw_data{};
             ret = SensorChannelGetRawData(channel, &raw_data);
             if (ret != 0) {
               throw PyEdgeAppError("SensorChannelGetRawData", ret);
             }

             EdgeAppLibSensorImageProperty image_property{};
             ret = SensorChannelGetProperty(
                 channel, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY, &image_property,
                 sizeof(image_property));
             if (ret != 0) {
               throw PyEdgeAppError("SensorStreamGetProperty", ret);
             }

             std::vector<ssize_t> shape{image_property.height,
                                        image_property.width, 3};
             return std::make_tuple(
                 py::array_t<uint8_t>(shape,
                                      static_cast<uint8_t *>(raw_data.address)),
                 raw_data.timestamp);
           })
      .def("get_outputs",
           [](PySensorFrame frame) -> py::typing::List<py::array_t<float>> {
             EdgeAppLibSensorChannel channel = 0;
             int channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT;
             int32_t ret = SensorFrameGetChannelFromChannelId(
                 frame.handle, channel_id, &channel);
             if (ret != 0) {
               throw PyEdgeAppError("SensorFrameGetChannelFromChannelId", ret);
             }

             EdgeAppLibSensorRawData raw_data{};
             ret = SensorChannelGetRawData(channel, &raw_data);
             if (ret != 0) {
               throw PyEdgeAppError("SensorChannelGetRawData", ret);
             }

             EdgeAppLibSensorTensorShapesProperty tensor_shape{};
             ret = SensorChannelGetProperty(
                 channel, AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY,
                 &tensor_shape, sizeof(tensor_shape));
             if (ret != 0) {
               throw PyEdgeAppError("SensorChannelGetProperty", ret);
             }

             std::ostringstream ss;
             for (uint32_t i = 0; i < AITRIOS_SENSOR_SHAPES_ARRAY_LENGTH; ++i) {
               if (tensor_shape.shapes_array[i] == 0) break;
               ss << tensor_shape.shapes_array[i] << " ";
             }
             LOG_DBG("get_outputs - tensor_count: %d, tensor shapes: %s",
                     tensor_shape.tensor_count, ss.str().c_str());

             std::vector<std::vector<uint32_t>> shapes;
             uint32_t index = 0;
             while (index < AITRIOS_SENSOR_SHAPES_ARRAY_LENGTH) {
               uint32_t dimension = tensor_shape.shapes_array[index++];
               if (dimension == 0) break;

               shapes.emplace_back();
               for (uint32_t j = 0; j < dimension; ++j) {
                 shapes.back().push_back(tensor_shape.shapes_array[index++]);
               }
             }

             ss.clear();
             ss.str("");
             for (const auto &shape : shapes) {
               ss << "[ ";
               for (const auto &dim : shape) {
                 ss << dim << " ";
               }
               ss << "]";
             }
             LOG_DBG("get_outputs - tensor shape: %s", ss.str().c_str());

             py::list outputs;
             float *data_ptr = static_cast<float *>(raw_data.address);
             uint32_t offset = 0;
             for (const auto &shape : shapes) {
               size_t num_elements = std::accumulate(
                   shape.begin(), shape.end(), static_cast<size_t>(1),
                   std::multiplies<uint32_t>());
               auto output_array = py::array_t<float>(shape, data_ptr + offset);
               outputs.append(output_array);
               offset += num_elements;
             }

             return outputs;
           })
      .def("get_channel",
           [](PySensorFrame frame, int channel_id) -> PySensorChannel {
             EdgeAppLibSensorChannel channel = 0;
             int32_t ret = SensorFrameGetChannelFromChannelId(
                 frame.handle, channel_id, &channel);
             if (ret != 0) {
               throw PyEdgeAppError("SensorFrameGetChannelFromChannelId", ret);
             }

             return PySensorChannel{channel};
           })
      .def_readonly("handle", &PySensorFrame::handle)
      .def("__repr__", [](PySensorFrame frame) {
        return "<edge_app_sdk.SensorFrame handle=" +
               std::to_string(frame.handle) + ">";
      });
}
