# Edge Application Sample for Object Detection
"**Edge Application**" sample for Object Detection is post-processing of the AI model output to limit the output by setting the maximum number of detections and the threshold.

Various object detection models are supported.

The following models have been verified:

- Custom Vision models (e.g., SSD-based, available via "Console for AITRIOS")
- SSD MobileNet V1/V2
- YOLOv8n


## Initialize process
The Custom Parameters are used to configure the "**Edge Application**" and is passed by a configuration callback to the **`onConfigure(char *topic, void *value, int valuesize)`** event function as a json string inside the **`value`** parameter.

This function parses this **`value`** and sets the values of the variables corresponding to the post-processing parameters. Since Custom Parameters are in JSON format, it is parsed using [parson](../../libs/third_party/parson/).

The following parameters defined under **`custom_settings`** are used in the sample "**Edge Application**":

### ai_models.detection

This object contains two objects: **`ai_model_bundle_id`** and **`parameters`**, which are described next.

- **`ai_model_bundle_id`**<br>
  Description: ID to specify the AI ​​model to use. 6 digit HEX value string like '000000'. 
>**NOTE**
>
>This value can be obtained after deploying the AI ​​model from the Console UI.
  

- **`parameters`**<br>
  Description: Parameters of AI model. 

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

  - **`bbox_order`**<br>
    Description: Indicates the coordinate order used for bounding boxes, such as `xyxy`, `xywh`, or `xxyy`.  
Unless otherwise specified, `x` and `y` denote the top-left coordinates (`xmin`, `ymin`) of the bounding box.  
The default is `yxyx`.
  >**NOTE**
  >
  >The value of the parameter depends on the AI model.The order would be converted to the **`Detections`** as defined in [**`detection_utils.hpp`**](./data_processor/src/detection_utils.hpp) format during the [Format](#3-format) stage.

  - **`bbox_normalization`**<br>
  　Description: Indicates whether bounding box values are normalized (`true`, default) or expressed as pixel values in the input tensor's coordinate system (`false`).
  >**NOTE**
  >
  >The value of the parameter depends on the AI model. Normalized values will be converted to pixel values during the [Format](#3-format) stage.
  
  - **`class_score_order`**<br>
  　Description: Specifies the order in which class ID and score appear in the output. For example, `class_score` means `(class_id, score)` and `score_class` means `(score, class_id)`.
  >**NOTE**
  >
  >The value of the parameter depends on the AI model.

### area (Optional)

You can use this object to find the number of objects in a certain area.
If area is included in Configuration, the count result of the objects within the set area and the objects within the area will be sent.

- **`coordinates`**<br>
  Description: Coordinates of the specified area. 

  - **`left`**<br>
    Description: X coordinate for the top left corner of Area.<br>
 
  - **`top`**<br>
    Description: Y coordinate for the top left corner of Area.<br>

  - **`right`**<br>
    Description: X coordinate for the bottom right corner of Area.<br>

  - **`bottom`**<br>
    Description: Y coordinate for the bottom right corner of Area.<br>

- **`overlap`**<br>
  Description: Threshold on the value of the overlap between the Area and the detected object, above which an object is considered to be inside the area. The overlap is calculated as ratio of the intersection area of the object and the Area, divided by the area of the object.

- **`class_id`**<br>
  Description: Only objects that match one of the specified **`class_id`** s are checked. Setting **`class_id`** to an empty array means that all objects will be checked. The count results of objects detected in the area will be returned, so only results with a count of **`1`** or greater will be returned.<br>
  For example, there is one object with **`class_id = 3`** in the area.<br>
  **`"class_id": [3, 5]`** will return **`"area_count": ["3": 1]`**.<br>
  **`"class_id": []`** will return **`"area_count": ["3": 1]`**, in the same situation as above.<br>

    >**NOTE**
  >
  >The value of the parameter depends on the AI model.


### metadata_settings

You can use this object to specify the metada format.

- **`format`**<br>
  Description: **`0`** means BASE64 encoding, **`1`** means JSON serialization.

### Custom Parameters
Even if the Custom Parameters are not specified in the "**Console for AITRIOS**", it is possible to operate by setting an initial value in [the program](./data_processor/src/detection_utils.cpp). The sample sets the following values:

```json
"custom_settings": {
    "ai_models": {
        "detection": {
            "ai_model_bundle_id": "000001",
            "parameters": {
                "max_detections": 10,
                "threshold": 0.3,
                "input_width": 320,
                "input_height": 320,
                "bbox_order": "xyxy",
                "bbox_normalization": false,
                "class_score_order": "cls_score"
            }
        }
    },
    "area": {
        "coordinates": {
            "left": 10,
            "top": 15,
            "right": 610,
            "bottom": 415
        },
        "overlap": 0.5,
        "class_id": [
            1,
            3
        ]
    },
    "metadata_settings": {
        "format": 0
    }
}
```

The parameters used in this sample are defined in DTDL [**`edge_app_od_interface.json`**](./package/edge_app_od_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Analyze process
The process consists of three steps:<br>
```
// 1. Parse
  Detections *detections =
      CreateDetections(in_data, in_size, analyze_params);

// 2. Filter 
  FilterByParams(&detections, analyze_params);

// 3. Format
  JSON_Value *tensor_output = MakeDetectionJson(detections, analyze_params);
```

See [**`detection_data_processor.cpp`**](./data_processor/src/detection_data_processor.cpp) for the Analyze process.
### 1. Parse

**`CreateDetections`** provides the function to parse the output of the Detection AI model received as a char pointer  **`*in_data`** (the type casted output of the IMX500, excluding the format dependent on IMX500) and output the result as a format of **`Detections`** as defined in [**`detection_utils.hpp`**](./data_processor/src/detection_utils.hpp).
```
typedef struct {
  uint8_t class_id;
  float score;
  BBox bbox;
} DetectionData;

typedef struct {
  uint16_t num_detections;
  DetectionData *detection_data;
} Detections;
```

### 2. Filter

**`FilterByParams`** filters **`Detections`** data parsed by **`CreateDetections`** using a specified threshold.

### 3. Format
**`MakeDetectionJson`** and **`MakeAreaJson`** take **`Detections`** as input and serialize them as JSON format.<br>
If **`area`** is NOT included in Configuration, this sample "**Edge Application**" formats metadata with **`MakeDetectionJson`** and sends it like the following sample json.
```json
[
    {
        "class_id": 2,
        "score": 0.62,
        "bounding_box": {
            "left": 50,
            "top": 50,
            "right": 68,
            "bottom": 90
        }
    },
    {
        "class_id": 1,
        "score": 0.59,
        "bounding_box": {
            "left": 55,
            "top": 56,
            "right": 65,
            "bottom": 64
        }
    }
]
  ```

If **`area`** is included in Configuration, this sample "**Edge Application**" formats metadata with **`MakeAreaJson`** and sends it like the following sample json.


```json
{
    "area_count": {
        "1": 1,
        "3": 0
    },
    "detections": [
        {
            "class_id": 1,
            "score": 0.59,
            "bounding_box": {
                "left": 55,
                "top": 56,
                "right": 65,
                "bottom": 64
            }
        }
    ]
}
```
  >**NOTE**
  >
  >**`detections`** only includes objects that are determined to be within **`area`** and match the specified **`class_id`**.

**`MakeDetectionFlatbuffer`** and **`MakeAreaFlatbuffer`** take **`Detections`** as input and serialize them as defined in the Flatbuffers schema.
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

table CountData{
    class_id:uint;
    count:uint;
}

table AreaCountTop {
  area_count:[CountData];
  perception:ObjectDetectionData;
}

table ObjectDetectionTop {
  perception:ObjectDetectionData;
}

root_type ObjectDetectionTop;
union ObjectDetectionUnion {ObjectDetectionTop, AreaCountTop}

table ObjectDetectionRoot {
  metadata:ObjectDetectionUnion;
}

root_type ObjectDetectionRoot;
```

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
