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

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <vector>

#include "data_export.h"
#include "draw.h"
#include "log.h"
#include "nn.h"
#include "send_data.h"
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
  const std::vector<float> *mean_values;
  const std::vector<float> *norm_values;
};

#define MAX_GRAPH_CONTEXTS 8
#define MAX_OUTPUT_TENSORS_SIZE 512 * 1024  // 500KB for output tensors
#define MAX_OUTPUT_TENSOR_NUM 4

enum TensorMemoryOwner {
  Unknown,
  Sensor,  // memory ownership is with Sensor/host
  App      // memory ownership is with the application
};

// Structure to hold temporary tensor information
// This is used to manage input tensors
struct TempTensorInfo {
  uint8_t *buffer = nullptr;
  size_t size = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint64_t timestamp = 0;  ///< Timestamp for the tensor
  TensorMemoryOwner memory_owner =
      TensorMemoryOwner::Unknown;  ///< Memory ownership of the tensor
};

typedef struct {
  EdgeAppLibSensorCore *sensor_core;     /**< Sensor core. */
  EdgeAppLibSensorStream *sensor_stream; /**< Sensor stream. */
  EdgeAppLibGraphContext *graph_ctx; /**< Multiple graph execution contexts. */
  EdgeAppCoreTarget target;          /**< Target for each graph context. */
  TempTensorInfo temp_input;
  uint32_t model_idx; /**< Count of loaded models. */
  const std::vector<float> *mean_values;
  const std::vector<float> *norm_values;
} EdgeAppCoreCtx;

namespace EdgeAppCore {

enum TensorDataType : uint8_t {
  TensorTypeFloat16 = 0,
  TensorTypeFloat32 = 1,
  TensorTypeUInt8 = 2,
  TensorTypeInt32 = 3,
  TensorTypeInt64 = 4
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
  EdgeAppLibDrawFormat format = AITRIOS_DRAW_FORMAT_UNDEFINED;
  TensorMemoryOwner memory_owner = TensorMemoryOwner::Unknown;

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

// Preprocess callback function type
// Parameters: input_data, input_property, output_data, output_property
// Returns: EdgeAppCoreResult (success/failure)
typedef EdgeAppCoreResult (*PreprocessCallback)(
    const void *input_data, EdgeAppLibImageProperty input_property,
    void **output_data, EdgeAppLibImageProperty *output_property);

// Preprocess callback function type
// Parameters: input_data, input_property, tensor
// Returns: EdgeAppCoreResult (success/failure)
typedef EdgeAppCoreResult (*PreprocessCallbackTensor)(
    const void *input_data, EdgeAppLibImageProperty input_property,
    Tensor *output_tensor);
/**
 * @brief Automatically releases the frame when it goes out of scope.
 */
class ProcessedFrame {
 public:
  ProcessedFrame(EdgeAppLibSensorStream *s, EdgeAppLibSensorFrame f)
      : ctx_(nullptr),
        shared_ctx_(nullptr),
        stream_(s),
        frame_(f),
        roi_(nullptr),
        preprocess_callback_(nullptr),
        preprocess_tensor_callback_(nullptr),
        is_computed_(true),
        preprocessed_data_(nullptr),
        preprocessed_memory_owner_(TensorMemoryOwner::Unknown),
        owns_frame_(true) {}

  // Constructor for method chaining
  ProcessedFrame(EdgeAppCoreCtx *ctx, EdgeAppCoreCtx *shared_ctx,
                 EdgeAppLibSensorFrame f)
      : ctx_(ctx),
        shared_ctx_(shared_ctx),
        stream_(shared_ctx ? shared_ctx->sensor_stream : nullptr),
        frame_(f),
        roi_(nullptr),
        preprocess_callback_(nullptr),
        preprocess_tensor_callback_(nullptr),
        is_computed_(false),
        preprocessed_data_(nullptr),
        preprocessed_memory_owner_(TensorMemoryOwner::Unknown),
        owns_frame_(false) {}

  // Constructor for empty/failed ProcessedFrame
  ProcessedFrame()
      : ctx_(nullptr),
        shared_ctx_(nullptr),
        stream_(nullptr),
        frame_(0),
        roi_(nullptr),
        preprocess_callback_(nullptr),
        preprocess_tensor_callback_(nullptr),
        is_computed_(false),
        preprocessed_data_(nullptr),
        preprocessed_memory_owner_(TensorMemoryOwner::Unknown),
        owns_frame_(false) {}

  // Destructor ensures frame is released only if we own it
  ~ProcessedFrame() {
    if (owns_frame_ && stream_ && frame_ != 0) {
      if (EdgeAppLib::SensorReleaseFrame(*stream_, frame_) < 0) {
        LOG_ERR("SensorReleaseFrame failed in ProcessedFrame destructor.");
      }
      frame_ = 0;  // Reset frame to avoid double release
    }
    // Note: preprocessed data memory management is handled by GetInput or
    // UnloadModel
  }

  // Move constructor, transfer ownership of the frame from source to this.
  ProcessedFrame(ProcessedFrame &&source) noexcept
      : ctx_(source.ctx_),
        shared_ctx_(source.shared_ctx_),
        stream_(source.stream_),
        frame_(source.frame_),
        roi_(source.roi_),
        preprocess_callback_(source.preprocess_callback_),
        preprocess_tensor_callback_(source.preprocess_tensor_callback_),
        is_computed_(source.is_computed_),
        preprocessed_data_(source.preprocessed_data_),
        preprocessed_memory_owner_(source.preprocessed_memory_owner_),
        owns_frame_(source.owns_frame_) {
    source.ctx_ = nullptr;
    source.shared_ctx_ = nullptr;
    source.stream_ = nullptr;
    source.frame_ = 0;
    source.roi_ = nullptr;
    source.preprocess_callback_ = nullptr;
    source.preprocess_tensor_callback_ = nullptr;
    source.is_computed_ = false;
    source.preprocessed_data_ = nullptr;
    source.preprocessed_memory_owner_ = TensorMemoryOwner::Unknown;
    source.owns_frame_ = false;
  }

  // Prevent copying frames to avoid double free
  ProcessedFrame(const ProcessedFrame &) = delete;
  ProcessedFrame &operator=(const ProcessedFrame &) = delete;

  // Define move assignment operator
  ProcessedFrame &operator=(ProcessedFrame &&source) noexcept {
    if (this != &source) {
      // Clean up current preprocessed data if we own it
      if (preprocessed_data_ &&
          preprocessed_memory_owner_ == TensorMemoryOwner::App) {
        free(preprocessed_data_);
      }

      // Move all members from source
      ctx_ = source.ctx_;
      shared_ctx_ = source.shared_ctx_;
      stream_ = source.stream_;
      frame_ = source.frame_;
      roi_ = source.roi_;
      preprocess_callback_ = source.preprocess_callback_;
      preprocess_tensor_callback_ = source.preprocess_tensor_callback_;
      is_computed_ = source.is_computed_;
      preprocessed_data_ = source.preprocessed_data_;
      preprocessed_memory_owner_ = source.preprocessed_memory_owner_;
      owns_frame_ = source.owns_frame_;

      // Clear source
      source.ctx_ = nullptr;
      source.shared_ctx_ = nullptr;
      source.stream_ = nullptr;
      source.frame_ = 0;
      source.roi_ = nullptr;
      source.preprocess_callback_ = nullptr;
      source.preprocess_tensor_callback_ = nullptr;
      source.is_computed_ = false;
      source.preprocessed_data_ = nullptr;
      source.preprocessed_memory_owner_ = TensorMemoryOwner::Unknown;
      source.owns_frame_ = false;
    }
    return *this;
  }

  // Implicit cast to EdgeAppLibSensorFrame for passing next Process call
  operator EdgeAppLibSensorFrame() const {
    if (!is_computed_) {
      return 0;  // Return invalid frame number
    }
    return frame_;
  }

  // Methods for chaining:
  // withROI(roi).withPreprocessing(callback).compute()
  ProcessedFrame &withROI(const EdgeAppLibSensorImageCropProperty &roi) {
    roi_ = &roi;
    return *this;
  }

  // Templated preprocessing setter: accepts either callback type
  template <typename Callback>
  ProcessedFrame &withPreprocessing(Callback callback) {
    if constexpr (std::is_same_v<Callback, PreprocessCallback>) {
      preprocess_callback_ = callback;
      preprocess_tensor_callback_ = nullptr;
    } else if constexpr (std::is_same_v<Callback, PreprocessCallbackTensor>) {
      preprocess_tensor_callback_ = callback;
      preprocess_callback_ = nullptr;
    } else {
      static_assert(!sizeof(Callback),
                    "Unsupported preprocessing callback type");
    }
    return *this;
  }

  ProcessedFrame compute();

  bool empty() const {
    if (!is_computed_) {
      return true;
    }
    return frame_ == 0 || stream_ == nullptr;
  }

 private:
  void ProcessInternal(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame,
                       EdgeAppLibSensorImageCropProperty &roi);
  EdgeAppCoreCtx *ctx_ = nullptr;
  EdgeAppCoreCtx *shared_ctx_ = nullptr;
  EdgeAppLibSensorStream *stream_ = nullptr;
  EdgeAppLibSensorFrame frame_ = 0;
  const EdgeAppLibSensorImageCropProperty *roi_ = nullptr;

  // Support both callback types
  PreprocessCallback preprocess_callback_ = nullptr;
  PreprocessCallbackTensor preprocess_tensor_callback_ = nullptr;
  bool is_computed_ = false;
  void *preprocessed_data_ = nullptr;
  TensorMemoryOwner preprocessed_memory_owner_ = TensorMemoryOwner::Unknown;
  bool owns_frame_ = false;  // If true, destructor will release the frame
};

// C++ only APIs
EdgeAppCoreResult LoadModel(EdgeAppCoreModelInfo models, EdgeAppCoreCtx &ctx,
                            EdgeAppCoreCtx *shared_ctx);
ProcessedFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame);
ProcessedFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame,
                       EdgeAppLibSensorImageCropProperty &roi);
Tensor GetOutput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                 uint32_t max_tensor_num = 1);
std::vector<Tensor> GetOutputs(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                               uint32_t max_tensor_num);
Tensor GetInput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame);
EdgeAppCoreResult UnloadModel(EdgeAppCoreCtx &ctx);
EdgeAppCoreResult SendInputTensor(Tensor *input_tensor);
EdgeAppCoreResult SendInference(void *data, size_t datalen,
                                EdgeAppLibSendDataType datatype,
                                uint64_t timestamp);

}  // namespace EdgeAppCore
#endif  // __cplusplus

#endif  // EDGEAPP_CORE_H_
