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
#define DEFAULT_SSD_INPUT_TENSOR_WIDTH \
  (320)  // Derived from Custom Vision Object Detection Model on AITRIOS
         // Console
#define DEFAULT_SSD_INPUT_TENSOR_HEIGHT \
  (320)  // Derived from Custom Vision Object Detection Model on AITRIOS
         // Console
#define DEFAULT_THRESHOLD (0.3)
#define DEFAULT_MAX_DETECTIONS (10)

typedef struct {
  uint16_t max_detections;
  float threshold;
  uint16_t input_width;
  uint16_t input_height;
} DataProcessorCustomParam;

JSON_Value *CreateSSDOutputJson(float *out_data_pr, int num_elements,
                                DataProcessorCustomParam ssd_param);

int CreateSSDOutputFlatbuffer(float *out_data_pr, int num_elements,
                              flatbuffers::FlatBufferBuilder *builder,
                              DataProcessorCustomParam ssd_param);
void ExtractDetectionsToFlatbufferObject(
    float *out_data_pr,
    std::vector<flatbuffers::Offset<SmartCamera::GeneralObject> > *gdata_vector,
    flatbuffers::FlatBufferBuilder *builder, int num_of_detections,
    DataProcessorCustomParam ssd_param);

void ExtractBboxToFlatbuffer(
    float *out_data_pr, int index, int num_of_detections,
    flatbuffers::FlatBufferBuilder *builder,
    flatbuffers::Offset<SmartCamera::BoundingBox2d> *bbox_data,
    DataProcessorCustomParam ssd_param);

int ExtractNumberOfDetections(int num_elements);

DataProcessorResultCode ExtractMaxDetections(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr);
DataProcessorResultCode ExtractThreshold(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr);
DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr);
DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr);
DataProcessorResultCode VerifyConstraints(
    JSON_Object *json, DataProcessorCustomParam *ssd_param_pr);

#endif  // DETECTION_UTILS_H
