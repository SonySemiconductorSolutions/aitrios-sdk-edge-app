# Edge Application Sample for License Plate Recognition (lp_recog)

"**Edge Application**" sample for license plate recognition that outputs the recognition results of Japanese license plates.

This sample demonstrates a two-stage AI inference pipeline combining IMX500 and CPU-based AI models.

The following models have been verified:
- TensorFlow Lite

**Note**: This sample has been tested and verified to work on Raspberry Pi platform.

## Overview
- **Two-stage AI Pipeline**: First detects license plates using an AI model on the IMX500, then uses a subsequent TensorFlow Lite AI model running on CPU to recognize characters on the detected license plate images.
- **Hybrid Processing**: Leverages both IMX500 hardware acceleration for object detection and CPU-based tflite-runtime for character recognition.
- **Japanese License Plate Support**: Generates Japanese license plate notation by combining kanji, hiragana, numbers, and symbols (-, ・, .) from category IDs.

## Main Processing Flow
1. **License Plate Detection (IMX500)**
   - AI model running on IMX500 detects license plate regions in the input image.
   - Uses the `LPDDataProcessorAnalyze` function to process detection results.
   - Updates the Region of Interest (ROI) based on detected license plate coordinates.

2. **Character Recognition (CPU)**
   - Cropped license plate image is processed by a TensorFlow Lite model running on CPU.
   - Uses the `LPRDataProcessorAnalyze` function to recognize individual characters.
   - Converts float array predictions into character category IDs.

3. **License Plate String Generation**
   - Uses the `interpret_predictions` function to assemble a Japanese license plate string from character predictions.
   - Assigns kanji, hiragana, numbers, and symbols based on category IDs.
   - Automatically handles upper/lower line formatting and special characters (・, -).

4. **Output**
   - Results are formatted as strings and sent via metadata port.

## AI Model Deployment

### IMX500 Model (License Plate Detection)
- Deploy the license plate detection model to IMX500 using **AITRIOS Console** or **Local Console**.
- The model is referenced by `ai_model_bundle_id` (network_id) in the configuration.
- Supports wasi-nn runtime on IMX500 hardware.

### CPU Model (Character Recognition)
- The character recognition model must be **directly placed on the device**.
- Model file should be a TensorFlow Lite (`.tflite`) format.
- Default path: `/opt/senscord/share/tflite_models/LPR.tflite`
- The model path can be configured via `custom_settings.ai_models_cpu.lp_recognition.ai_model_path`.

## Main Data Structures
- **`Prediction`**: Detection result for one character (coordinates, score, category ID)
- **`Detections`/`DetectionData`**: List of detection results with bounding boxes for license plate detection
- **`LPContent`**: Elements of a license plate (kanji, hiragana, numbers, etc.)
- **`LPDataProcessorAnalyzeParam`**: Parameters for license plate detection analysis
- **`LPAnalysisParam`**: Analysis parameters containing ROI and tensor information

## Category ID Assignment
Character recognition model outputs category IDs mapped to Japanese license plate characters:
- **0,1**: Symbols (-, ・, .)
- **2-11**: Numbers (0-9)
- **12-128**: Kanji (place names like Yokohama, Nagoya, etc.)
- **129-170**: Hiragana (a, ka, sa, etc.)

## Configuration Parameters

The application is configured via DTDL interface defined in [**`edge_app_lp_recog_interface.json`**](./package/edge_app_lp_recog_interface.json). For a sample configuration, see [**`configuration.json`**](./configuration/configuration.json).

### Custom Settings Parameters

#### ai_models_imx500.lp_detection
Configuration for the license plate detection model running on IMX500:

- **`ai_model_bundle_id`**: Network ID of the deployed model
- **`parameters`**:
  - `max_detections` (int): Maximum number of detections to process (default: 200)
  - `threshold` (float): Detection confidence threshold (default: 0.3)
  - `input_width` (int): Model input image width (default: 300)
  - `input_height` (int): Model input image height (default: 300)
  - `bbox_normalization` (bool): Whether bounding box coordinates are normalized (0-1) (default: true)

#### ai_models_cpu.lp_recognition
Configuration for the character recognition model running on CPU:

- **`ai_model_path`**: Full path to the TensorFlow Lite model file on device (default: "/opt/senscord/share/tflite_models/LPR.tflite")
- **`parameters`**:
  - `threshold` (float): Character recognition confidence threshold (default: 0.5)

#### metadata_settings
- **`format`** (int): Output metadata format
  - `0`: Base64 encoded string
  - `1`: String

### Example Configuration
```json
{
  "custom_settings": {
    "ai_models_imx500": {
      "lp_detection": {
        "ai_model_bundle_id": "${network_id_1}",
        "parameters": {
          "max_detections": 200,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300,
          "bbox_normalization": true
        }
      }
    },
    "ai_models_cpu": {
      "lp_recognition": {
        "ai_model_path": "/opt/senscord/share/tflite_models/LPR.tflite",
        "parameters": {
          "threshold": 0.5
        }
      }
    },
    "metadata_settings": {
      "format": 1
    }
  }
}
```

## Main Data Structures

## Main Functions
- **`LPDDataProcessorAnalyze(float*, uint32_t, LPDataProcessorAnalyzeParam*)`**
  - Processes license plate detection results from IMX500
  - Updates ROI based on detected license plate coordinates
- **`LPRDataProcessorAnalyze(float*, uint32_t, char**, uint32_t*)`**
  - Processes character recognition results from CPU model
  - Generates Japanese license plate string from predictions
- **`CreateLPDetections(float*, uint32_t, DataProcessorCustomParam, Tensor*)`**
  - Converts float array from IMX500 into detection list
- **`interpret_predictions(const std::vector<Prediction>&)`**
  - Assembles license plate string from character predictions
- **`filter_predictions_by_score(std::vector<Prediction>&, float)`**
  - Filters predictions based on confidence threshold

## Implementation Notes
- **Character Layout**: The system automatically handles upper/lower line formatting for Japanese license plates.
- **Special Characters**: If a dot (・) is detected, a hyphen (-) will not be output in the same position.
- **Memory Management**: Ensure proper memory management for tensor data and recognition results.
- **Error Handling**: The application includes comprehensive error handling for both detection and recognition stages.
- **Performance**: IMX500 provides hardware-accelerated inference for detection, while CPU handles character recognition with tflite-runtime.

For detailed category ID assignments and character mappings, see `lp_recog_utils.cpp`.

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
