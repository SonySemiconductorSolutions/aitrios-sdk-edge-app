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

#include "../py_data_export.hpp"
#include "bindings.hpp"
#include "exceptions.hpp"
#include "log.h"
#include "py_sensor_types.hpp"
#include "sensor.h"

namespace py = pybind11;

using namespace EdgeAppLib;

void bind_data_export(py::module_ &m) {
  m.def(
       "stream",
       []() {
         extern EdgeAppLibSensorStream s_stream;  // from py_sm.cpp
         return PySensorStream{s_stream};
       },
       "Gets the current sensor stream.")

      .def(
          "send_metadata",
          [](PySensorFrame frame) {
            void sendMetadata(EdgeAppLibSensorFrame * frame);  // from py_sm.cpp

            sendMetadata(&frame.handle);
          },
          R"(
          Sends the Metadata to the cloud synchronously.

          This function sends the post-processed output tensor (metadata) from the
          provided sensor frame to the cloud.
          )",
          py::arg("frame").none(false))

      .def(
          "send_input_tensor",
          [](PySensorFrame frame) {
            void sendInputTensorSync(EdgeAppLibSensorFrame *
                                     frame);  // from py_sm.cpp

            sendInputTensorSync(&frame.handle);
          },
          R"(
          Sends the Input Tensor to the cloud synchronously.

          This function sends the input tensor data from the provided frame to the
          cloud.
          )",
          py::arg("frame").none(false));

  m.def("get_port_settings", &getPortSettingsStr,
        "Get port settings as JSON string");

  m.def(
      "send_state",
      [](const std::string &topic, const std::string &state)
          -> EdgeAppLibDataExportResult { return sendState(topic, state); },
      "Send state to the cloud synchronously", py::arg("topic"),
      py::arg("state"));

  m.def(
      "get_workspace_directory",
      []() -> std::string { return getWorkspaceDirectory(); },
      "Get the workspace directory");

  m.def(
      "format_timestamp",
      [](uint64_t timestamp) -> std::string {
        return formatTimestamp(timestamp);
      },
      "Format timestamp into string representation", py::arg("timestamp"));

  m.def(
      "get_file_suffix",
      [](EdgeAppLibDataExportDataType dataType) -> std::string {
        return getFileSuffix(dataType);
      },
      "Get file suffix for data export", py::arg("dataType").none(false));

  m.def(
      "send_file",
      [](EdgeAppLibDataExportDataType dataType, const std::string &filePath,
         const std::string &url, int timeout_ms) -> EdgeAppLibDataExportResult {
        py::gil_scoped_release release;
        return sendFile(dataType, filePath, url, timeout_ms);
      },
      "Send file to the cloud synchronously", py::arg("dataType"),
      py::arg("filePath"), py::arg("url"), py::arg("timeout_ms"));

  m.def(
      "send_telemetry",
      [](py::bytes data, int timeout_ms) -> EdgeAppLibDataExportResult {
        char *buffer;
        ssize_t length;
        if (PyBytes_AsStringAndSize(data.ptr(), &buffer, &length) < 0) {
          throw py::error_already_set();
        }
        py::gil_scoped_release release;
        return sendTelemetry(buffer, length, timeout_ms);
      },
      "Send telemetry to the cloud synchronously", py::arg("data"),
      py::arg("timeout_ms"));
}
