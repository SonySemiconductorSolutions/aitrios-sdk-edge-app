# "**Edge Application**" Sample for Switching DNN
"**Edge Application**" sample for switching DNN is a post-processing app that dynamically switches between two models : Object Detection and Image Classification.

In this sample, you can use two models for post-processing, such as using an object detection model to detect birds and then using an image classification model to classify bird types.

The following Object Detection and Image Classification models are supported :
- SSD MobileNet V1
- MobileNet V2

## Initialize process
The Custom Parameters are used to configure the "**Edge Application**" and is passed by a configuration callback to the **`onConfigure(char *topic, void *value, int valuesize)`** event functions as a json string inside a **`value`** parameter.

This function parses this value and sets the values of the variables corresponding to the post-processing parameters. Since Custom Parameters are in JSON format, they are parsed using [parson](../../libs/third_party/).

The following parameters are used in the sample "**Edge Application**" :

- **`ai_models`**<br>
  - **`detection_bird`**<br>
    > **NOTE**
    >
    > An identifier for validation of linking Wasm implementation and Custom Parameters.
    - **`ai_model_bundle_id`**<br>
    Description: **`NetworkId`** for Object Detection model.
    > **NOTE**
    >
    > Set to 6 digits.
    > The **`NetworkId`** can be obtained from the "**Console UI**".
    > See ["**Console User Manual**"](https://developer.aitrios.sony-semicon.com/en/edge-ai-sensing/documents/console-v2/console-user-manual/) for details.
    - **`param`**<br>
      - **`max_detections`**<br>
        Description: Threshold of detections number. The maximum number of detected Bounding boxes you want to get after "**Edge Application**".
        > **NOTE**
        >
        > For SSD MobileNet V1, the value can be changed between 0 and 10.
      - **`threshold`**<br>
        Description: Score threshold.
        > **NOTE**
        >
        > The value can be changed between 0 and 1.
      - **`input_width`**<br>
        Description: Width of AI model's input tensor.
        > **NOTE**
        >
        > The value of the parameter depends on the AI model. The minimum value is 1.
      - **`input_height`**<br>
        Description: Height of AI model's input tensor.
        > **NOTE**
        >
        > The value of the parameter depends on the AI model. The minimum value is 1.
  - **`classification_bird`**<br>
    > **NOTE**
    >
    > An identifier for validation of linking Wasm implementation and Custom Parameters.
    - **`ai_model_bundle_id`**<br>
        Description: **`NetworkId`** for Image Classification model.
        > **NOTE**
        >
        > Set to 6 digits.
        > The **`NetworkId`** can be obtained from the "**Console UI**".
        > See ["**Console User Manual**"](https://developer.aitrios.sony-semicon.com/en/edge-ai-sensing/documents/console-v2/console-user-manual/) for details.
    - **`param`**<br>
      - **`max_predictions`**<br>
        Description: Threshold of predictions number. The maximum number of predictions you want to get after "**Edge Application**".
        > **NOTE**
        >
        > For MobileNet V2, it can be changed from 0 to 1001.

If the Custom Parameters are not specified in the "**Console for AITRIOS**", the sample application does not operate correctly.

### Custom Parameters

The following Custom Parameters are an example for this sample application.

```json
{
    "ai_models": {
        "detection_bird": {
            "ai_model_bundle_id": "000001",
            "param": {
                "max_detections": 5,
                "threshold": 0.0,
                "input_width": 300,
                "input_height": 300
            }
        },
        "classification_bird": {
            "ai_model_bundle_id": "000002",
            "param": {
                "max_predictions": 5
            }
        }
    }
}
```

The parameters used in this sample is defined in DTDL [**`edge_app_switch_dnn_interface.json`**](./package/edge_app_switch_dnn_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Analyze process
See [**`switch_dnn_analyzer.cpp`**](./data_processor/src/switch_dnn_analyzer.cpp) for the Analyze process.

### Object Detection

**`AnalyzerOd::Analyze`** provides the function to parse the output of the AI model received as a float pointer **`*p_data`** (the output of the IMX500, excluding the format dependent on IMX500), sort it, and output the data of the number of **`max_detections`** specified in Custom Parameters, in the FlatBuffers format.

The sample uses the following FlatBuffers schema [**`switch_dnn_objectdetection.fbs`**](./schemas/switch_dnn_objectdetection.fbs) :

```
namespace SmartCamera;

table BoundingBox2d {
  left:int;
  top:int;
  right:int;
  bottom:int;
}

union BoundingBox {
  BoundingBox2d,
}

table GeneralObject {
  class_id:uint;
  bounding_box:BoundingBox;
  score:float;
  is_used_for_cropping:bool;
}

table ObjectDetectionData {
  object_detection_list:[GeneralObject];
}

table ObjectDetectionTop {
  perception:ObjectDetectionData;
  trace_id:ulong;
}

root_type ObjectDetectionTop;
```

By using SSD MobileNet V1 pre-trained with COCO dataset, the detected objects may include birds and the other objects. So in the analyze process, the detected objects are filtered by class id of bird (**`15`**).

### Image Classification
**`AnalyzerIc::Analyze`** provides the function to parse the output of the AI model received as a float pointer **`*p_data`** (the output of the IMX500, excluding the format dependent on IMX500), sort it, and output the data of the number of **`max_predictions`** specified in Custom Parameters, in the FlatBuffers format.

The sample uses the following FlatBuffers schema [**`switch_dnn_classification.fbs`**](./schemas/switch_dnn_classification.fbs) :


```
namespace SmartCamera;

table GeneralClassification {
  class_id:uint;
  score:float;
}

table ClassificationData {
  classification_list:[GeneralClassification];
}

table ClassificationTop {
  perception:ClassificationData;
  trace_id:ulong;
}

root_type ClassificationTop;
```

## Switch DNN

The sample instructs Edge AI Device to set Crop based on the Bbox detected by the Object Detection and switch the model to Image Classification after the analysis processing of Object Detection.

The inference results of Object Detection and Image Classification are linked by **`trace_id`**.
The Image Classification inference result, which corresponds to the Object Detection inference result, has the same **`trace_id`** value as the Object Detection inference result.

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
