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

#ifndef CLASSIFICATION_UTILS_HPP
#define CLASSIFICATION_UTILS_HPP

#include "data_processor_api.hpp"
#include "flatbuffers/flatbuffers.h"
#include "parson.h"

/* -------------------------------------------------------- */
/* define                                                   */
/* -------------------------------------------------------- */
#define DEFAULT_ID_VERSION \
  "00.01.01.00"  // Format: "AA.XX.YY.ZZ" where AA: ID, XX.YY.ZZ: Version

#ifndef DEFAULT_MAX_PREDICTIONS
#define DEFAULT_MAX_PREDICTIONS (3)
#endif

typedef struct tagDataProcessorCustomParam {
  uint16_t maxPredictions;
} DataProcessorCustomParam;

/* -------------------------------------------------------- */
/* structure                                                */
/* -------------------------------------------------------- */

typedef struct tagClassificationItem {
  int index = 0;
  float score = 0;
} ClassificationItem;

DataProcessorResultCode ExtractMaxPredictions(
    JSON_Object *json, DataProcessorCustomParam *cls_param_pr);

int CreateClassificationFlatbuffer(float *out_data_pr, int num_elements,
                                   flatbuffers::FlatBufferBuilder *builder,
                                   DataProcessorCustomParam cls_param_pr);

#endif  // CLASSIFICATION_UTILS_HPP
