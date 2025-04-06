# Edge Application Sample for Pose Detection
"**Edge Application**" sample for Pose Detection is post-processing of the AI model output to show human skeleton by setting the maximum number of detections and the threshold.

Supporting Pose Detection models using SSD such as the following.
- posenet mobilenet v1

## Initialize process
The Custom Parameters are used to configure the "**Edge Application**" and is passed by a configuration callback to the **`onConfigure(char *topic, void *value, int valuesize)`** event function as a json string inside the **`value`** parameter.

This function parses this **`value`** and sets the values of the variables corresponding to the post-processing parameters. Since Custom Parameters are in JSON format, it is parsed using [parson](../../libs/third_party/parson/).

The following parameters are used in the sample "**Edge Application**":

- **`score_threshold`**<br>
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

- **`output_width`**<br>
  Description: Width of AI model's output tensor.<br>
>**NOTE**
>
>The value of the parameter depends on the AI model.

- **`output_height`**<br>
  Description: Height of AI model's output tensor.<br>
>**NOTE**
>
>The value of the parameter depends on the AI model.

- **`iou_threshold`**<br>
  Description: Intersection over Union Score threshold.<br>
>**NOTE**
>
>The value can be changed between 0 and 1. When the value of IoU approaches 1, it indicates that the predicted box almost completely overlaps with the real box, indicating a high degree of matching; When the value of IoU approaches 0, it indicates that there is almost no overlap between the two.

- **`nms_radius`**<br>
  Description: Non Maximum Suppression Radius.<br>
>**NOTE**
>
>When performing Non Maximum Suppression (NMS), define the size of a local region to ensure that only one feature point is preserved within that region.

- **`max_pose_detections`**<br>
  Description: Maximum Number of Pose Detections.<br>
>**NOTE**
>
>The maximum number to detect pose.

- **`heatmap_index`**<br>
  Description: Index of Heatmap.<br>
>**NOTE**
>
>Predict the offset of each key point, usually including the offset in the x and y directions.

- **`offset_index`**<br>
  Description: Offset Index.<br>
>**NOTE**
>
>Offset Index

- **`forward_displacement_index`**<br>
  Description: Forward Index.<br>
>**NOTE**
>
>Refer to a moving average indicator in trading, a method for analyzing the pose of a moving platform, or a way to estimate displacement risk.

- **`backward_displacement_index`**<br>
  Description: Backward Index.<br>
>**NOTE**
>
>Refer to the movement of an object in a negative direction, or a policy that promotes equal opportunities for underrepresented groups.

### Custom Parameters
Even if the Custom Parameters are not specified in the "**Console for AITRIOS**", it is possible to operate by setting an initial value in [the program](./data_processor/src/posenet_utils.cpp). The sample sets the following values:

```json
{
    "input_width" : 449,
    "input_height" : 449,
    "output_width" : 29,
    "output_height" : 29,
    "score_threshold" : 0.5,
    "iou_threshold" : 0.28,
    "nms_radius" : 20,
    "max_pose_detections" : 15,
    "heatmap_index" : 2,
    "offset_index" : 3,
    "forward_displacement_index" : 1,
    "backward_displacement_index" : 0
}

```

The parameters used in this sample is defined in DTDL [**`edge_app_pn_interface.json`**](./package/edge_app_pn_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Analyze process
See [**`posenet_data_processor.cpp`**](./data_processor/src/posenet_data_processor.cpp) for the Analyze process.


**`DataProcessorAnalyze`** provides the function to parse the output of the AI model received as a char pointer  **`*in_data`** (the type casted output of the IMX500, excluding the format dependent on IMX500), sort it, and output the data of the number of **`max_predictions`** specified in Custom Parameters, in the FlatBuffers format.


The sample uses the following FlatBuffers schema  [**`poseestimation.fbs`**](./schemas/poseestimation.fbs):

```
namespace SmartCamera;

enum KeyPointName:byte { Nose = 0,
                         LeftEye,
                         RightEye,
                         LeftEar,
                         RightEar,
                         LeftShoulder,
                         RightShoulder,
                         LeftElbow,
                         RightElbow,
                         LeftWrist,
                         RightWrist,
                         LeftHip,
                         RightHip,
                         LeftKnee,
                         RightKnee,
                         LeftAnkle,
                         RightAnkle = 16,
                       }

table Point2d {
  x:int;
  y:int;
}

union Point {
  Point2d,
}

table KeyPoint {
  score:float;
  point:Point;
  name:KeyPointName;
}

table GeneralPose {
  score:float;
  keypoint_list:[KeyPoint];
}

table PoseEstimationData {
  pose_list:[GeneralPose];
}

table PoseEstimationTop {
  perception:PoseEstimationData;
}

root_type PoseEstimationTop;
```

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
