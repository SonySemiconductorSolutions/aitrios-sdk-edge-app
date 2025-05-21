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

#include "segmentation_utils.hpp"

#include <edgeapp/log.h>
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
#include "parson.h"
#include "semantic_segmentation_generated.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *seg_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_height", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    seg_param_pr->inputHeight = (uint16_t)aux;
    return kDataProcessorOk;
  }
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_height' parameter "
      "is %d",
      DEFAULT_SS_INPUT_TENSOR_HEIGHT);
  seg_param_pr->inputHeight = DEFAULT_SS_INPUT_TENSOR_HEIGHT;
  json_object_set_number(json, "input_height", DEFAULT_SS_INPUT_TENSOR_HEIGHT);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *seg_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_width", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    seg_param_pr->inputWidth = (uint16_t)aux;
    return kDataProcessorOk;
  }
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_width' parameter "
      "is %d",
      DEFAULT_SS_INPUT_TENSOR_WIDTH);
  seg_param_pr->inputWidth = DEFAULT_SS_INPUT_TENSOR_WIDTH;
  json_object_set_number(json, "input_width", DEFAULT_SS_INPUT_TENSOR_WIDTH);
  return kDataProcessorInvalidParam;
}

int CreateSegmentationFlatbuffer(float *out_data_pr, int num_elements,
                                 flatbuffers::FlatBufferBuilder *builder,
                                 DataProcessorCustomParam seg_param) {
  LOG_DBG("Creating flatbuffer from aray of floats");

  LOG_DBG("Height: %d, Width: %d", seg_param.inputHeight, seg_param.inputWidth);

  if (num_elements != seg_param.inputHeight * seg_param.inputWidth) {
    LOG_ERR("Invalid num_elements");
    return 1;
  }
  uint16_t *class_id_map_array =
      (uint16_t *)malloc(num_elements * sizeof(uint16_t));
  if (class_id_map_array == NULL) {
    LOG_ERR("Failed to malloc for class_id_map_array");
    return 1;
  }

  // NOTE: contains value with highest classid. Assumming 1 <= class id <= N
  // moving from column layout to row layout
  int num_classes = 0;
  for (int i = 0; i < seg_param.inputHeight; i++) {
    for (int j = 0; j < seg_param.inputWidth; j++) {
      uint16_t class_id = (uint16_t)out_data_pr[j * seg_param.inputHeight + i];
      class_id_map_array[i * seg_param.inputWidth + j] = class_id;
      num_classes = MAX(num_classes, class_id);
    }
  }

  flatbuffers::Offset<flatbuffers::Vector<uint16_t>> class_id_map =
      builder->CreateVector(class_id_map_array, num_elements);

  // NOTE: network does not contains classes
  flatbuffers::Offset<flatbuffers::Vector<float>> score_map(0);

  auto out_data = SmartCamera::CreateSemanticSegmentationData(
      *builder, seg_param.inputHeight, seg_param.inputWidth, class_id_map,
      num_classes, score_map);

  auto out_top = SmartCamera::CreateSemanticSegmentationTop(*builder, out_data);

  builder->Finish(out_top);
  free(class_id_map_array);
  return 0;
}
