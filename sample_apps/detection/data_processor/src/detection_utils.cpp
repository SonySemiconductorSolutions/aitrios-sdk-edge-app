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

#define MAX_DETECTION_DATA_SIZE (sizeof(DetectionData) * UINT16_MAX)

DataProcessorResultCode ExtractMaxDetections(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "max_detections", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    detection_param_pr->max_detections = (uint16_t)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->max_detections = DEFAULT_MAX_DETECTIONS;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'max_detections' parameter "
      "is %d",
      DEFAULT_MAX_DETECTIONS);
  json_object_set_number(json, "max_detections", DEFAULT_MAX_DETECTIONS);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractBboxNorm(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  bool aux = false;
  if (GetValueBoolean(json, "bbox_normalization", &aux) == 0) {
    detection_param_pr->bbox_normalized = aux;
    return kDataProcessorOk;
  }
  // Default value
  detection_param_pr->bbox_normalized = true;
  return kDataProcessorOk;
}

DataProcessorResultCode ExtractBboxOrder(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  char aux[BBOX_ORDER_SIZE] = {0};
  memset(detection_param_pr->bbox_order, 0, BBOX_ORDER_SIZE);
  if (GetValueString(json, "bbox_order", aux) == 0) {
    if (strlen(aux) >= BBOX_ORDER_SIZE) {
      LOG_ERR("Bbox Order name is too long");
      return kDataProcessorOutOfRange;
    }
    strncpy(detection_param_pr->bbox_order, aux, strlen(aux));
    return kDataProcessorOk;
  }
  // Default value
  strncpy(detection_param_pr->bbox_order, "yxyx", 4);
  return kDataProcessorOk;
}

DataProcessorResultCode ExtractClassOrder(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  char aux[CLS_SCORE_SIZE] = {0};
  memset(detection_param_pr->class_score_order, 0, CLS_SCORE_SIZE);
  if (GetValueString(json, "class_score_order", aux) == 0) {
    if (strlen(aux) >= CLS_SCORE_SIZE) {
      LOG_ERR("Class Score order name is too long");
      return kDataProcessorOutOfRange;
    }
    strncpy(detection_param_pr->class_score_order, aux, strlen(aux));
    LOG_INFO("DataProcessorConfigure: class_score_order is %s",
             detection_param_pr->class_score_order);
    return kDataProcessorOk;
  }
  // Default value
  strncpy(detection_param_pr->class_score_order, "cls_score", 9);
  return kDataProcessorOk;
}

DataProcessorResultCode ExtractThreshold(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "threshold", &aux) == 0) {
    if (aux < 0.0 || aux > 1.0) {
      LOG_INFO("DataProcessorConfigure: threshold value out of range");
      return kDataProcessorOutOfRange;
    }
    detection_param_pr->threshold = (float)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->threshold = DEFAULT_THRESHOLD;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'threshold' parameter "
      "is %f",
      DEFAULT_THRESHOLD);
  json_object_set_number(json, "threshold", DEFAULT_THRESHOLD);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_height", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    detection_param_pr->input_height = (uint16_t)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->input_height = DEFAULT_INPUT_TENSOR_HEIGHT;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_height' parameter "
      "is %d",
      DEFAULT_INPUT_TENSOR_HEIGHT);
  json_object_set_number(json, "input_height", DEFAULT_INPUT_TENSOR_HEIGHT);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode VerifyConstraints(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  if (detection_param_pr->threshold < 0.0 ||
      detection_param_pr->threshold > 1.0) {
    LOG_WARN("threshold value out of range, set to default threshold");
    detection_param_pr->threshold = DEFAULT_THRESHOLD;
    json_object_set_number(json, "threshold", detection_param_pr->threshold);
    return kDataProcessorInvalidParam;
  }
  return kDataProcessorOk;
}

DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_width", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    detection_param_pr->input_width = (uint16_t)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->input_width = DEFAULT_INPUT_TENSOR_WIDTH;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_width' parameter "
      "is %d",
      DEFAULT_INPUT_TENSOR_WIDTH);
  json_object_set_number(json, "input_width", DEFAULT_INPUT_TENSOR_WIDTH);
  return kDataProcessorInvalidParam;
}

uint16_t ExtractNumberOfDetections(uint32_t num_elements) {
  if (num_elements < 7) {
    return 0;
  }
  uint16_t detections_in_tensor = (num_elements - 1) / 6;
  return detections_in_tensor;
}

static BBoxOrder parse_bbox_order(const char *fmt) {
  if (strncmp(fmt, "xywh", 4) == 0) return BBOX_XYWH;
  if (strncmp(fmt, "xyxy", 4) == 0) return BBOX_XYXY;
  if (strncmp(fmt, "xxyy", 4) == 0) return BBOX_XXYY;
  if (strncmp(fmt, "yxyx", 4) == 0) return BBOX_YXYX;
  return BBOX_UNKNOWN;
}

static inline uint16_t maybe_scale(float value, int dim, bool normalized) {
  return normalized ? (uint16_t)round(value * dim) : (uint16_t)round(value);
}

BBox ExtractBbox(float *in_data, int index, uint16_t num_detections,
                 DataProcessorCustomParam detection_param) {
  BBox bbox = {};
  int idx0 = index;
  int idx1 = index + num_detections;
  int idx2 = index + 2 * num_detections;
  int idx3 = index + 3 * num_detections;

  const int w = detection_param.input_width - 1;
  const int h = detection_param.input_height - 1;

  BBoxOrder order = parse_bbox_order(detection_param.bbox_order);
  bool normalized = detection_param.bbox_normalized;

  switch (order) {
    case BBOX_XYXY:
      // [xmin, ymin, xmax, ymax]
      bbox.left = maybe_scale(in_data[idx0], w, normalized);
      bbox.top = maybe_scale(in_data[idx1], h, normalized);
      bbox.right = maybe_scale(in_data[idx2], w, normalized);
      bbox.bottom = maybe_scale(in_data[idx3], h, normalized);
      break;

    case BBOX_XXYY:
      // [xmin, xmax, ymin, ymax]
      bbox.left = maybe_scale(in_data[idx0], w, normalized);
      bbox.right = maybe_scale(in_data[idx1], w, normalized);
      bbox.top = maybe_scale(in_data[idx2], h, normalized);
      bbox.bottom = maybe_scale(in_data[idx3], h, normalized);
      break;

    case BBOX_XYWH:
      // [xmin, ymin, width, height]
      bbox.left = maybe_scale(in_data[idx0], w, normalized);
      bbox.top = maybe_scale(in_data[idx1], h, normalized);
      bbox.right = bbox.left + maybe_scale(in_data[idx2], w, normalized);
      bbox.bottom = bbox.top + maybe_scale(in_data[idx3], h, normalized);
      break;

    case BBOX_YXYX:
      // [ymin, xmin, ymax, xmax]
      bbox.top = maybe_scale(in_data[idx0], h, normalized);
      bbox.left = maybe_scale(in_data[idx1], w, normalized);
      bbox.bottom = maybe_scale(in_data[idx2], h, normalized);
      bbox.right = maybe_scale(in_data[idx3], w, normalized);
      break;

    case BBOX_UNKNOWN:
    default:
      LOG_ERR("Unknown bbox order: %s", detection_param.bbox_order);
      break;
  }

  LOG_DBG("left = %d, top = %d, right = %d, bottom = %d", bbox.left, bbox.top,
          bbox.right, bbox.bottom);
  return bbox;
}

Detections *CreateDetections(float *in_data, uint32_t in_size,
                             DataProcessorCustomParam detection_param) {
  Detections *detections = (Detections *)malloc(sizeof(Detections));
  if (detections == NULL) {
    LOG_ERR("Failed to allocate memory for detections.");
    return detections;
  }

  uint32_t num_elements = in_size / sizeof(float);
  /* Extract number of detections */
  detections->num_detections = ExtractNumberOfDetections(num_elements);

  size_t detection_data_size =
      detections->num_detections * sizeof(DetectionData);
  if (detection_data_size > MAX_DETECTION_DATA_SIZE) {
    LOG_ERR("Memory size (%zu) exceeds the maximum allowed size (%zu).",
            detection_data_size, MAX_DETECTION_DATA_SIZE);
    free(detections);
    return NULL;
  }

  detections->detection_data = (DetectionData *)malloc(detection_data_size);

  if (detections->detection_data == NULL) {
    LOG_ERR("Failed to allocate memory for detection_data.");
    return detections;
  }

  bool is_score_first = false;
  const char *format = detection_param.class_score_order;
  if (strncmp(format, "score_cls", 9) == 0) {
    is_score_first = true;
  } else {
    is_score_first = false;
  }

  int class_id_base = detections->num_detections * (is_score_first ? 5 : 4);
  int score_base = detections->num_detections * (is_score_first ? 4 : 5);

  for (int i = 0; i < detections->num_detections; i++) {
    detections->detection_data[i].class_id = in_data[class_id_base + i];
    detections->detection_data[i].score = in_data[score_base + i];
    detections->detection_data[i].bbox =
        ExtractBbox(in_data, i, detections->num_detections, detection_param);
  }

  return detections;
}

uint32_t CalculateArea(BBox bbox) {
  return (bbox.right - bbox.left) * (bbox.bottom - bbox.top);
}

BBox CalculateIntersection(BBox bbox1, BBox bbox2) {
  BBox inter;
  inter.left = bbox1.left > bbox2.left ? bbox1.left : bbox2.left;
  inter.top = bbox1.top > bbox2.top ? bbox1.top : bbox2.top;
  inter.right = bbox1.right < bbox2.right ? bbox1.right : bbox2.right;
  inter.bottom = bbox1.bottom < bbox2.bottom ? bbox1.bottom : bbox2.bottom;
  // if there was no intersection
  if (inter.left >= inter.right || inter.top >= inter.bottom) {
    inter.left = inter.top = inter.right = inter.bottom = 0;
  }
  return inter;
}

double CalculateOverlap(BBox bbox, BBox area) {
  BBox inter = CalculateIntersection(bbox, area);
  uint32_t inter_area = CalculateArea(inter);
  return (double)inter_area / CalculateArea(bbox);
}

bool ContainClassId(uint16_t class_id, uint16_t class_id_array[]) {
  for (int i = 0; i < CLASS_IDS_SIZE; i++) {
    if (class_id == class_id_array[i]) return true;
  }
  return false;
}

AreaCount *CreateAreaCount(Detections **detections, Area area) {
  Detections *filtered_detections = (Detections *)malloc(sizeof(Detections));
  if (filtered_detections == NULL) {
    LOG_ERR("Failed to allocate memory for filtered_detections.");
    return NULL;
  }
  filtered_detections->num_detections = 0;
  filtered_detections->detection_data = NULL;

  AreaCount *count_result =
      (AreaCount *)malloc(CLASS_IDS_SIZE * sizeof(AreaCount));

  if (count_result == NULL) {
    LOG_ERR("Failed to allocate memory for count_result.");
    free(filtered_detections);
    return NULL;
  }

  memset(count_result, UINT16_MAX, CLASS_IDS_SIZE * sizeof(AreaCount));

  for (int i = 0; i < (*detections)->num_detections; i++) {
    if (area.num_of_class == 0 ||
        (ContainClassId((*detections)->detection_data[i].class_id,
                        area.class_ids))) {
      LOG_DBG(
          "Class_id (%u) is detected. Calculates the overlap "
          "with area.",
          (*detections)->detection_data[i].class_id);
      if (CalculateOverlap((*detections)->detection_data[i].bbox,
                           area.coordinates) >= area.overlap) {
        LOG_DBG("Class_id (%u) is detected in the area.",
                (*detections)->detection_data[i].class_id);

        // In the case of that class_ids is empty.
        bool found_class_id = false;
        for (int j = 0; j < CLASS_IDS_SIZE; j++) {
          if (count_result[j].class_id ==
              (*detections)->detection_data[i].class_id) {
            count_result[j].count++;
            found_class_id = true;
            break;
          } else if (count_result[j].class_id == UINT16_MAX) {
            count_result[j].class_id =
                (*detections)->detection_data[i].class_id;
            count_result[j].count = 1;
            found_class_id = true;
            break;
          }
        }
        if (!found_class_id) {
          LOG_WARN(
              "Class id = %d was detected in the area but it'll be "
              "ignored because it exceeds the "
              "limitation of the size(=%d).",
              (*detections)->detection_data[i].class_id, CLASS_IDS_SIZE);
        } else {
          // Add detection to filtered_detections
          filtered_detections->num_detections++;
          filtered_detections->detection_data = (DetectionData *)realloc(
              filtered_detections->detection_data,
              filtered_detections->num_detections * sizeof(DetectionData));
          if (filtered_detections->detection_data == NULL) {
            LOG_ERR("Failed to allocate memory for filtered_detections.");
            free(count_result);
            free(filtered_detections);
            return NULL;
          }
          memcpy(&filtered_detections
                      ->detection_data[filtered_detections->num_detections - 1],
                 &(*detections)->detection_data[i], sizeof(DetectionData));
        }
      }
    }
  }
  if (*detections != NULL) {
    free((*detections)->detection_data);
    free(*detections);
  }

  *detections = filtered_detections;

  return count_result;
}

void FilterByParams(Detections **detections,
                    DataProcessorCustomParam detection_param) {
  Detections *filtered_detections = (Detections *)malloc(sizeof(Detections));
  if (filtered_detections == NULL) {
    LOG_ERR("Failed to allocate memory for filtered_detections.");
    return;
  }
  filtered_detections->num_detections = 0;
  filtered_detections->detection_data = NULL;

  uint16_t detections_above_threshold = 0;
  for (int i = 0; i < (*detections)->num_detections; i++) {
    if ((*detections)->detection_data[i].score < detection_param.threshold) {
      LOG_DBG(
          "Ignored detection_data[%u] because score(%f) is lower than the "
          "specified threshold(%f).",
          i, (*detections)->detection_data[i].score, detection_param.threshold);
      continue;
    } else {
      detections_above_threshold++;
      filtered_detections->num_detections++;
      filtered_detections->detection_data = (DetectionData *)realloc(
          filtered_detections->detection_data,
          filtered_detections->num_detections * sizeof(DetectionData));
      if (filtered_detections->detection_data == NULL) {
        LOG_ERR("Failed to allocate memory for filtered_detections.");
        free(filtered_detections);
        return;
      }
      memcpy(&filtered_detections
                  ->detection_data[filtered_detections->num_detections - 1],
             &(*detections)->detection_data[i], sizeof(DetectionData));

      if (detections_above_threshold >= detection_param.max_detections) {
        LOG_DBG(
            "Maximum number of detections reached, stopping to process more "
            "detections");
        break;
      }
    }
  }
  if (*detections != NULL) {
    free((*detections)->detection_data);
    free(*detections);
  }
  *detections = filtered_detections;
  return;
}

JSON_Value *MakeDetectionJson(Detections *detections) {
  LOG_DBG("Creating JSON from Detections.");

  JSON_Value *detections_value = json_value_init_array();
  JSON_Array *detections_array = json_array(detections_value);

  for (uint16_t i = 0; i < detections->num_detections; i++) {
    JSON_Value *bbox_value = json_value_init_object();
    JSON_Object *bbox_obj = json_object(bbox_value);
    json_object_set_number(bbox_obj, "left",
                           detections->detection_data[i].bbox.left);
    json_object_set_number(bbox_obj, "top",
                           detections->detection_data[i].bbox.top);
    json_object_set_number(bbox_obj, "right",
                           detections->detection_data[i].bbox.right);
    json_object_set_number(bbox_obj, "bottom",
                           detections->detection_data[i].bbox.bottom);

    JSON_Value *detection_value = json_value_init_object();
    JSON_Object *detection = json_object(detection_value);
    json_object_set_number(detection, "class_id",
                           detections->detection_data[i].class_id);
    json_object_set_number(detection, "score",
                           detections->detection_data[i].score);
    json_object_set_value(detection, "bounding_box", bbox_value);

    json_array_append_value(detections_array, detection_value);
  }

  return detections_value;
}

JSON_Value *MakeAreaJson(Detections *detections, AreaCount *area_count,
                         size_t num_of_class) {
  LOG_DBG("Creating JSON from Detections and AreaCount.");

  JSON_Value *o_value = json_value_init_object();
  JSON_Object *o_obj = json_object(o_value);

  JSON_Value *area_count_value = json_value_init_object();
  JSON_Object *area_count_obj = json_object(area_count_value);

  for (size_t i = 0; i < CLASS_IDS_SIZE; i++) {
    if (area_count[i].class_id == UINT16_MAX) break;
    char class_id_str[32];
    snprintf(class_id_str, sizeof(class_id_str), "%u", area_count[i].class_id);
    json_object_set_number(area_count_obj, class_id_str, area_count[i].count);
  }

  JSON_Value *detections_value = MakeDetectionJson(detections);

  json_object_set_value(o_obj, "area_count", area_count_value);
  json_object_set_value(o_obj, "detections", detections_value);

  return o_value;
}

DataProcessorResultCode MakeDetectionFlatbuffer(
    Detections *detections, flatbuffers::FlatBufferBuilder *builder) {
  std::vector<flatbuffers::Offset<SmartCamera::GeneralObject>> gdata_vector;
  LOG_DBG("Creating flatbuffer from Detections.");
  for (uint16_t i = 0; i < detections->num_detections; i++) {
    flatbuffers::Offset<SmartCamera::BoundingBox2d> bbox_data =
        SmartCamera::CreateBoundingBox2d(
            *builder, detections->detection_data[i].bbox.left,
            detections->detection_data[i].bbox.top,
            detections->detection_data[i].bbox.right,
            detections->detection_data[i].bbox.bottom);

    auto general_data = SmartCamera::CreateGeneralObject(
        *builder, detections->detection_data[i].class_id,
        SmartCamera::BoundingBox_BoundingBox2d, bbox_data.Union(),
        detections->detection_data[i].score);

    gdata_vector.push_back(general_data);
  }
  auto out_data_top = SmartCamera::CreateObjectDetectionTop(
      *builder, SmartCamera::CreateObjectDetectionData(
                    *builder, builder->CreateVector(gdata_vector)));

  auto out_data = SmartCamera::CreateObjectDetectionRoot(
      *builder, SmartCamera::ObjectDetectionUnion_ObjectDetectionTop,
      out_data_top.Union());

  builder->Finish(out_data);

  return kDataProcessorOk;
}

DataProcessorResultCode MakeAreaFlatbuffer(
    Detections *detections, AreaCount *area_count,
    flatbuffers::FlatBufferBuilder *builder, size_t num_of_class) {
  std::vector<flatbuffers::Offset<SmartCamera::GeneralObject>> gdata_vector;
  LOG_DBG("Creating flatbuffer from Detections and AreaCount.");
  for (uint16_t i = 0; i < detections->num_detections; i++) {
    flatbuffers::Offset<SmartCamera::BoundingBox2d> bbox_data =
        SmartCamera::CreateBoundingBox2d(
            *builder, detections->detection_data[i].bbox.left,
            detections->detection_data[i].bbox.top,
            detections->detection_data[i].bbox.right,
            detections->detection_data[i].bbox.bottom);

    auto general_data = SmartCamera::CreateGeneralObject(
        *builder, detections->detection_data[i].class_id,
        SmartCamera::BoundingBox_BoundingBox2d, bbox_data.Union(),
        detections->detection_data[i].score);

    gdata_vector.push_back(general_data);
  }

  std::vector<flatbuffers::Offset<SmartCamera::CountData>> cdata_vector;

  for (size_t i = 0; i < CLASS_IDS_SIZE; i++) {
    if (area_count[i].class_id == UINT16_MAX) break;
    auto count_data = SmartCamera::CreateCountData(
        *builder, area_count[i].class_id, area_count[i].count);

    cdata_vector.push_back(count_data);
  }

  auto area_count_top = SmartCamera::CreateAreaCountTop(
      *builder, builder->CreateVector(cdata_vector),
      SmartCamera::CreateObjectDetectionData(
          *builder, builder->CreateVector(gdata_vector)));

  auto out_data = SmartCamera::CreateObjectDetectionRoot(
      *builder, SmartCamera::ObjectDetectionUnion_AreaCountTop,
      area_count_top.Union());

  builder->Finish(out_data);
  return kDataProcessorOk;
}
