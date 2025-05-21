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

#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

void bind_sensor_channel(py::module_ &m);

void bind_sensor_frame(py::module_ &m);

py::class_<struct PySensorStream> bind_sensor_stream(py::module_ &m);

void bind_stream_properties(py::module_ &m,
                            py::class_<struct PySensorStream> &cls_stream);

void bind_data_export(py::module_ &m);

void bind_enums(py::module_ &m);

void bind_sensor_error(py::module_ &m);
