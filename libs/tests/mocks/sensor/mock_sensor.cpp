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

#include "mock_sensor.hpp"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "log.h"
#include "memory_manager.hpp"
#include "mock_device.hpp"
#include "parson.h"
#include "sensor.h"
#include "sm_api.hpp"
#include "testing_utils.hpp"

#define OUTPUT "output"
#define INPUT "input"

EdgeAppLibSensorStream stream_check = 0;

static int EdgeAppLibSensorCoreInitCalled = 0;
static int EdgeAppLibSensorCoreInitSuccess = 0;
static int EdgeAppLibSensorCoreExitCalled = 0;
static int EdgeAppLibSensorCoreExitSuccess = 0;
static int EdgeAppLibSensorCoreOpenStreamCalled = 0;
static int EdgeAppLibSensorCoreOpenStreamSuccess = 0;
static int EdgeAppLibSensorCoreCloseStreamCalled = 0;
static int EdgeAppLibSensorCoreCloseStreamSuccess = 0;
static int EdgeAppLibSensorStartCalled = 0;
static int EdgeAppLibSensorStartSuccess = 0;
static int EdgeAppLibSensorStopCalled = 0;
static int EdgeAppLibSensorStopSuccess = 0;
static int EdgeAppLibSensorGetFrameCalled = 0;
static int EdgeAppLibSensorGetFrameSuccess = 0;
static int EdgeAppLibSensorReleaseFrameCalled = 0;
static int EdgeAppLibSensorReleaseFrameSuccess = 0;
static int EdgeAppLibSensorStreamGetPropertyCalled = 0;
static int EdgeAppLibSensorStreamGetPropertySuccess = 0;
static int EdgeAppLibSensorStreamSetPropertyCalled = 0;
static int EdgeAppLibSensorStreamSetPropertySuccess = 0;
static int EdgeAppLibSensorFrameGetChannelFromChannelIdCalled = 0;
static int EdgeAppLibSensorFrameGetChannelFromChannelIdSuccess = 0;
static int EdgeAppLibSensorChannelGetRawDataCalled = 0;
static int EdgeAppLibSensorChannelGetRawDataSuccess = 0;
static int EdgeAppLibSensorChannelGetPropertyCalled = 0;
static int EdgeAppLibSensorChannelGetPropertySuccess = 0;
static int EdgeAppLibSensorChannelSubFrameCurrentNum = 1;
static int EdgeAppLibSensorChannelSubFrameDivisionNum = 1;
static int EdgeAppLibSensorGetLastErrorStringCalled = 0;
static int EdgeAppLibSensorGetLastErrorStringSuccess = 0;
static int EdgeAppLibSensorGetLastErrorLevelCalled = 0;
static int EdgeAppLibSensorInputDataTypeEnableChannelSuccess = 0;
EdgeAppLibSensorErrorLevel EdgeAppLibSensorGetLastErrorLevelSuccess =
    AITRIOS_SENSOR_LEVEL_UNDEFINED;
static int EdgeAppLibSensorGetLastErrorCauseCalled = 0;
EdgeAppLibSensorErrorCause EdgeAppLibSensorGetLastErrorCauseSuccess =
    AITRIOS_SENSOR_ERROR_NONE;
static EdgeAppLibSensorImageProperty EdgeAppLibSensorChannelImageProperty = {};

typedef struct {
  void *value;
  size_t value_size;
} MapValue;

static std::map<std::string, MapValue> property_map;
static uint32_t frame_number = 0;
static float **data = nullptr;
static uint32_t *lengths = nullptr;
static uint32_t num_arrays = 0;
static uint32_t tensor_size = 0;

float *load_binary_file(const std::string &in_bin_file, uint32_t *out_size) {
  float *data = nullptr;
  std::ifstream ifs(in_bin_file, std::ios::in | std::ios::binary);

  if (!ifs) {
    return nullptr;
  }

  ifs.seekg(0, ifs.end);
  uint32_t file_size = ifs.tellg();
  ifs.seekg(0, ifs.beg);

  uint32_t num_floats = file_size / sizeof(float);

  data = static_cast<float *>(malloc(file_size));
  if (!data) {
    ifs.close();
    return nullptr;
  }

  ifs.read(reinterpret_cast<char *>(data), file_size);
  ifs.close();
  *out_size = file_size;
  return data;
}
char *load_jsonc_file(const char *filepath) {
  JSON_Value *root = json_parse_file_with_comments(filepath);
  if (root == NULL) {
    fprintf(stderr, "Failed to parse JSONC file: %s\n", filepath);
    return strdup("[]");
  }

  char *serialized = json_serialize_to_string(root);
  char *json_buffer = strdup(serialized);

  json_free_serialized_string(serialized);
  json_value_free(root);

  return json_buffer;
}

// Output tensor for classification to 18 categories.
// Correct ppl parameters to process this output tensor
// can be found in rearch/sample_apps/classification/test_data

#ifdef MOCK_CLASSIFICATION
#define MOCK_DATA                                                      \
  "[[0.171875, 0.01074225, 0.01074225, 0.195312, 0.070312, 0.050781, " \
  "0.027344,0.01074225, 0.027344, 0.01074225, 0.171875, 0.0625, "      \
  "0.042969, 0.09375, 0.01074225, 0.01074225, 0.01074225, 0.01074225 ]]"
// Output tensor for number of detection equal to 10 which has the order
// y_min (10), x_min (10), y_max (10), x_max (10), class (10), score (10),
// number_of_detections (1) Correct ppl parameters to process this output tensor
// can be found in rearch/sample_apps/detection/test_data
#elif defined(MOCK_DETECTION)
#define MOCK_DATA                                                              \
  "[[0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.15, 0.25, 0.35, "     \
  "0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.6, 0.7, 0.8, 0.0, 0.0, 0.0, "    \
  "0.0, 0.0, 0.0, 0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 235, " \
  "132, 95, 187, 0, 0, 0, 0, 0, 0, 0.8, 0.2, 0.6, 0.4, 0.0, 0.0, 0.0, 0.0, "   \
  "0.0, 0.0,10]]"

// Correct ppl parameters to process this output tensor
// can be found in rearch/sample_apps/segmentation/test_data
#elif defined(MOCK_SEGMENTATION)
#define MOCK_DATA "[ 1, 2, 1, 3, 2, 3, 1, 3, 2, 4, 1, 3, 2, 4, 4, 1 ]"

#elif defined(MOCK_PASSTHROUGH)
// To run onIterate() successful, set mock data
#define MOCK_DATA                                                              \
  "[[0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.15, 0.25, 0.35, "     \
  "0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.6, 0.7, 0.8, 0.0, 0.0, 0.0, "    \
  "0.0, 0.0, 0.0, 0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 235, " \
  "132, 95, 187, 0, 0, 0, 0, 0, 0, 0.8, 0.2, 0.6, 0.4, 0.0, 0.0, 0.0, 0.0, "   \
  "0.0, 0.0,10]]"

#elif defined(MOCK_APITEST)
// To run onIterate() successful, set mock data
#define MOCK_DATA                                                              \
  "[[0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.15, 0.25, 0.35, "     \
  "0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.6, 0.7, 0.8, 0.0, 0.0, 0.0, "    \
  "0.0, 0.0, 0.0, 0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 235, " \
  "132, 95, 187, 0, 0, 0, 0, 0, 0, 0.8, 0.2, 0.6, 0.4, 0.0, 0.0, 0.0, 0.0, "   \
  "0.0, 0.0,10]]"
#elif defined(MOCK_LP_RECOG)
#define MOCK_DATA          \
  "[[0.8, 0.9, 0.7, 0.1, " \
  "0.1, 0.3, 0.5, 0.0, "   \
  "0.1, 0.3, 0.5, 0.0, "   \
  "0.2, 0.4, 0.6, 0.0, "   \
  "0.2, 0.4, 0.6, 0.0, "   \
  "4.0, 1.0, 2.0, 3.0, 0.0]]"
#elif defined(MOCK_BARCODE)
#define MOCK_DATA                                                              \
  "[[0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.15, 0.25, 0.35, "     \
  "0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.6, 0.7, 0.8, 0.0, 0.0, 0.0, "    \
  "0.0, 0.0, 0.0, 0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 235, " \
  "132, 95, 187, 0, 0, 0, 0, 0, 0, 0.8, 0.2, 0.6, 0.4, 0.0, 0.0, 0.0, 0.0, "   \
  "0.0, 0.0,10]]"
#elif defined(MOCK_GAZE)
char *MOCK_DATA =
    load_jsonc_file("./sample_apps/gaze/test_data/mock_gaze.jsonc");
#elif defined(MOCK_POSENET)
char *MOCK_DATA = (char *)load_binary_file(
    "./sample_apps/posenet/test_data/westworld_out_w481_h353.bin",
    &tensor_size);
#elif defined(MOCK_SWITCH)
#define MOCK_DATA                                                             \
  "[[0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.15, 0.25, "          \
  "0.35, "                                                                    \
  "0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.6, 0.7, 0.8, 0.0, 0.0, 0.0, "   \
  "0.0, 0.0, 0.0, 0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 15, " \
  "132, 95, 187, 0, 0, 0, 0, 0, 0, "                                          \
  "0.8, 0.2, 0.6, 0.4, 0.0, 0.0, "                                            \
  "0.0, 0.0, 0.0, 0.0, "                                                      \
  "10]]"
#else
#warning "Empty mock data"
#define MOCK_DATA "[]"
#endif

static EdgeAppLibSensorFrame latest_frame = 10;
static EdgeAppLibSensorChannel latest_channel_id = 0;
static std::map<EdgeAppLibSensorFrame, std::vector<EdgeAppLibSensorChannel>>
    map_frame_channels;
static std::map<EdgeAppLibSensorChannel, uint32_t> map_channel_channel_id;
static std::map<EdgeAppLibSensorChannel, EdgeAppLibSensorRawData *>
    map_channel_data;

int wasEdgeAppLibSensorCoreInitCalled() {
  return EdgeAppLibSensorCoreInitCalled;
}
void setEdgeAppLibSensorCoreInitFail() { EdgeAppLibSensorCoreInitSuccess = -1; }
void resetEdgeAppLibSensorCoreInitSuccess() {
  EdgeAppLibSensorCoreInitSuccess = 0;
}
void resetEdgeAppLibSensorCoreInitCalled() {
  EdgeAppLibSensorCoreInitCalled = 0;
}

int wasEdgeAppLibSensorCoreExitCalled() {
  return EdgeAppLibSensorCoreExitCalled;
}
void setEdgeAppLibSensorCoreExitFail() { EdgeAppLibSensorCoreExitSuccess = -1; }
void resetEdgeAppLibSensorCoreExitSuccess() {
  EdgeAppLibSensorCoreExitSuccess = 0;
}
void resetEdgeAppLibSensorCoreExitCalled() {
  EdgeAppLibSensorCoreExitCalled = 0;
}
namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
int32_t SensorCoreInit(EdgeAppLibSensorCore *core) {
  if (core == NULL) {
    return -1;
  }
  EdgeAppLibSensorCoreInitCalled += 1;
  *core = (EdgeAppLibSensorCore)DUMMY_HANDLE;
  return EdgeAppLibSensorCoreInitSuccess;
}

int32_t SensorCoreExit(EdgeAppLibSensorCore core) {
  EdgeAppLibSensorCoreExitCalled += 1;
  for (auto it = property_map.begin(); it != property_map.end();) {
    if (it->second.value) free(it->second.value);
    it = property_map.erase(it);
  }
  return EdgeAppLibSensorCoreExitSuccess;
}
int32_t SensorStreamSetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, const void *value,
                                size_t value_size) {
  if (EdgeAppLibSensorStreamSetPropertySuccess != 0) {
    return EdgeAppLibSensorStreamSetPropertySuccess;
  }

  if (std::string(property_key) ==
      std::string(AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY)) {
    const EdgeAppLibSensorCameraFrameRateProperty *frameRateProperty =
        static_cast<const EdgeAppLibSensorCameraFrameRateProperty *>(value);
    if (frameRateProperty->denom == 0) {
      return -1;
    }
  }

  MapValue _value;
  std::string _key(property_key);
  if (property_key != NULL) {
    std::string _key(property_key);
    if (property_map.find(property_key) != property_map.end())
      _value = property_map[property_key];
    else {
      _value.value = malloc(value_size);
      _value.value_size = value_size;
    }
    if (value != NULL) {
      memcpy(_value.value, value, value_size);
      property_map[_key] = _value;
      EdgeAppLibSensorStreamSetPropertyCalled = 1;
      updateProperty(stream, property_key, value, value_size);
    }
  }
  return EdgeAppLibSensorStreamSetPropertySuccess;
}
int32_t SensorFrameGetChannelFromChannelId(EdgeAppLibSensorFrame frame,
                                           uint32_t channel_id,
                                           EdgeAppLibSensorChannel *channel) {
  EdgeAppLibSensorFrameGetChannelFromChannelIdCalled = 1;
  if (EdgeAppLibSensorFrameGetChannelFromChannelIdSuccess != 0) {
    return EdgeAppLibSensorFrameGetChannelFromChannelIdSuccess;
  }
  if (channel_id != (uint32_t)AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE &&
      channel_id != (uint32_t)AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT &&
      channel_id != (uint32_t)AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_RAW_IMAGE) {
    return -1;
  }
  *channel = latest_channel_id++;
  map_frame_channels[frame].push_back(*channel);
  map_channel_channel_id[*channel] = channel_id;
  return EdgeAppLibSensorFrameGetChannelFromChannelIdSuccess;
}
int32_t SensorChannelGetRawData(EdgeAppLibSensorChannel channel,
                                struct EdgeAppLibSensorRawData *raw_data) {
  EdgeAppLibSensorChannelGetRawDataCalled = 1;
  if (EdgeAppLibSensorChannelGetRawDataSuccess != 0) {
    return EdgeAppLibSensorChannelGetRawDataSuccess;
  }

  uint64_t channel_id = map_channel_channel_id[channel];
  if (channel_id == (uint64_t)AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE) {
    uint8_t out_data_aux[15] = {0, 1, 2,  3,  4,  5,  6, 7,
                                8, 9, 10, 11, 12, 13, 14};
    raw_data->address = (uint8_t *)malloc(sizeof(out_data_aux));
    raw_data->type = strdup("image");
    raw_data->size = 15;
    raw_data->timestamp = 10;
    memcpy(raw_data->address, out_data_aux, sizeof(out_data_aux));

    struct EdgeAppLibSensorRawData *raw_data_cpy =
        (struct EdgeAppLibSensorRawData *)malloc(sizeof(*raw_data));
    memcpy(raw_data_cpy, raw_data, sizeof(*raw_data));
    map_channel_data[channel] = raw_data_cpy;
    EsfMemoryManagerHandle handle = (EsfMemoryManagerHandle)DUMMY_HANDLE;
    size_t size = 0;
#if defined(MOCK_PASSTHROUGH) || defined(MOCK_BARCODE) || defined(MOCK_LP_RECOG)
    setEsfMemoryManagerPreadFail();
#endif
    if (EsfMemoryManagerPread(handle, (void *)raw_data->address, raw_data->size,
                              0, &size) == 0) {
      LOG_INFO("EsfMemoryManagerPread success");
      // We need to assume the size of the raw data is smaller if the input
      // tensor is encoded
      raw_data->size = raw_data->size / 2;
      raw_data->address = malloc(raw_data->size);
    }
    return EdgeAppLibSensorChannelGetRawDataSuccess;
  }
  if (channel_id == (uint64_t)AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_RAW_IMAGE) {
    uint8_t out_data_aux[15] = {0, 1, 2,  3,  4,  5,  6, 7,
                                8, 9, 10, 11, 12, 13, 14};
    raw_data->address = (uint8_t *)malloc(sizeof(out_data_aux));
    raw_data->type = strdup("image");
    raw_data->size = 15;
    raw_data->timestamp = 10;
    memcpy(raw_data->address, out_data_aux, sizeof(out_data_aux));
    struct EdgeAppLibSensorRawData *raw_data_cpy =
        (struct EdgeAppLibSensorRawData *)malloc(sizeof(*raw_data));
    memcpy(raw_data_cpy, raw_data, sizeof(*raw_data));
    map_channel_data[channel] = raw_data_cpy;
    return EdgeAppLibSensorChannelGetRawDataSuccess;
  }
  if (channel_id == (uint64_t)AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT) {
    JSON_Value *output_tensor_val = json_parse_string(MOCK_DATA);

    std::string _key(AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY);
    auto ref = property_map.find(_key);
    if (ref != property_map.end()) {
      EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle_id_prop = {0};
      memcpy(&ai_model_bundle_id_prop, ref->second.value,
             sizeof(ai_model_bundle_id_prop));
      if (strncmp(ai_model_bundle_id_prop.ai_model_bundle_id, "000001",
                  strlen(ai_model_bundle_id_prop.ai_model_bundle_id)) == 0) {
        // mock data for switch_dnn detection
        LOG_INFO("Using mock data for switch_dnn detection\n");
        const char *mock_data_detection =
            "[[0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.15, 0.25, "
            "0.35, "
            "0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.6, 0.7, 0.8, 0.0, 0.0, "
            "0.0, "
            "0.0, 0.0, 0.0, 0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, "
            "0.0, 15, "
            "132, 95, 187, 0, 0, 0, 0, 0, 0,"
            "0.8, 0.2, 0.6, 0.4, 0.0, 0.0, "
            "0.0, 0.0, 0.0, 0.0,"
            "10]]";
        json_value_free(output_tensor_val);
        output_tensor_val = json_parse_string(mock_data_detection);
      } else if (strcmp(ai_model_bundle_id_prop.ai_model_bundle_id,
                        "${network_id_2}") == 0) {
        // mock data for switch_dnn classification
        LOG_INFO("Using mock data for switch_dnn classification");
        char *mock_data_classification = (char *)malloc(10 * 1000);
        char *loc = mock_data_classification;
        size_t tempLen;
        for (int i = 0; i < 1001; i++) {
          if (i == 0) {
            snprintf(loc, 10, "[[%d,", 0);
          } else if (i == 26) {
            snprintf(loc, 10, "%d,", 1);
          } else if (i != 1000) {
            snprintf(loc, 10, "%d,", 0);
          } else {
            snprintf(loc, 10, "%d]]", 0);
          }
          tempLen = strlen(loc);
          loc += tempLen;
        }
        json_value_free(output_tensor_val);
        output_tensor_val = json_parse_string(mock_data_classification);
        free(mock_data_classification);
      }
    }
    const char *output_tensor = json_serialize_to_string(output_tensor_val);
    if (output_tensor_val != NULL) {
      data = StringToFloatArrayForIT(output_tensor, &num_arrays, &lengths);
      if (frame_number >= num_arrays) frame_number = 0;
      LOG_INFO("num_arrays %d frame_number: %d lengths: %d\n", num_arrays,
               frame_number, lengths[frame_number]);
      raw_data->address = malloc(lengths[frame_number] * sizeof(float));
      memcpy(raw_data->address, data[frame_number],
             lengths[frame_number] * sizeof(float));
      raw_data->size = lengths[frame_number] * sizeof(float);
      frame_number++;
    } else {
      raw_data->address = (float *)MOCK_DATA;
      raw_data->size = tensor_size;
      printf("posenet MOCK_DATA size: %d\n", tensor_size);
    }
    raw_data->type = strdup("float");
    raw_data->timestamp = 10;

    struct EdgeAppLibSensorRawData *raw_data_cpy =
        (struct EdgeAppLibSensorRawData *)malloc(sizeof(*raw_data));
    memcpy(raw_data_cpy, raw_data, sizeof(*raw_data));
    map_channel_data[channel] = raw_data_cpy;

    json_free_serialized_string((char *)output_tensor);
    json_value_free(output_tensor_val);

    return EdgeAppLibSensorChannelGetRawDataSuccess;
  }
  return -1;
}
int32_t SensorCoreOpenStream(EdgeAppLibSensorCore core, const char *stream_key,
                             EdgeAppLibSensorStream *stream) {
  if (core == 0) {
    return -1;
  }
  EdgeAppLibSensorCoreOpenStreamCalled += 1;
  if (EdgeAppLibSensorCoreOpenStreamSuccess != 0) {
    return EdgeAppLibSensorCoreOpenStreamSuccess;
  }
  *stream = (EdgeAppLibSensorStream)DUMMY_HANDLE;
  stream_check = *stream;

  EdgeAppLibSensorChannelImageProperty.height = 1;
  EdgeAppLibSensorChannelImageProperty.width = 5;
  EdgeAppLibSensorChannelImageProperty.stride_bytes = 5 * 3;
  snprintf(EdgeAppLibSensorChannelImageProperty.pixel_format,
           AITRIOS_SENSOR_PIXEL_FORMAT_LENGTH,
           AITRIOS_SENSOR_PIXEL_FORMAT_RGB24);

  EdgeAppLibSensorImageCropProperty value = {
      .left = 10, .top = 15, .width = 20, .height = 25};
  EdgeAppLib::SensorStreamSetProperty(
      *stream, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY, &value,
      sizeof(EdgeAppLibSensorImageCropProperty));
  EdgeAppLibSensorInputDataTypeProperty enabled = {.count = 1, .channels = {0}};
  EdgeAppLib::SensorStreamSetProperty(
      *stream, AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY, &enabled,
      sizeof(EdgeAppLibSensorInputDataTypeProperty));

  EdgeAppLibSensorInfoStringProperty sensor_name = {0};
  sensor_name.category = AITRIOS_SENSOR_INFO_STRING_SENSOR_NAME;

  // for one device.
  strncpy(sensor_name.info, "IMX500", strlen("IMX500"));

  // for the other device.
  // strncpy(sensor_name.info, "AI-ISP", strlen("AI-ISP"));

  EdgeAppLib::SensorStreamSetProperty(
      *stream, AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY, &sensor_name,
      sizeof(EdgeAppLibSensorInfoStringProperty));

  const char *dummySensorId = "00000000000000000000000000000000";

  EdgeAppLibSensorInfoStringProperty sensor_version_id = {0};
  if (!strncmp("IMX500", sensor_name.info, strlen("IMX500"))) {
    sensor_version_id.category = AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION;
    strncpy(sensor_version_id.info, dummySensorId, strlen(dummySensorId));
  } else if (!strncmp("AI-ISP", sensor_name.info, strlen("AI-ISP"))) {
    sensor_version_id.category =
        AITRIOS_SENSOR_INFO_STRING_AIISP_AI_MODEL_VERSION;
    strncpy(sensor_version_id.info, dummySensorId, strlen(dummySensorId));
  }
  EdgeAppLib::SensorStreamSetProperty(
      *stream, AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY, &sensor_version_id,
      sizeof(EdgeAppLibSensorInfoStringProperty));

  return EdgeAppLibSensorCoreOpenStreamSuccess;
}
int32_t SensorGetFrame(EdgeAppLibSensorStream stream,
                       EdgeAppLibSensorFrame *frame, int32_t timeout_msec) {
  EdgeAppLibSensorGetFrameCalled = 1;
  if (EdgeAppLibSensorGetFrameSuccess != 0) {
    return EdgeAppLibSensorGetFrameSuccess;
  }
  memcpy(frame, &latest_frame, sizeof(latest_frame));
  latest_frame += 1;
  return EdgeAppLibSensorGetFrameSuccess;
}
enum EdgeAppLibSensorErrorCause SensorGetLastErrorCause(void) {
  EdgeAppLibSensorGetLastErrorCauseCalled = 1;
  return EdgeAppLibSensorGetLastErrorCauseSuccess;
}
int32_t SensorReleaseFrame(EdgeAppLibSensorStream stream,
                           EdgeAppLibSensorFrame frame) {
  auto channels_it = map_frame_channels.find(frame);

  if (channels_it != map_frame_channels.end()) {
    for (auto channel : channels_it->second) {
      auto data_it = map_channel_data.find(channel);
      if (data_it != map_channel_data.end()) {
        auto data_relase = data_it->second;
        // Don't free MOCK_DATA as it's statically allocated
        if (data_relase->address != (void *)MOCK_DATA) {
          free(data_relase->address);
        }
        free(data_relase->type);
        free(data_relase);
        map_channel_data.erase(data_it);
      }
      map_channel_channel_id.erase(map_channel_channel_id.find(channel));
    }
    map_frame_channels.erase(channels_it);
  }

  if (data) {
    for (uint32_t i = 0; i < num_arrays; ++i) {
      free(data[i]);
    }
    free(data);
    data = nullptr;
  }

  if (lengths) {
    free(lengths);
    lengths = nullptr;
  }
  EdgeAppLibSensorReleaseFrameCalled = 1;
  return EdgeAppLibSensorReleaseFrameSuccess;
}
int32_t SensorStreamGetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, void *value,
                                size_t value_size) {
  EdgeAppLibSensorStreamGetPropertyCalled = 1;
  if (property_key != NULL) {
    std::string _key(property_key);
    auto ref = property_map.find(_key);
    if (ref == property_map.end()) return -1;
    if (value != NULL) {
      memcpy(value, ref->second.value, value_size);
    }
  }
  return EdgeAppLibSensorStreamGetPropertySuccess;
}
int32_t SensorCoreCloseStream(EdgeAppLibSensorCore core,
                              EdgeAppLibSensorStream stream) {
  core = 0;
  EdgeAppLibSensorCoreCloseStreamCalled = 1;
  for (auto it = property_map.begin(); it != property_map.end();) {
    if (it->second.value) free(it->second.value);
    it = property_map.erase(it);
  }
  return EdgeAppLibSensorCoreCloseStreamSuccess;
}
enum EdgeAppLibSensorErrorLevel SensorGetLastErrorLevel(void) {
  EdgeAppLibSensorGetLastErrorLevelCalled = 1;
  return EdgeAppLibSensorGetLastErrorLevelSuccess;
}
int32_t SensorChannelGetProperty(EdgeAppLibSensorChannel channel,
                                 const char *property_key, void *value,
                                 size_t value_size) {
  EdgeAppLibSensorChannelGetPropertyCalled = 1;

  if (EdgeAppLibSensorChannelGetPropertySuccess != 0) {
    return EdgeAppLibSensorChannelGetPropertySuccess;
  }

  if (strncmp(property_key, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
              strlen(property_key)) == 0 ||
      strncmp(property_key, AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY,
              strlen(property_key)) == 0) {
    std::string _key(property_key);
    auto ref = property_map.find(_key);
    if (ref == property_map.end()) return -1;
    memcpy(value, ref->second.value, value_size);
  } else if (strncmp(property_key, AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY,
                     strlen(property_key)) == 0) {
    if (EdgeAppLibSensorChannelGetPropertySuccess == 0) {
      struct EdgeAppLibSensorSubFrameProperty *subframe =
          (struct EdgeAppLibSensorSubFrameProperty *)value;
      subframe->current_num = EdgeAppLibSensorChannelSubFrameCurrentNum;
      subframe->division_num = EdgeAppLibSensorChannelSubFrameDivisionNum;
    }
  } else if (strncmp(property_key, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                     strlen(property_key)) == 0) {
    EdgeAppLibSensorImageProperty *image_property =
        (EdgeAppLibSensorImageProperty *)value;

    image_property->width = EdgeAppLibSensorChannelImageProperty.width;
    image_property->height = EdgeAppLibSensorChannelImageProperty.height;
    image_property->stride_bytes =
        EdgeAppLibSensorChannelImageProperty.stride_bytes;

    snprintf(image_property->pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_LENGTH,
             "%s", EdgeAppLibSensorChannelImageProperty.pixel_format);
  }

  return EdgeAppLibSensorChannelGetPropertySuccess;
}
int32_t SensorStart(EdgeAppLibSensorStream stream) {
  int started = 1;
  stream = started;
  EdgeAppLibSensorStartCalled = 1;
  return EdgeAppLibSensorStartSuccess;
}
int32_t SensorStop(EdgeAppLibSensorStream stream) {
  int stopped = 0;
  stream = stopped;
  EdgeAppLibSensorStopCalled = 1;
  return EdgeAppLibSensorStopSuccess;
}
int32_t SensorGetLastErrorString(enum EdgeAppLibSensorStatusParam param,
                                 char *buffer, uint32_t *length) {
  // TODO: fill in the buffer and specify its length
  EdgeAppLibSensorGetLastErrorStringCalled = 1;
  return EdgeAppLibSensorGetLastErrorStringSuccess;
}
int32_t SensorInputDataTypeEnableChannel(
    EdgeAppLibSensorInputDataTypeProperty *property, uint32_t channel_id,
    bool enable) {
  // Same implementation as the original function, but this is a helper function
  // that doesn't interact with Senscord and doesn't need mocking
  if (property == NULL) {
    LOG_ERR("SensorInputDataTypeEnableChannel");
    return EdgeAppLibSensorInputDataTypeEnableChannelSuccess;
  }

  // Search for the channel in case it is already enabled
  uint32_t i = 0;
  for (; i < property->count; i++) {
    if (property->channels[i] == channel_id) {
      break;
    }
  }

  if (enable) {
    if (i == property->count) {
      // Channel not found, add it
      if (property->count < AITRIOS_SENSOR_CHANNEL_LIST_MAX) {
        property->channels[property->count++] = channel_id;
      } else {
        LOG_ERR(
            "SensorInputDataTypeEnableChannel too many channels "
            "enabled");
        return -1;
      }
    }
  } else {  // disable
    if (i != property->count) {
      // Channel found, remove it
      while (++i < property->count) {
        property->channels[i - 1] = property->channels[i];
      }
      --property->count;
    }
  }

  return 0;
}
#ifdef __cplusplus
}
#endif /* __cplusplus */
}  // namespace EdgeAppLib
int wasEdgeAppLibSensorStreamSetPropertyCalled() {
  return EdgeAppLibSensorStreamSetPropertyCalled;
}
void setEdgeAppLibSensorStreamSetPropertyFail() {
  EdgeAppLibSensorStreamSetPropertySuccess = -1;
}
void resetEdgeAppLibSensorStreamSetPropertySuccess() {
  EdgeAppLibSensorStreamSetPropertySuccess = 0;
}
void resetEdgeAppLibSensorStreamSetPropertyCalled() {
  EdgeAppLibSensorStreamSetPropertyCalled = 0;
}

int wasEdgeAppLibSensorCoreOpenStreamCalled() {
  return EdgeAppLibSensorCoreOpenStreamCalled;
}
void setEdgeAppLibSensorCoreOpenStreamFail() {
  EdgeAppLibSensorCoreOpenStreamSuccess = -1;
}
void resetEdgeAppLibSensorCoreOpenStreamSuccess() {
  EdgeAppLibSensorCoreOpenStreamSuccess = 0;
}
void resetEdgeAppLibSensorCoreOpenStreamCalled() {
  EdgeAppLibSensorCoreOpenStreamCalled = 0;
}

int wasEdgeAppLibSensorCoreCloseStreamCalled() {
  return EdgeAppLibSensorCoreCloseStreamCalled;
}
void setEdgeAppLibSensorCoreCloseStreamFail() {
  EdgeAppLibSensorCoreCloseStreamSuccess = -1;
}
void resetEdgeAppLibSensorCoreCloseStreamSuccess() {
  EdgeAppLibSensorCoreCloseStreamSuccess = 0;
}
void resetEdgeAppLibSensorCoreCloseStreamCalled() {
  EdgeAppLibSensorCoreCloseStreamCalled = 0;
}

int wasEdgeAppLibSensorStartCalled() { return EdgeAppLibSensorStartCalled; }
void setEdgeAppLibSensorStartFail() { EdgeAppLibSensorStartSuccess = -1; }
void resetEdgeAppLibSensorStartSuccess() { EdgeAppLibSensorStartSuccess = 0; }
void resetEdgeAppLibSensorStartCalled() { EdgeAppLibSensorStartCalled = 0; }

int wasEdgeAppLibSensorStopCalled() { return EdgeAppLibSensorStopCalled; }
void setEdgeAppLibSensorStopFail() { EdgeAppLibSensorStopSuccess = -1; }
void resetEdgeAppLibSensorStopSuccess() { EdgeAppLibSensorStopSuccess = 0; }
void resetEdgeAppLibSensorStopCalled() { EdgeAppLibSensorStopCalled = 0; }

int wasEdgeAppLibSensorGetFrameCalled() {
  return EdgeAppLibSensorGetFrameCalled;
}
void setEdgeAppLibSensorGetFrameFail() { EdgeAppLibSensorGetFrameSuccess = -1; }
void resetEdgeAppLibSensorGetFrameSuccess() {
  EdgeAppLibSensorGetFrameSuccess = 0;
}
void resetEdgeAppLibSensorGetFrameCalled() {
  EdgeAppLibSensorGetFrameCalled = 0;
}

int wasEdgeAppLibSensorReleaseFrameCalled() {
  return EdgeAppLibSensorReleaseFrameCalled;
}
void setEdgeAppLibSensorReleaseFrameFail() {
  EdgeAppLibSensorReleaseFrameSuccess = -1;
}
void resetEdgeAppLibSensorReleaseFrameSuccess() {
  EdgeAppLibSensorReleaseFrameSuccess = 0;
}
void resetEdgeAppLibSensorReleaseFrameCalled() {
  EdgeAppLibSensorReleaseFrameCalled = 0;
}

int wasEdgeAppLibSensorStreamGetPropertyCalled() {
  return EdgeAppLibSensorStreamGetPropertyCalled;
}
void setEdgeAppLibSensorStreamGetPropertyFail() {
  EdgeAppLibSensorStreamGetPropertySuccess = -1;
}
void resetEdgeAppLibSensorStreamGetPropertySuccess() {
  EdgeAppLibSensorStreamGetPropertySuccess = 0;
}
void resetEdgeAppLibSensorStreamGetPropertyCalled() {
  EdgeAppLibSensorStreamGetPropertyCalled = 0;
}

int wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled() {
  return EdgeAppLibSensorFrameGetChannelFromChannelIdCalled;
}
void setEdgeAppLibSensorFrameGetChannelFromChannelIdFail() {
  EdgeAppLibSensorFrameGetChannelFromChannelIdSuccess = -1;
}
void resetEdgeAppLibSensorFrameGetChannelFromChannelIdSuccess() {
  EdgeAppLibSensorFrameGetChannelFromChannelIdSuccess = 0;
}
void resetEdgeAppLibSensorFrameGetChannelFromChannelIdCalled() {
  EdgeAppLibSensorFrameGetChannelFromChannelIdCalled = 0;
}

int wasEdgeAppLibSensorChannelGetRawDataCalled() {
  return EdgeAppLibSensorChannelGetRawDataCalled;
}
void setEdgeAppLibSensorChannelGetRawDataFail() {
  EdgeAppLibSensorChannelGetRawDataSuccess = -1;
}
void resetEdgeAppLibSensorChannelGetRawDataSuccess() {
  EdgeAppLibSensorChannelGetRawDataSuccess = 0;
}
void resetEdgeAppLibSensorChannelGetRawDataCalled() {
  EdgeAppLibSensorChannelGetRawDataCalled = 0;
}

int wasEdgeAppLibSensorChannelGetPropertyCalled() {
  return EdgeAppLibSensorChannelGetPropertyCalled;
}
void setEdgeAppLibSensorChannelGetPropertyFail() {
  EdgeAppLibSensorChannelGetPropertySuccess = -1;
}
void resetEdgeAppLibSensorChannelGetPropertySuccess() {
  EdgeAppLibSensorChannelGetPropertySuccess = 0;
}
void resetEdgeAppLibSensorChannelGetPropertyCalled() {
  EdgeAppLibSensorChannelGetPropertyCalled = 0;
}

void setEdgeAppLibSensorChannelSubFrameCurrentNum(int num) {
  EdgeAppLibSensorChannelSubFrameCurrentNum = num;
}
void setEdgeAppLibSensorChannelSubFrameDivisionNum(int num) {
  EdgeAppLibSensorChannelSubFrameDivisionNum = num;
}

int wasEdgeAppLibSensorGetLastErrorStringCalled() {
  return EdgeAppLibSensorGetLastErrorStringCalled;
}
void setEdgeAppLibSensorGetLastErrorStringFail() {
  EdgeAppLibSensorGetLastErrorStringSuccess = -1;
}
void resetEdgeAppLibSensorGetLastErrorStringSuccess() {
  EdgeAppLibSensorGetLastErrorStringSuccess = 0;
}
void resetEdgeAppLibSensorGetLastErrorStringCalled() {
  EdgeAppLibSensorGetLastErrorStringCalled = 0;
}

int wasEdgeAppLibSensorGetLastErrorLevelCalled() {
  return EdgeAppLibSensorGetLastErrorLevelCalled;
}
void setEdgeAppLibSensorGetLastErrorLevelFail() {
  EdgeAppLibSensorGetLastErrorLevelSuccess = AITRIOS_SENSOR_LEVEL_FAIL;
}
void resetEdgeAppLibSensorGetLastErrorLevelSuccess() {
  EdgeAppLibSensorGetLastErrorLevelSuccess = AITRIOS_SENSOR_LEVEL_UNDEFINED;
}
void resetEdgeAppLibSensorGetLastErrorLevelCalled() {
  EdgeAppLibSensorGetLastErrorLevelCalled = 0;
}

int wasEdgeAppLibSensorGetLastErrorCauseCalled() {
  return EdgeAppLibSensorGetLastErrorCauseCalled;
}
void setEdgeAppLibSensorGetLastErrorCauseFail() {
  EdgeAppLibSensorGetLastErrorCauseSuccess = AITRIOS_SENSOR_ERROR_UNKNOWN;
}
void setEdgeAppLibSensorGetLastErrorCauseFail2(
    EdgeAppLibSensorErrorCause cause) {
  EdgeAppLibSensorGetLastErrorCauseSuccess = cause;
}
void resetEdgeAppLibSensorGetLastErrorCauseSuccess() {
  EdgeAppLibSensorGetLastErrorCauseSuccess = AITRIOS_SENSOR_ERROR_NONE;
}
void resetEdgeAppLibSensorGetLastErrorCauseCalled() {
  EdgeAppLibSensorGetLastErrorCauseCalled = 0;
}
void setEdgeAppLibSensorInputDataTypeEnableChannelFail() {
  EdgeAppLibSensorInputDataTypeEnableChannelSuccess = -1;
}
void resetEdgeAppLibSensorInputDataTypeEnableChannelSuccess() {
  EdgeAppLibSensorInputDataTypeEnableChannelSuccess = 0;
}

EdgeAppLibSensorErrorCause EdgeAppLibLogSensorError() {
  return EdgeAppLib::SensorGetLastErrorCause();
}

void setEdgeAppLibSensorChannelImageProperty(
    EdgeAppLibSensorImageProperty property) {
  EdgeAppLibSensorChannelImageProperty.width = property.width;
  EdgeAppLibSensorChannelImageProperty.height = property.height;
  EdgeAppLibSensorChannelImageProperty.stride_bytes = property.stride_bytes;
  snprintf(EdgeAppLibSensorChannelImageProperty.pixel_format,
           AITRIOS_SENSOR_PIXEL_FORMAT_LENGTH, "%s", property.pixel_format);
}

void resetEdgeAppLibSensorChannelImageProperty() {
  EdgeAppLibSensorChannelImageProperty.width = 5;
  EdgeAppLibSensorChannelImageProperty.height = 1;
  EdgeAppLibSensorChannelImageProperty.stride_bytes = 5 * 3;
  snprintf(EdgeAppLibSensorChannelImageProperty.pixel_format,
           sizeof(EdgeAppLibSensorChannelImageProperty.pixel_format), "%s",
           AITRIOS_SENSOR_PIXEL_FORMAT_RGB24);
}
