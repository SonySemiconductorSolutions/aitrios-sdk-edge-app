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

template <typename TProperty>
static py::class_<TProperty> define_stream_property(
    py::handle scope, py::class_<PySensorStream> cls_stream,
    const char *property_cls_name, const char *stream_property_name,
    const char *property_key) {
  auto cls_property = py::class_<TProperty>(scope, property_cls_name);

  cls_property.def(py::init<>());

  cls_stream.def_property(
      stream_property_name,
      [property_key](PySensorStream stream) -> TProperty {
        TProperty value{};
        int32_t ret = SensorStreamGetProperty(stream.handle, property_key,
                                              &value, sizeof(value));
        if (ret != 0) {
          throw PyEdgeAppError("SensorStreamGetProperty", ret);
        }
        return value;
      },
      [property_key](PySensorStream stream, const TProperty &value) {
        int32_t ret = SensorStreamSetProperty(stream.handle, property_key,
                                              &value, sizeof(value));
        if (ret != 0) {
          throw PyEdgeAppError("SensorStreamGetProperty", ret);
        }
      });

  return cls_property;
}

void bind_stream_properties(py::module_ &m,
                            py::class_<PySensorStream> &cls_stream) {
  m.attr("AI_MODEL_BUNDLE_ID_SIZE") = AI_MODEL_BUNDLE_ID_SIZE;

  define_stream_property<EdgeAppLibSensorImageCropProperty>(
      m, cls_stream, "SensorImageCrop", "image_crop",
      AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY)
      .def_readwrite("left", &EdgeAppLibSensorImageCropProperty::left)
      .def_readwrite("top", &EdgeAppLibSensorImageCropProperty::top)
      .def_readwrite("width", &EdgeAppLibSensorImageCropProperty::width)
      .def_readwrite("height", &EdgeAppLibSensorImageCropProperty::height)
      .def("__repr__", [](const EdgeAppLibSensorImageCropProperty &p) {
        return "<edge_app_sdk.SensorImageCrop left=" + std::to_string(p.left) +
               ", top=" + std::to_string(p.top) +
               ", width=" + std::to_string(p.width) +
               ", height=" + std::to_string(p.height) + ">";
      });

  define_stream_property<EdgeAppLibSensorAiModelBundleIdProperty>(
      m, cls_stream, "SensorAiModelBundleId", "ai_model_bundle_id",
      AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY)
      .def_property(
          "id",
          [](const EdgeAppLibSensorAiModelBundleIdProperty &p) {
            return py::str(p.ai_model_bundle_id);
          },
          [](EdgeAppLibSensorAiModelBundleIdProperty &p, py::str value) {
            auto tmp_str = value.cast<std::string>();
            if (tmp_str.size() >= AI_MODEL_BUNDLE_ID_SIZE) {
              throw std::invalid_argument(
                  "string is too long for ai_model_bundle_id");
            }
            strncpy(p.ai_model_bundle_id, tmp_str.data(),
                    sizeof(p.ai_model_bundle_id) - 1);
            p.ai_model_bundle_id[tmp_str.size()] = '\0';
          })
      .def("__repr__", [](const EdgeAppLibSensorAiModelBundleIdProperty &p) {
        return "<edge_app_sdk.SensorAiModelBundleId id='" +
               std::string(p.ai_model_bundle_id) + "'>";
      });
}
