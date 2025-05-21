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

#include <optional>
#include <string>

namespace py = pybind11;

struct PyEdgeApp {
  std::string stream_key;
  py::object instance;
  py::object on_create;
  py::object on_configure;
  py::object on_iterate;
  py::object on_stop;
  py::object on_start;
  py::object on_destroy;

  void init(const py::type &edge_app_cls,
            const std::optional<py::str> &stream_key);
  void reset();
};

extern PyEdgeApp g_py_edge_app;
