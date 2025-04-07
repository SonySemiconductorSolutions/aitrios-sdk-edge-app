# Edge Application Sample for Object Detection
"**Edge Application**" sample for Object Detection is post-processing of the AI model output to limit the output by setting the maximum number of detections and the threshold.

Supporting Object Detection models using SSD such as the following.
- Custom Vision (available model on "**Console for AITRIOS**")

## Initialize process
The Custom Parameters are used to configure the "**Edge Application**" and is passed by a configuration callback to the **`onConfigure(char *topic, void *value, int valuesize)`** event function as a json string inside the **`value`** parameter.

This function parses this **`value`** and sets the values of the variables corresponding to the post-processing parameters. Since Custom Parameters are in JSON format, it is parsed using [parson](../../libs/third_party/parson/).

The following parameters are used in the sample "**Edge Application**":

- **`max_detections`**<br>
  Description: Threshold of detections number. The maximum number of detected Bounding boxes you want to get after "**Edge Application**".<br>
>**NOTE**
>
>For the Custom Vision model, the value can be changed between 0 and 64.

- **`threshold`**<br>
  Description: Score threshold.<br>
>**NOTE**
>
>The value can be changed between 0 and 1. If an invalid value is entered, the default value is applied.

- **`input_width`**<br>
  Description: Width of AI model's input tensor.<br>
>**NOTE**
>
>The value of the parameter depends on the AI model.

- **`input_height`**<br>
  Description: Height of AI model's input tensor.<br>
>**NOTE**
>
>The value of the parameter depends on the AI model.

### Custom Parameters
Even if the Custom Parameters are not specified in the "**Console for AITRIOS**", it is possible to operate by setting an initial value in [the program](./data_processor/src/detection_utils.cpp). The sample sets the following values:

```json
{
    "max_detections" : 10,
    "threshold" : 0.3,
    "input_width" : 320,
    "input_height" : 320
}
```

The parameters used in this sample is defined in DTDL [**`edge_app_od_interface.json`**](./package/edge_app_od_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Analyze process
See [**`detection_data_processor.cpp`**](./data_processor/src/detection_data_processor.cpp) for the Analyze process.


**`DataProcessorAnalyze`** provides the function to parse the output of the AI model received as a char pointer  **`*in_data`** (the type casted output of the IMX500, excluding the format dependent on IMX500), sort it, and output the data of the number of **`max_predictions`** specified in Custom Parameters, in the FlatBuffers format.


The sample uses the following FlatBuffers schema  [**`objectdetection.fbs`**](./schemas/objectdetection.fbs):

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
}

table ObjectDetectionData {
  object_detection_list:[GeneralObject];
}

table ObjectDetectionTop {
  perception:ObjectDetectionData;
}

root_type ObjectDetectionTop;
```

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
