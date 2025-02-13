# Edge Application Sample for Semantic Segmentation

>**NOTE**
>
>We do not release the AI model corresponding to the "**Edge Application**" sample for Semantic Segmentation, so the sample cannot be run on Edge AI Devices.

"**Edge Application**" sample for Semantic Segmentation is post-processing of the AI model output to set the class id for each pixel in the image.

The tables below show the output data format for semantic segmentation models supported by the sample. In the provided example, the input tensor width and height values are "125 x 125", which depends depend on the AI model in use.

| Category | Description                      |
| -------- | -------------------------------- |
| Shape    | 15625(125 x 125)-dim flat vector |
| Values   | class indexes                    |

The array details are as follows:

| Index                 | Corresponding image coordinates |
| --------------------- | ------------------------------- |
| Idx=0, ..., 124       | x0y0, x0y1, ..., x0y124         |
| Idx=125, ..., 249     | x1y0, x1y1, ..., x1y124         |
| ...                   |                                 |
| Idx=15501, ..., 15625 | x124y0, x124y1, ..., x124y124   |

## Initialize process
The Custom Parameters are used to configure the "**Edge Application**" and is passed by a configuration callback to the **`onConfigure(char *topic, void *value, int valuesize)`** event function as a json string inside the **`value`** parameter.
 
This function parses this **`value`** and sets the values of the variables corresponding to the post-processing parameters. Since Custom Parameters are in JSON format, it is parsed using [parson](../../libs/third_party/parson/).

The following parameters are used in the sample "**Edge Application**":

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
Even if the Custom Parameters are not specified in the "**Console for AITRIOS**", it is possible to operate by setting an initial value in [the program](./data_processor/src/segmentation_utils.hpp). The sample sets the following values:
 
```json
{
    "input_width" : 125,
    "input_height" : 125
}
```

The parameters used in this sample is defined in DTDL [**`edge_app_ss_interface.json`**](./package/edge_app_ss_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Analyze process
See [**`segmentation_data_processor.cpp`**](./data_processor/src/segmentation_data_processor.cpp) for the Analyze process.

**`DataProcessorAnalyze`** provides the function to parse the output of the AI model received as a char pointer  **`*in_data`** (the type casted output of the IMX500, excluding the format dependent on IMX500), sort it, and output the data in which the class ID is set for each pixel of the **`input_width`** * **`input_height`** image, in the FlatBuffers format.

The sample uses the following FlatBuffers schema  [**`semantic_segmentation.fbs`**](./schemas/semantic_segmentation.fbs):

```
namespace SmartCamera;

table SemanticSegmentationData {
  height: ushort;
  width: ushort;
  class_id_map: [ushort];
  num_class_id: ushort;
  score_map: [float]; 
}

table SemanticSegmentationTop {
  perception:SemanticSegmentationData;
}

root_type SemanticSegmentationTop;
```

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
