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

#include "log.h"
#include "sensor.h"

namespace py = pybind11;

using namespace EdgeAppLib;

int run_sm(py::type edge_app_class, std::optional<py::str> stream_key);

class PyEdgeAppError : public std::exception {
 public:
  PyEdgeAppError(const char *func, int32_t code)
      : msg_(std::string(func) + " : ret=" + std::to_string(code)) {}

  const char *what() const noexcept override { return msg_.c_str(); }

 private:
  std::string msg_;
};

struct PySensorStream {
  EdgeAppLibSensorStream handle;
};

struct PySensorFrame {
  EdgeAppLibSensorFrame handle;
};

struct PySensorChannel {
  EdgeAppLibSensorChannel handle;
};

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

PYBIND11_MODULE(_edge_app_sdk, m) {
  auto cls_stream = py::class_<PySensorStream>(m, "SensorStream");
  auto cls_frame = py::class_<PySensorFrame>(m, "SensorFrame");
  auto cls_channel = py::class_<PySensorChannel>(m, "SensorChannel");

  py::register_exception<PyEdgeAppError>(m, "EdgeAppError");

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

  cls_frame
      .def("get_inputs",
           [](PySensorFrame frame) -> py::array_t<uint8_t> {
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
             return py::array_t<uint8_t>(
                 shape, static_cast<uint8_t *>(raw_data.address));
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

  cls_channel
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
          py::arg("frame").none(false))

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
