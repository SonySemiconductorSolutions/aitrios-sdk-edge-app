# EdgeAppCore API Walk-through

## Overview

The `EdgeAppCore` API provides essential functions for managing AI models, processing sensor frames, and exporting data.
It bridges the gap between sensor data acquisition and data export in edge AI applications, offering robust handling of sensor streams, inference processing, and data transmission.

This API leverages `EdgeAppLib` to seamlessly operate both Sensor interfaces and Wasi-nn for AI inference, enabling unified management of hardware resources and software processing.

The API is organized as follows:

- **[Setup Models and sensors](#api-for-model-management)**: This section offers functions to load and unload AI models, manage resources, and configure execution targets.
- **[Inference Processing](#api-for-frame-processing)**: This section includes frame acquisition, ROI-based cropping, and inference processing. It uses automatic resource handling to ensure safe and efficient usage.
- **[Input/Output Tensor Access](#api-for-inputoutput-tensor-access)**: This section provides functions to access the input and output tensors for further analysis or post-processing.


These APIs streamline sensor data management, from acquisition to export, making them suitable for applications involving IoT, edge processing, and data analytics.

---


###  API List
| Function                       | Description                                                                 |
| ------------------------------ | --------------------------------------------------------------------------- |
| `EdgeAppCore::LoadModel`       | Loads the AI model and sets up the corresponding context.                   |
| `EdgeAppCore::Process`         | Processes a sensor frame (optionally with ROI cropping) and runs inference. |
| `EdgeAppCore::GetOutput`       | Retrieves the output tensor from the processed frame or inference graph.    |
| `EdgeAppCore::GetOutputByIndex` | Retrieves a specific output tensor by index from the processed frame.       |
| `EdgeAppCore::GetOutputs`      | Retrieves all output tensors as a vector from the processed frame.          |
| `EdgeAppCore::GetInput`        | Retrieves the input tensor from the frame or temporary buffer.              |
| `EdgeAppCore::UnloadModel`     | Unloads the loaded model and cleans up resources.                           |



###  Usage Example

```cpp
#include "edgeapp_core.h"
#include "log.h"  // LOG_ERR, LOG_WARN, etc.

using namespace EdgeAppCore;

int main() {
  // Step 1: Initialize model information

  const std::vector<float> mean_values = {0.485f, 0.456f, 0.406f};
  const std::vector<float> norm_values = {0.229f, 0.224f, 0.225f};

  EdgeAppCoreModelInfo model_info[2] = {
    {"000000", EdgeAppCoreTarget::edge_imx500, {}, {}},
    {"lp_recognition", EdgeAppCoreTarget::edge_cpu, &mean_values, &norm_values}
  };

  // Step 2: Initialize contexts
  EdgeAppCoreCtx ctx_imx500 = {};
  EdgeAppCoreCtx ctx_cpu = {};
  EdgeAppCoreCtx shared_ctx = {};

  // Step 3: Load the model for imx500
  if (LoadModel(model_info[0], ctx_imx500, &shared_ctx) != EdgeAppCoreResultSuccess) {
    LOG_ERR("Failed to load model.");
    return -1;
  }
  // Step 4: Load the model for cpu
  if (LoadModel(model_info[1], ctx_cpu, &shared_ctx) != EdgeAppCoreResultSuccess) {
    LOG_ERR("Failed to load model.");
    return -1;
  }

  // Step 5: Process the frame with ROI
  EdgeAppLibSensorImageCropProperty roi = {0, 0, 320, 240};
  auto frame = Process(ctx_cpu, &shared_ctx, 0, roi);

  // Step 6: Get the 4 output tensors
  Tensor output_tensor = GetOutput(ctx_cpu, frame, 4);
  if (output_tensor.data != nullptr) {
    LOG_ERR("Output tensor size: %zu", output_tensor.size);
  } else {
    LOG_WARN("Output tensor not available.");
  }

  // Step 8: Do some post processing

  // Step 9: Get the input tensor for debug (option)
  Tensor input_tensor = GetInput(ctx_cpu, frame);
  if (input_tensor.data != nullptr) {
    LOG_ERR("Input tensor size: %zu", input_tensor.size);
  } else {
    LOG_WARN("Input tensor not available.");
  }

  // Step 10: Send the input tensor to the cloud for retrain (option)
  if (SendInputTensor(&input_tensor) != EdgeAppCoreResultSuccess) {
    LOG_ERR("Failed to send input tensor.");
  }

  // Step 11: Unload the model
  if (UnloadModel(ctx_cpu) != EdgeAppCoreResultSuccess) {
    LOG_ERR("Failed to unload model.");
  }
  if (UnloadModel(ctx_imx500) != EdgeAppCoreResultSuccess) {
    LOG_ERR("Failed to unload model.");
  }
  return 0;
}

```

## Additional Notes

* The API is designed for **C++** usage, with RAII-style resource handling (`ProcessedFrame`).
* The `ProcessedFrame` ensures that resources are freed even in the case of exceptions or early returns.
* `ProcessedFrame` supports fluent API pattern for method chaining with `withROI()`, `withPreprocessing()`, and `compute()` methods.
* The `ProcessedFrame` can be implicitly converted to `EdgeAppLibSensorFrame` for use with `GetOutput` and `GetInput` functions.

## API Functions

### EdgeAppCore::Process

Processes a sensor frame with optional ROI cropping and preprocessing callback. Returns a `ProcessedFrame` object that supports fluent API for chaining operations.

**Signatures:**
```cpp
// Basic processing
ProcessedFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame);

// With ROI cropping
ProcessedFrame Process(EdgeAppCoreCtx &ctx, EdgeAppCoreCtx *shared_ctx,
                       EdgeAppLibSensorFrame frame,
                       EdgeAppLibSensorImageCropProperty &roi);
```

**Parameters:**
- `ctx`: EdgeApp context for the target model
- `shared_ctx`: Shared context (can be nullptr for first model)
- `frame`: Input sensor frame (0 for new frame from sensor)
- `roi`: Region of Interest for cropping

**ProcessedFrame Fluent API:**
The `ProcessedFrame` class supports method chaining for configuration before processing:

```cpp
// Method chaining API
ProcessedFrame frame = Process(ctx, &shared_ctx, 0)
    .withROI(roi)
    .withPreprocessing(callback)
    .compute();
```

**Preprocessing Callback:**
The callback function signature:
```cpp
typedef EdgeAppCoreResult (*PreprocessCallback)(
    const void *input_data, EdgeAppLibImageProperty input_property,
    void **output_data, EdgeAppLibImageProperty *output_property);
```

**Usage Examples:**
```cpp
// Basic processing
auto frame = Process(ctx, &shared_ctx, 0);

// With ROI cropping
EdgeAppLibSensorImageCropProperty roi = {0, 0, 320, 240};
auto frame = Process(ctx, &shared_ctx, 0, roi);

// With fluent API for ROI and preprocessing
EdgeAppCoreResult enhance_image(const void *input_data,
                               EdgeAppLibImageProperty input_property,
                               void **output_data,
                               EdgeAppLibImageProperty *output_property) {
    // Apply image enhancement and return processed data
    // output_data should be allocated by the callback
    // Return EdgeAppCoreResultSuccess on success
    return EdgeAppCoreResultSuccess;
}

auto frame = Process(ctx, &shared_ctx, 0)
    .withROI(roi)
    .withPreprocessing(enhance_image)
    .compute();

// Check if processing was successful
if (!frame.empty()) {
    // Use frame for GetOutput/GetInput operations
}
```

### EdgeAppCore::GetOutput

Retrieves output tensor(s) from the processed frame or inference graph.

**Signature:**
```cpp
Tensor GetOutput(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                 uint32_t max_tensor_num = MAX_OUTPUT_TENSOR_NUM);
```

### EdgeAppCore::GetOutputByIndex

Retrieves a specific output tensor by index from the processed frame.

**Signature:**
```cpp
Tensor GetOutputByIndex(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                        int32_t tensor_index);
```

**Parameters for GetOutput:**
- `ctx`: EdgeApp context
- `frame`: Sensor frame from Process function
- `max_tensor_num`: Maximum number of tensors to process (default: MAX_OUTPUT_TENSOR_NUM)

**Parameters for GetOutputByIndex:**
- `ctx`: EdgeApp context
- `frame`: Sensor frame from Process function
- `tensor_index`: Tensor index to retrieve
  - `>= 0`: Get specific tensor by index (0, 1, 2, ...)
  - `-1`: Get all tensors as flattened tensor

**Returns:**
- `Tensor`: Single tensor containing the requested data

### EdgeAppCore::GetOutputs

Retrieves all output tensors as a vector from the processed frame.

**Signature:**
```cpp
std::vector<Tensor> GetOutputs(EdgeAppCoreCtx &ctx, EdgeAppLibSensorFrame frame,
                               uint32_t max_tensor_num = MAX_OUTPUT_TENSOR_NUM);
```

**Parameters:**
- `ctx`: EdgeApp context  
- `frame`: Sensor frame from Process function
- `max_tensor_num`: Maximum number of tensors to retrieve (optional)

**Returns:**
- `std::vector<Tensor>`: Vector containing individual tensors

**Usage Examples:**
```cpp
// Get all tensors as vector
auto outputs = GetOutputs(ctx, frame, 4);
auto& scores = outputs[0];
auto& coords = outputs[1];

// Get specific tensor by index
Tensor first = GetOutputByIndex(ctx, frame, 0);   // First tensor
Tensor second = GetOutputByIndex(ctx, frame, 1);  // Second tensor

// Get all tensors flattened
Tensor all = GetOutputByIndex(ctx, frame, -1);    // All tensors combined

// Get output tensor with default settings
Tensor output = GetOutput(ctx, frame);  // Uses default max_tensor_num
```

## Summary

The `EdgeAppCore` API consolidates model management, sensor data processing, and data export.
It ensures safe and efficient operation in edge AI applications, offering flexibility for integration in various scenarios, from simple image processing to complex multi-model inference pipelines.

