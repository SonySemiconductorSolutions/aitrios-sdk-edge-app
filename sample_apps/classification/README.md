# Edge Application Sample for Image Classification
"**Edge Application**" sample for Image Classification is post-processing of the AI model output to limit the output by setting the maximum number of predictions.

Supporting Image Classification models such as the following.
- Image Classification AI model

## Initialize process
The Custom Parameters are used to configure the "**Edge Application**" and is passed by a configuration callback to the **`onConfigure(char *topic, void *value, int valuesize)`** event function as a json string inside the **`value`** parameter.

This function parses this **`value`** and sets the values of the variables corresponding to the post-processing parameters. Since Custom Parameters are in JSON format, they are parsed using [parson](../../libs/third_party/parson/).

The following parameters are used in the sample "**Edge Application**":

>**NOTE**
>
>The value of the parameter depends on the AI model.

- **`max_predictions`**<br>
  Description: Threshold of predictions number. The maximum number of predictions you want to get after "**Edge Application**".
  > **NOTE**
  >
  > For Image Classification AI model, it can be changed from 0 to the number of classes that the model is capable to detect. If an invalid value is entered, the default value is applied.

### Custom Parameters
Even if the Custom Parameters are not specified in the "**Console for AITRIOS**", it is possible to operate by setting an initial value in [the program](./data_processor/src/classification_utils.hpp#L30). The sample sets the following values:

```json
{
    "max_predictions": 3
}
```

> **NOTE**
>
> The preceding Custom Parameters are used when the number of classes of AI model you use is set to 18.

The parameters used in this sample is defined in DTDL [**`edge_app_ic_interface.json`**](./package/edge_app_ic_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Analyze process
See [**`classification_data_processor.cpp`**](./data_processor/src/classification_data_processor.cpp) for the Analyze process.

**`DataProcessorAnalyze`** provides the function to parse the output of the AI model received as a char pointer  **`*in_data`** (the type casted output of the IMX500, excluding the format dependent on IMX500), sort it, and output the data of the number of **`max_predictions`** specified in Custom Parameters, in the FlatBuffers format.

The sample uses the following FlatBuffers schema  [**`classification.fbs`**](./schemas/classification.fbs):

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
}

root_type ClassificationTop;
```

The float values are passed as the argument **`p_data`**, so they are sorted and stored in FlatBuffers up to the maximum set by **`max_predictions`** in Custom Parameters.

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
