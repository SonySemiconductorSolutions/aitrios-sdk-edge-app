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

#ifndef SEGMENTATION_UTILS_H
#define SEGMENTATION_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "data_processor_api.hpp"
#include "flatbuffers/flatbuffers.h"
#include "parson.h"

/* -------------------------------------------------------- */
/* define                                                   */
/* -------------------------------------------------------- */
#ifndef DEFAULT_SS_INPUT_TENSOR_WIDTH
#define DEFAULT_SS_INPUT_TENSOR_WIDTH (125)
#endif

#ifndef DEFAULT_SS_INPUT_TENSOR_HEIGHT
#define DEFAULT_SS_INPUT_TENSOR_HEIGHT (125)
#endif

#ifndef DEFAULT_THRESHOLD
#define DEFAULT_THRESHOLD (0.3)
#endif

typedef struct DataProcessorCustomParam {
  float threshold;
  uint16_t inputWidth;
  uint16_t inputHeight;
} DataProcessorCustomParam;

/**
 * @brief Create a Segmentation Data object
 *
 * @param data_body_str A pointer to a floating-point array. Flattened array
 * where each element corresponds to a class ID for a pixel in a semantic
 * segmentaiton task.
 * @param num_elements Number of elements in the float array
 * @param builder Object to build the flatbuffer
 * @param seg_param Struct that contains the necessary parameters
 * @return DataProcessorResultCode
 *
 * The function populates segmentation_output->classes, which is a 2D vector
 * structure. Each sub-vector represents a row of the segmentation data, with
 * each element in a sub-vector representing the class ID.
 * The function effectively converts the 1D input array into a structured 2D
 * format.
 */

int CreateSegmentationFlatbuffer(float *out_data_pr, int num_elements,
                                 flatbuffers::FlatBufferBuilder *builder,
                                 DataProcessorCustomParam seg_param);

DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *seg_param_pr);

DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *seg_param_pr);

#endif
