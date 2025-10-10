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

#include "classification_utils.hpp"

#include "classification_generated.h"
#include "data_processor_api.hpp"
#include "data_processor_utils.hpp"
#include "flatbuffers/flatbuffers.h"
#include "log.h"
#include "parson.h"

DataProcessorResultCode ExtractMaxPredictions(
    JSON_Object *json, DataProcessorCustomParam *cls_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "max_predictions", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    uint16_t max_predictions = (uint16_t)aux;
    cls_param_pr->maxPredictions = max_predictions;
    return kDataProcessorOk;
  }
  LOG_INFO(
      "DataProcessorConfigure: default value of 'max_predictions' parameter "
      "is %d",
      DEFAULT_MAX_PREDICTIONS);
  cls_param_pr->maxPredictions = DEFAULT_MAX_PREDICTIONS;
  json_object_set_number(json, "max_predictions", DEFAULT_MAX_PREDICTIONS);
  return kDataProcessorInvalidParam;
}

static void top_n_indexes(const float *data, int ndata, int n,
                          std::vector<int> *v) {
  assert(n <= ndata);
  assert(v->empty());
  v->reserve(n);
  for (int i = 0; i < ndata; i++) {
    int j;
    for (j = 0; j < v->size(); j++) {
      if (data[i] > data[(*v)[j]]) {
        break;
      }
    }
    if (j < n) {
      if (v->size() == n) {
        v->pop_back();
      }
      v->insert(v->begin() + j, i);
    }
  }
  assert(n == v->size());
}

JSON_Value *CreateClsOutputJson(float *out_data_pr, uint16_t num_elements,
                                DataProcessorCustomParam cls_param) {
  LOG_DBG("Creating JSON from array of floats.");

  std::vector<ClassificationItem> class_data(num_elements);

  for (int i = 0; i < num_elements; i++) {
    class_data[i] = {i, out_data_pr[i]};
  }

  if (class_data.size() > 0) {
    std::stable_sort(
        class_data.begin(), class_data.end(),
        [](const ClassificationItem &left, const ClassificationItem &right) {
          return left.score > right.score;
        });
  }

  JSON_Value *classifications_value = json_value_init_array();
  JSON_Array *classifications = json_array(classifications_value);

  if (num_elements > cls_param.maxPredictions) {
    num_elements = cls_param.maxPredictions;
    LOG_DBG("Maximum number of predictions to send %d.", num_elements);
  }

  for (int i = 0; i < num_elements; i++) {
    LOG_DBG("class = %d, score = %f", class_data[i].index, class_data[i].score);
    JSON_Value *classification_value = json_value_init_object();
    JSON_Object *classification = json_object(classification_value);
    json_object_set_number(classification, "class_id", class_data[i].index);
    json_object_set_number(classification, "score", class_data[i].score);
    json_array_append_value(classifications, classification_value);
  }
  return classifications_value;
}

int CreateClassificationFlatbuffer(float *out_data_pr, int num_elements,
                                   flatbuffers::FlatBufferBuilder *builder,
                                   DataProcessorCustomParam cls_param) {
  LOG_DBG("Creating flatbuffer from array of floats");

  if (out_data_pr == NULL) {
    LOG_ERR("No data to create the flatbuffer");
    return -1;
  }

  int nresults = num_elements;
  if (nresults > cls_param.maxPredictions) {
    nresults = cls_param.maxPredictions;
    LOG_DBG("Maximum number of predictions to send %d", nresults);
  }

  std::vector<int> results;
  top_n_indexes(out_data_pr, num_elements, nresults, &results);

  std::vector<flatbuffers::Offset<SmartCamera::GeneralClassification>>
      gdata_vector;

  for (int i = 0; i < results.size(); i++) {
    LOG_DBG("class = %d, score = %f", results[i], out_data_pr[results[i]]);
    auto general_data = SmartCamera::CreateGeneralClassification(
        *builder, results[i], out_data_pr[results[i]]);
    gdata_vector.push_back(general_data);
  }

  auto v_bbox = builder->CreateVector(gdata_vector);
  auto class_data_fb = SmartCamera::CreateClassificationData(*builder, v_bbox);
  auto out_data = SmartCamera::CreateClassificationTop(*builder, class_data_fb);
  builder->Finish(out_data);
  return 0;
}
