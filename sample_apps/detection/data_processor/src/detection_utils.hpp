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

#ifndef DETECTION_UTILS_H
#define DETECTION_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "data_processor_api.hpp"
#include "flatbuffers/flatbuffers.h"
#include "objectdetection_generated.h"
#include "parson.h"

/* -------------------------------------------------------- */
/* define                                                   */
/* -------------------------------------------------------- */
#define DEFAULT_INPUT_TENSOR_WIDTH \
  (320)  // Derived from Custom Vision Object Detection Model on AITRIOS
         // Console
#define DEFAULT_INPUT_TENSOR_HEIGHT \
  (320)  // Derived from Custom Vision Object Detection Model on AITRIOS
         // Console
#define DEFAULT_THRESHOLD (0.3)
#define DEFAULT_MAX_DETECTIONS (10)
#define DEFAULT_BBOX_ORDER "yxyx"
#define DEFAULT_BBOX_NORMALIZED true
#define DEFAULT_CLASS_SCORE_ORDER "cls_score"
#define DEFAULT_OUTPUT_FORMAT EdgeAppLibSendDataBase64
#define CLASS_IDS_SIZE (10)
#define BBOX_ORDER_SIZE (5)
#define CLS_SCORE_SIZE (10)

typedef struct {
  uint16_t max_detections;
  float threshold;
  uint16_t input_width;
  uint16_t input_height;
  char bbox_order[BBOX_ORDER_SIZE];
  bool bbox_normalized;
  char class_score_order[CLS_SCORE_SIZE];
} DataProcessorCustomParam;

typedef enum {
  BBOX_XYWH,
  BBOX_XYXY,
  BBOX_XXYY,
  BBOX_YXYX,
  BBOX_UNKNOWN
} BBoxOrder;

typedef struct {
  uint16_t left;
  uint16_t top;
  uint16_t right;
  uint16_t bottom;
} BBox;

typedef struct {
  uint16_t class_id;
  float score;
  BBox bbox;
} DetectionData;

typedef struct {
  uint16_t num_detections;
  DetectionData *detection_data;
} Detections;

typedef struct {
  BBox coordinates;
  float overlap;
  uint16_t class_ids[CLASS_IDS_SIZE];
  size_t num_of_class;
} Area;

typedef struct {
  uint16_t class_id;
  uint16_t count;
} AreaCount;

Detections *CreateDetections(float *in_data, uint32_t in_size,
                             DataProcessorCustomParam detection_param);

AreaCount *CreateAreaCount(Detections **detections, Area area);

void FilterByParams(Detections **detections,
                    DataProcessorCustomParam detection_param);

JSON_Value *MakeDetectionJson(Detections *detections);

JSON_Value *MakeAreaJson(Detections *detections, AreaCount *area_count,
                         size_t num_of_class);

DataProcessorResultCode MakeDetectionFlatbuffer(
    Detections *detections, flatbuffers::FlatBufferBuilder *builder);

DataProcessorResultCode MakeAreaFlatbuffer(
    Detections *detections, AreaCount *area_count,
    flatbuffers::FlatBufferBuilder *builder, size_t num_of_class);

void ExtractDetectionsToFlatbufferObject(
    float *out_data_pr,
    std::vector<flatbuffers::Offset<SmartCamera::GeneralObject> > *gdata_vector,
    flatbuffers::FlatBufferBuilder *builder, uint16_t num_of_detections,
    DataProcessorCustomParam detection_param);

void ExtractBboxToFlatbuffer(
    float *out_data_pr, int index, uint16_t num_of_detections,
    flatbuffers::FlatBufferBuilder *builder,
    flatbuffers::Offset<SmartCamera::BoundingBox2d> *bbox_data,
    DataProcessorCustomParam detection_param);

uint16_t ExtractNumberOfDetections(uint32_t num_elements);

DataProcessorResultCode ExtractMaxDetections(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);
DataProcessorResultCode ExtractThreshold(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);
DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);
DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);
DataProcessorResultCode VerifyConstraints(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);
DataProcessorResultCode ExtractBboxOrder(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);
DataProcessorResultCode ExtractBboxNorm(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);
DataProcessorResultCode ExtractClassOrder(
    JSON_Object *json, DataProcessorCustomParam *detection_pram_pr);

DataProcessorResultCode ExtractMetadataSettings(
    JSON_Object *json, EdgeAppLibSendDataType *out_format);

#endif  // DETECTION_UTILS_H
