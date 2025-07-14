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

#ifndef EDGEAPP_CORE_H_
#define EDGEAPP_CORE_H_

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#include "data_export.h"
#include "log.h"
#include "nn.h"
#include "sensor.h"

#define MAX_TENSOR_DIMS 4

typedef enum {
  EdgeAppCoreResultSuccess = 0,      /**< Operation succeeded. */
  EdgeAppCoreResultFailure = 1,      /**< Operation failed. */
  EdgeAppCoreResultTimeout = 2,      /**< Operation timed out. */
  EdgeAppCoreResultInvalidParam = 3, /**< Invalid parameter. */
  EdgeAppCoreResultDataTooLarge = 4, /**< Data size exceeds limits. */
  EdgeAppCoreResultDenied = 5        /**< Operation denied. */
} EdgeAppCoreResult;

typedef enum { edge_cpu, edge_gpu, edge_npu, edge_imx500 } EdgeAppCoreTarget;

struct EdgeAppCoreModelInfo {
  const char *model_name;    ///< Name of the model
  EdgeAppCoreTarget target;  ///< Target for the tensor
};

#define MAX_GRAPH_CONTEXTS 8
#define MAX_OUTPUT_TENSORS_SIZE 2822400 * 2  // 4 MB for output tensors
#define MAX_OUTPUT_TENSOR_NUM 4

// Structure to hold temporary tensor information
// This is used to manage input tensors
struct TempTensorInfo {
  uint8_t *buffer = nullptr;
  size_t size = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint64_t timestamp = 0;  ///< Timestamp for the tensor
};

typedef struct {
  EdgeAppLibSensorCore *sensor_core;     /**< Sensor core. */
  EdgeAppLibSensorStream *sensor_stream; /**< Sensor stream. */
  graph_execution_context *graph_ctx; /**< Multiple graph execution contexts. */
  EdgeAppCoreTarget target;           /**< Target for each graph context. */
  TempTensorInfo temp_input;
  uint32_t model_idx; /**< Count of loaded models. */
} EdgeAppCoreCtx;

inline execution_target ToExecutionTarget(EdgeAppCoreTarget target) {
  switch (target) {
    case edge_cpu:
      return cpu;
    case edge_gpu:
      return gpu;
    default:
      return cpu;
  }
}
#ifdef __cplusplus
}
#endif

// ------------------------------ C++ only ------------------------------
#ifdef __cplusplus

#include <memory>
#include <type_traits>
#include <vector>

namespace EdgeAppCore {

enum TensorDataType : uint8_t {
  TensorTypeFloat32 = 0,
  TensorTypeUInt8 = 1,
  TensorTypeInt32 = 2
  // Extend as needed
};

struct TensorShapeInfo {
  uint32_t ndim;  ///< Number of dimensions (e.g., 2 for [10, 4])
  uint32_t dims[MAX_TENSOR_DIMS];  ///< Shape dimensions

  size_t NumElements() const {
    size_t count = 1;
    for (uint8_t i = 0; i < ndim; ++i) {
      count *= dims[i];
    }
    return count;
  }
};

struct Tensor {
  void *data;                  ///< Pointer to raw tensor data
  TensorShapeInfo shape_info;  ///< Shape information
  TensorDataType type;         ///< Type of the tensor (float32, uint8, etc.)
  size_t size;
  uint64_t timestamp;
  char name[64] = {0};  ///< Optional name for the tensor

  template <typename T>
  T *DataAs() {
    if constexpr (std::is_same<T, float>::value) {
      return (type == TensorTypeFloat32) ? static_cast<T *>(data) : nullptr;
    } else if constexpr (std::is_same<T, uint8_t>::value) {
      return (type == TensorTypeUInt8) ? static_cast<T *>(data) : nullptr;
    } else if constexpr (std::is_same<T, int32_t>::value) {
      return (type == TensorTypeInt32) ? static_cast<T *>(data) : nullptr;
    } else {
      return nullptr;  // Unsupported type
    }
  }

  template <typename T>
  const T *DataAs() const {
    return const_cast<Tensor *>(this)->DataAs<T>();
  }
};

/**
 * @brief Automatically releases the frame when it goes out of scope.
 */
class AutoFrame {
 public:
  AutoFrame(EdgeAppLibSensorStream *s, EdgeAppLibSensorFrame f)
      : stream_(s), frame_(f) {}

  // Destructor ensures frame is released
  ~AutoFrame() {
    if (stream_ && frame_) {
      LOG_WARN("Releasing frame: %llu", frame_);
      if (EdgeAppLib::SensorReleaseFrame(*stream_, frame_) < 0) {
        LOG_ERR("SensorReleaseFrame failed.");
      }
      frame_ = 0;  // Reset frame to avoid double release
    }
  }

  // Prevent copying
  AutoFrame(const AutoFrame &) = delete;
  AutoFrame &operator=(const AutoFrame &) = delete;

  // Allow move semantics
  AutoFrame(AutoFrame &&other) noexcept
      : stream_(other.stream_), frame_(other.frame_) {
    other.stream_ = nullptr;
    other.frame_ = 0;
  }

  AutoFrame &operator=(AutoFrame &&other) noexcept {
    if (this != &other) {
      stream_ = other.stream_;
      frame_ = other.frame_;
      other.stream_ = nullptr;
      other.frame_ = 0;
    }
    return *this;
  }

  // Implicit cast to EdgeAppLibSensorFrame
  operator EdgeAppLibSensorFrame() const { return frame_; }

 private:
  EdgeAppLibSensorStream *stream_ = nullptr;
  EdgeAppLibSensorFrame frame_ = 0;
};

// C++ only APIs
EdgeAppCoreResult LoadModel(EdgeAppCoreModelInfo models, EdgeAppCoreCtx &ctx,
                            EdgeAppCoreCtx *shared_ctx);
AutoFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                  EdgeAppLibSensorFrame frame,
                  EdgeAppLibSensorImageCropProperty &roi);
Tensor GetOutput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                 uint32_t max_tensor_num = MAX_OUTPUT_TENSOR_NUM);
Tensor GetInput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame);
EdgeAppCoreResult UnloadModel(EdgeAppCoreCtx &ctx);
EdgeAppCoreResult SendInputTensor(Tensor *input_tensor);

}  // namespace EdgeAppCore
#endif  // __cplusplus

#endif  // EDGEAPP_CORE_H_
