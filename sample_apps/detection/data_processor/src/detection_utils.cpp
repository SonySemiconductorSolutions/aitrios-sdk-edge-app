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

#include "detection_utils.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

#include "data_processor_api.hpp"
#include "data_processor_utils.hpp"
#include "log.h"
#include "parson.h"

DataProcessorResultCode ExtractMaxDetections(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "max_detections", &aux) == 0) {
    ssd_param_pr->max_detections = (uint16_t)aux;
    return kDataProcessorOk;
  }
  ssd_param_pr->max_detections = DEFAULT_MAX_DETECTIONS;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'max_detections' parameter "
      "is %d",
      DEFAULT_MAX_DETECTIONS);
  json_object_set_number(json, "max_detections", DEFAULT_MAX_DETECTIONS);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractThreshold(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "threshold", &aux) == 0) {
    ssd_param_pr->threshold = (float)aux;
    return kDataProcessorOk;
  }
  ssd_param_pr->threshold = DEFAULT_THRESHOLD;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'threshold' parameter "
      "is %f",
      DEFAULT_THRESHOLD);
  json_object_set_number(json, "threshold", DEFAULT_THRESHOLD);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_height", &aux) == 0) {
    ssd_param_pr->input_height = (uint16_t)aux;
    return kDataProcessorOk;
  }
  ssd_param_pr->input_height = DEFAULT_SSD_INPUT_TENSOR_HEIGHT;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_height' parameter "
      "is %d",
      DEFAULT_SSD_INPUT_TENSOR_HEIGHT);
  json_object_set_number(json, "input_height", DEFAULT_SSD_INPUT_TENSOR_HEIGHT);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode VerifyConstraints(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr) {
  if (ssd_param_pr->threshold < 0.0 || ssd_param_pr->threshold > 1.0) {
    LOG_WARN("threshold value out of range, set to default threshold");
    ssd_param_pr->threshold = DEFAULT_THRESHOLD;
    json_object_set_number(json, "threshold", ssd_param_pr->threshold);
    return kDataProcessorInvalidParam;
  }
  return kDataProcessorOk;
}

DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_width", &aux) == 0) {
    ssd_param_pr->input_width = (uint16_t)aux;
    return kDataProcessorOk;
  }
  ssd_param_pr->input_width = DEFAULT_SSD_INPUT_TENSOR_WIDTH;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_width' parameter "
      "is %d",
      DEFAULT_SSD_INPUT_TENSOR_WIDTH);
  json_object_set_number(json, "input_width", DEFAULT_SSD_INPUT_TENSOR_WIDTH);
  return kDataProcessorInvalidParam;
}

int ExtractNumberOfDetections(int num_elements) {
  int detections_in_tensor = (num_elements - 1) / 6;
  return detections_in_tensor;
}

static JSON_Value *ExtractBboxToJson(float *out_data_pr, int index,
                                     int num_of_detections,
                                     DataProcessorCustomParam ssd_param) {
  /* Extract bounding box co-ordinates */
  uint16_t ymin =
      (uint16_t)(round((out_data_pr[index]) * (ssd_param.input_height - 1)));
  uint16_t xmin =
      (uint16_t)(round((out_data_pr[index + (1 * num_of_detections)]) *
                       (ssd_param.input_width - 1)));
  uint16_t ymax =
      (uint16_t)(round((out_data_pr[index + (2 * num_of_detections)]) *
                       (ssd_param.input_height - 1)));
  uint16_t xmax =
      (uint16_t)(round((out_data_pr[index + (3 * num_of_detections)]) *
                       (ssd_param.input_width - 1)));

  LOG_DBG("left = %d, top = %d, right = %d, bottom = %d", xmin, ymin, xmax,
          ymax);

  JSON_Value *bbox_value = json_value_init_object();
  JSON_Object *bbox = json_object(bbox_value);
  json_object_set_number(bbox, "left", xmin);
  json_object_set_number(bbox, "top", ymin);
  json_object_set_number(bbox, "right", xmax);
  json_object_set_number(bbox, "bottom", ymax);
  return bbox_value;
}

static JSON_Value *ExtractDetectionsToJson(float *out_data_pr,
                                           int num_of_detections,
                                           DataProcessorCustomParam ssd_param) {
  JSON_Value *detections_value = json_value_init_array();
  JSON_Array *detections = json_array(detections_value);

  uint8_t detections_above_threshold = 0;

  for (uint8_t i = 0; i < num_of_detections; i++) {
    /* Extract score */
    float score;
    score = out_data_pr[(num_of_detections * 5) + i];
    /* Filter Detections */
    if (score < ssd_param.threshold) {
      LOG_DBG("Ignored detection because of threshold");
      continue;
    } else {
      detections_above_threshold++;

      /* Extract class index */
      uint8_t class_index = out_data_pr[(num_of_detections * 4) + i];

      /* Extract bounding box co-ordinates */
      JSON_Value *bbox =
          ExtractBboxToJson(out_data_pr, i, num_of_detections, ssd_param);

      LOG_DBG("class = %d, score = %f", class_index, score);

      JSON_Value *detection_value = json_value_init_object();
      JSON_Object *detection = json_object(detection_value);
      json_object_set_number(detection, "class_id", class_index);
      json_object_set_number(detection, "score", score);
      json_object_set_value(detection, "bbox", bbox);

      json_array_append_value(detections, detection_value);
    }
    if (detections_above_threshold >= ssd_param.max_detections) {
      LOG_DBG(
          "Maximum number of detections reached, stopping to process more "
          "detections");
      break;
    }
  }

  return detections_value;
}

JSON_Value *CreateSSDOutputJson(float *out_data_pr, int num_elements,
                                DataProcessorCustomParam ssd_param) {
  LOG_DBG("Creating JSON from array of floats");

  /* Extract number of detections */
  int num_of_detections = ExtractNumberOfDetections(num_elements);

  return ExtractDetectionsToJson(out_data_pr, num_of_detections, ssd_param);
}

void ExtractBboxToFlatbuffer(
    float *out_data_pr, int index, int num_of_detections,
    flatbuffers::FlatBufferBuilder *builder,
    flatbuffers::Offset<SmartCamera::BoundingBox2d> *bbox_data,
    DataProcessorCustomParam ssd_param) {
  /* Extract bounding box co-ordinates */
  uint16_t ymin =
      (uint16_t)(round((out_data_pr[index]) * (ssd_param.input_height - 1)));
  uint16_t xmin =
      (uint16_t)(round((out_data_pr[index + (1 * num_of_detections)]) *
                       (ssd_param.input_width - 1)));
  uint16_t ymax =
      (uint16_t)(round((out_data_pr[index + (2 * num_of_detections)]) *
                       (ssd_param.input_height - 1)));
  uint16_t xmax =
      (uint16_t)(round((out_data_pr[index + (3 * num_of_detections)]) *
                       (ssd_param.input_width - 1)));

  LOG_DBG("left = %d, top = %d, right = %d, bottom = %d", xmin, ymin, xmax,
          ymax);

  *bbox_data =
      SmartCamera::CreateBoundingBox2d(*builder, xmin, ymin, xmax, ymax);
}

void ExtractDetectionsToFlatbufferObject(
    float *out_data_pr,
    std::vector<flatbuffers::Offset<SmartCamera::GeneralObject> > *gdata_vector,
    flatbuffers::FlatBufferBuilder *builder, int num_of_detections,
    DataProcessorCustomParam ssd_param) {
  uint8_t detections_above_threshold = 0;

  for (uint8_t i = 0; i < num_of_detections; i++) {
    /* Extract score */
    float score;
    score = out_data_pr[(num_of_detections * 5) + i];
    /* Filter Detections */
    if (score < ssd_param.threshold) {
      LOG_DBG("Ignored detection because of threshold");
      continue;
    } else {
      detections_above_threshold++;
      flatbuffers::Offset<SmartCamera::BoundingBox2d> bbox_data;

      /* Extract class index */
      uint8_t class_index = out_data_pr[(num_of_detections * 4) + i];

      /* Extract bounding box co-ordinates */
      ExtractBboxToFlatbuffer(out_data_pr, i, num_of_detections, builder,
                              &bbox_data, ssd_param);

      LOG_DBG("class = %d, score = %f", class_index, score);

      auto general_data = SmartCamera::CreateGeneralObject(
          *builder, class_index, SmartCamera::BoundingBox_BoundingBox2d,
          bbox_data.Union(), score);

      gdata_vector->push_back(general_data);
    }
    if (detections_above_threshold >= ssd_param.max_detections) {
      LOG_DBG(
          "Maximum number of detections reached, stopping to process more "
          "detections");
      break;
    }
  }
}

int CreateSSDOutputFlatbuffer(float *out_data_pr, int num_elements,
                              flatbuffers::FlatBufferBuilder *builder,
                              DataProcessorCustomParam ssd_param) {
  std::vector<flatbuffers::Offset<SmartCamera::GeneralObject> > gdata_vector;
  LOG_DBG("Creating flatbuffer from array of floats");

  /* Extract number of detections */
  int num_of_detections = ExtractNumberOfDetections(num_elements);

  ExtractDetectionsToFlatbufferObject(out_data_pr, &gdata_vector, builder,
                                      num_of_detections, ssd_param);

  auto out_data = SmartCamera::CreateObjectDetectionTop(
      *builder, SmartCamera::CreateObjectDetectionData(
                    *builder, builder->CreateVector(gdata_vector)));

  builder->Finish(out_data);

  return 0;
}
