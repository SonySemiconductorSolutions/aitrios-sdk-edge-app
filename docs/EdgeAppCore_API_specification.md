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
| `EdgeAppCore::GetInput`        | Retrieves the input tensor from the frame or temporary buffer.              |
| `EdgeAppCore::UnloadModel`     | Unloads the loaded model and cleans up resources.                           |



###  Usage Example

```cpp
#include "edgeapp_core.h"
#include "log.h"  // LOG_ERR, LOG_WARN, etc.

using namespace EdgeAppCore;

int main() {
  // Step 1: Initialize model information
EdgeAppCoreModelInfo model_info[4] = {
    {"000000", EdgeAppCoreTarget::edge_imx500},
    {"/home/pi/yolo11n.onnx", EdgeAppCoreTarget::edge_cpu},
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
  Tensor output_tensor = GetOutput(ctx_cpu, auto_frame, 4);
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

* The API is designed for **C++** usage, with RAII-style resource handling (`AutoFrame`).
* The `AutoFrame` ensures that resources are freed even in the case of exceptions or early returns.

## Summary

The `EdgeAppCore` API consolidates model management, sensor data processing, and data export.
It ensures safe and efficient operation in edge AI applications, offering flexibility for integration in various scenarios, from simple image processing to complex multi-model inference pipelines.

