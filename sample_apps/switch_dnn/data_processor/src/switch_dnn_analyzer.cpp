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

#include "switch_dnn_analyzer.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "sm_utils.hpp"
/* -------------------------------------------------------- */
/* define                                                   */
/* -------------------------------------------------------- */
#define BIRD_CLASS (15)

#define LOG_CM "<AnalyzerCommon>"
#define LOG_OD "<AnalyzerOd>"
#define LOG_IC "<AnalyzerIc>"

/////////////////////////////////
// AnalyzerBase
/////////////////////////////////
AnalyzerBase::AnalyzerBase() { pthread_mutex_init(&mutex_, nullptr); }
AnalyzerBase::~AnalyzerBase() { pthread_mutex_destroy(&mutex_); }
int AnalyzerBase::Lock() { return pthread_mutex_lock(&mutex_); }
int AnalyzerBase::Unlock() { return pthread_mutex_unlock(&mutex_); }
char *AnalyzerBase::Format(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(print_buffer_, sizeof(print_buffer_), format, ap);
  va_end(ap);
  return print_buffer_;
}

/////////////////////////////////
// AnalyzerCommon
/////////////////////////////////

AnalyzerBase::ResultCode AnalyzerCommon::ValidateParam(const void *param) {
  AnalyzerBase::ScopeLock lock(this);
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerCommon::SetValidatedParam(const void *param) {
  AnalyzerBase::ScopeLock lock(this);
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerCommon::ClearValidatingParam() {
  AnalyzerBase::ScopeLock lock(this);
  DoClearValidatingParam();
  return AnalyzerBase::ResultCode::kOk;
}

void AnalyzerCommon::DoClearValidatingParam() {
  // nothing to do.
}

AnalyzerBase::ResultCode AnalyzerCommon::Analyze(const float *p_data,
                                                 uint32_t size,
                                                 uint64_t trace_id) {
  AnalyzerBase::ScopeLock lock(this);
  return AnalyzerBase::ResultCode::kInvalidParam;
}
AnalyzerBase::ResultCode AnalyzerCommon::Serialize(void **out_buf,
                                                   uint32_t *size,
                                                   const Allocator *allocator) {
  AnalyzerBase::ScopeLock lock(this);
  return AnalyzerBase::ResultCode::kInvalidParam;
}

AnalyzerBase::ResultCode AnalyzerCommon::GetNetworkId(
    char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const {
  return AnalyzerBase::ResultCode::kInvalidParam;
}

/////////////////////////////////
// AnalyzerOd
/////////////////////////////////

AnalyzerBase::ResultCode AnalyzerOd::ValidateParam(const void *param) {
  EdgeAppLibLogDebug(LOG_OD, "ValidateParam");
  AnalyzerBase::ScopeLock lock(this);
  // parse the json parameter
  JSON_Value *root_value = json_parse_string((char *)param);
  JSON_Value_Type type = json_value_get_type(root_value);
  if (type != JSONObject) {
    json_value_free(root_value);
    EdgeAppLibLogError(LOG_OD "ValidateParam", "Invalid param");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  if (!validating_param_) {
    validating_param_ = new PPLParam;
  }
  AnalyzerBase::ResultCode ret =
      ObjectDetectionParamInit(root_value, validating_param_);
  if (ret != AnalyzerBase::ResultCode::kOk) {
    EdgeAppLibLogError(LOG_OD "ValidateParam: Get json_parse Fail Err:",
                       Format("%d", ret));
    json_value_free(root_value);
    DoClearValidatingParam();
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  json_value_free(root_value);
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::SetValidatedParam(const void *param) {
  EdgeAppLibLogDebug(LOG_OD, "SetValidatedParam");
  AnalyzerBase::ScopeLock lock(this);
  if (!validating_param_) {
    return AnalyzerBase::ResultCode::kOtherError;
  }
  // set ppl parameter
  param_ = *validating_param_;
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::ClearValidatingParam() {
  EdgeAppLibLogDebug(LOG_OD, "ClearValidatingParam");
  AnalyzerBase::ScopeLock lock(this);
  DoClearValidatingParam();
  return AnalyzerBase::ResultCode::kOk;
}

void AnalyzerOd::DoClearValidatingParam() {
  if (validating_param_) {
    delete validating_param_;
    validating_param_ = nullptr;
  }
}

AnalyzerBase::ResultCode AnalyzerOd::Analyze(const float *p_data, uint32_t size,
                                             uint64_t trace_id) {
  EdgeAppLibLogDebug(LOG_OD, "Analyze");
  AnalyzerBase::ScopeLock lock(this);

  PPLParam ppl_parameter;
  AnalyzerOd::GetParam(ppl_parameter);

  if (p_data == nullptr) {
    EdgeAppLibLogError(LOG_OD "Analyze:", "Invalid param pdata=nullptr");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }

  // call interface process
  DetectionOutputTensor objectdetection_output;
  createDetectionData(p_data, size, &objectdetection_output);

  // call analyze process
  DetectionData output_objectdetection_data;
  analyzeDetectionOutput(objectdetection_output, &output_objectdetection_data,
                         ppl_parameter, trace_id);
  data_ = output_objectdetection_data;
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::Serialize(void **out_buf, uint32_t *size,
                                               const Allocator *allocator) {
  EdgeAppLibLogDebug(LOG_OD, "Serialize");
  AnalyzerBase::ScopeLock lock(this);

  DetectionData detection_data;
  AnalyzerOd::GetAnalyzedData(detection_data);

  flatbuffers::FlatBufferBuilder *builder =
      new flatbuffers::FlatBufferBuilder();
  createSSDOutputFlatbuffer(builder, &detection_data);

  uint8_t *buf_ptr = builder->GetBufferPointer();
  if (buf_ptr == nullptr) {
    EdgeAppLibLogError(LOG_OD, "Error could not create Flatbuffer");
    builder->Clear();
    delete (builder);
    return AnalyzerBase::ResultCode::kOtherError;
  }

  uint32_t buf_size = builder->GetSize();
  uint8_t *p_out_param = nullptr;
  p_out_param = (uint8_t *)allocator->Malloc(buf_size);
  if (p_out_param == nullptr) {
    EdgeAppLibLogError(LOG_OD
                       "malloc failed for creating flatbuffer, malloc size=",
                       Format("%u", buf_size));
    builder->Clear();
    delete (builder);
    return AnalyzerBase::ResultCode::kMemoryError;
  }
  EdgeAppLibLogDebug(LOG_OD "p_out_param=", Format("%p", p_out_param));

  // Copy Data
  memcpy(p_out_param, buf_ptr, buf_size);
  *out_buf = p_out_param;
  *size = buf_size;

  // Clean up
  builder->Clear();
  delete (builder);

  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::GetNetworkId(
    char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const {
  memcpy(network_id, network_id_, AI_MODEL_BUNDLE_ID_SIZE);

  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerOd::GetAnalyzedData(
    DetectionData &data) const {
  data = data_;
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::GetInputTensorSize(
    uint16_t &width, uint16_t &height) const {
  width = param_.input_width_;
  height = param_.input_height_;
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::SetNetworkId(const char *network_id) {
  if (network_id == nullptr) {
    EdgeAppLibLogError(LOG_OD "SetNetworkId:",
                       "AI model bundle ID is not available");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }

  size_t id_length = strlen(network_id);
  if (id_length >= sizeof(network_id_)) {
    EdgeAppLibLogError(LOG_OD "SetNetworkId:",
                       "AI model bundle ID is too long");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }

  memcpy(network_id_, network_id, id_length);
  network_id_[id_length] = 0L;  // null terminated.

  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::ObjectDetectionParamInit(
    JSON_Value *root_value, AnalyzerOd::PPLParam *p_param) {
  JSON_Object *object = json_value_get_object(root_value);
  int ret = json_object_has_value(object, "ai_models");
  if (ret) {
    JSON_Object *models = json_object_get_object(object, "ai_models");
    ret = json_object_has_value(models, "detection_bird");
    if (ret) {
      JSON_Object *detection_param =
          json_object_get_object(models, "detection_bird");

      // Check network id
      ret = json_object_has_value(detection_param, "ai_model_bundle_id");
      if (ret) {
        const char *network_id_str =
            json_object_get_string(detection_param, "ai_model_bundle_id");
        AnalyzerBase::ResultCode res = SetNetworkId(network_id_str);
        if (res != AnalyzerBase::ResultCode::kOk) {
          return res;
        }
        EdgeAppLibLogDebug(
            LOG_OD "ObjectDetectionParamInit ai_model_bundle_id:", network_id_);
      } else {
        EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                           "json file does not have ai_model_bundle_id");
        return AnalyzerBase::ResultCode::kInvalidParam;
      }

      // Check param
      ret = json_object_has_value(detection_param, "param");
      if (ret) {
        JSON_Object *param = json_object_get_object(detection_param, "param");
        ret = json_object_has_value(param, "max_detections");
        if (ret) {
          double aux = json_object_get_number(param, "max_detections");
          if (aux < 0) return AnalyzerBase::ResultCode::kInvalidParamOutOfRange;
          uint16_t maxDetections =
              (int)json_object_get_number(param, "max_detections");
          EdgeAppLibLogDebug(LOG_OD "ObjectDetectionParamInit max_detections:",
                             Format("%hu", maxDetections));
          p_param->max_detections_ = maxDetections;
        } else {
          EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                             "json file does not have max_detections");
          return AnalyzerBase::ResultCode::kInvalidParam;
        }
        ret = json_object_has_value(param, "threshold");
        if (ret) {
          float threshold = json_object_get_number(param, "threshold");
          EdgeAppLibLogDebug(LOG_OD "ObjectDetectionParamInit threshold:",
                             Format("%f", threshold));
          if (threshold < 0.0 || threshold > 1.0) {
            EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                               "threshold value out of range");
            return AnalyzerBase::ResultCode::kInvalidParamOutOfRange;
          } else {
            p_param->threshold_ = threshold;
          }
        } else {
          EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                             "json file does not have threshold");
          return AnalyzerBase::ResultCode::kInvalidParam;
        }
        ret = json_object_has_value(param, "input_width");
        if (ret) {
          double aux = json_object_get_number(param, "input_width");
          if (aux < 0) return AnalyzerBase::ResultCode::kInvalidParamOutOfRange;
          uint16_t input_width = json_object_get_number(param, "input_width");
          EdgeAppLibLogDebug(LOG_OD "ObjectDetectionParamInit input_width:",
                             Format("%hu", input_width));
          p_param->input_width_ = input_width;
        } else {
          EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                             "json file does not have input_width");
          return AnalyzerBase::ResultCode::kInvalidParam;
        }
        ret = json_object_has_value(param, "input_height");
        if (ret) {
          double aux = json_object_get_number(param, "input_height");
          if (aux < 0) return AnalyzerBase::ResultCode::kInvalidParamOutOfRange;
          uint16_t input_height = json_object_get_number(param, "input_height");
          EdgeAppLibLogDebug(LOG_OD "ObjectDetectionParamInit input_height:",
                             Format("%hu", input_height));
          p_param->input_height_ = input_height;
        } else {
          EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                             "json file does not have input_height");
          return AnalyzerBase::ResultCode::kInvalidParam;
        }
        ret = json_object_has_value(param, "force_switch");
        if (ret) {
          uint8_t force_switch = json_object_get_number(param, "force_switch");
          EdgeAppLibLogDebug(LOG_OD "ObjectDetectionParamInit input_height:",
                             Format("%hu", force_switch));
          p_param->force_switch_ = force_switch;
        }
      } else {
        EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                           "json file does not have param");
        return AnalyzerBase::ResultCode::kInvalidParam;
      }
    } else {
      EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                         "json file does not have detection_bird");
      return AnalyzerBase::ResultCode::kInvalidParam;
    }
  } else {
    EdgeAppLibLogError(LOG_OD "ObjectDetectionParamInit:",
                       "json file does not have models");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerOd::GetParam(
    AnalyzerOd::PPLParam &param) const {
  param = param_;
  return ResultCode::kOk;
}

void AnalyzerOd::createDetectionData(
    const float *data_body, uint32_t num_elements,
    AnalyzerOd::DetectionOutputTensor *objectdetection_output) {
  const float *out_data = data_body;
  uint32_t count = 0;
  std::vector<OutputTensorBbox> v_bbox;
  std::vector<float> v_scores;
  std::vector<float> v_classes;
  // Extract number of Detections
  uint8_t totalDetections = (num_elements - 1) / 6;

  // Extract bounding box co-ordinates
  for (uint8_t i = 0; i < totalDetections; i++) {
    OutputTensorBbox bbox;
    bbox.y_min_ = out_data[count + i];
    bbox.x_min_ = out_data[count + i + (1 * totalDetections)];
    bbox.y_max_ = out_data[count + i + (2 * totalDetections)];
    bbox.x_max_ = out_data[count + i + (3 * totalDetections)];
    v_bbox.push_back(bbox);
  }
  count += (totalDetections * 4);

  // Extract class indices
  for (uint8_t i = 0; i < totalDetections; i++) {
    float class_index;
    class_index = out_data[count];
    v_classes.push_back(class_index);
    count++;
  }

  // Extract scores
  for (uint8_t i = 0; i < totalDetections; i++) {
    float score;
    score = out_data[count];
    v_scores.push_back(score);
    count++;
  }

  // Extract number of Detections
  uint8_t numOfDetections = (uint8_t)out_data[count];
  if (numOfDetections > totalDetections) {
    EdgeAppLibLogWarn(LOG_OD "Unexpected value for numOfDetections:",
                      Format("%hhu", numOfDetections));
    EdgeAppLibLogWarn(LOG_OD "setting it to", Format("%hhu", totalDetections));
    numOfDetections = totalDetections;
  }
  objectdetection_output->num_of_detections_ = numOfDetections;
  objectdetection_output->bboxes_ = v_bbox;
  objectdetection_output->scores_ = v_scores;
  objectdetection_output->classes_ = v_classes;
}

void AnalyzerOd::analyzeDetectionOutput(
    AnalyzerOd::DetectionOutputTensor out_tensor,
    AnalyzerOd::DetectionData *output_objectdetection_data,
    AnalyzerOd::PPLParam param, uint64_t trace_id) {
  uint8_t num_of_detections;
  uint8_t detections_above_threshold = 0;
  std::vector<Rect> v_bbox;
  std::vector<float> v_scores;
  std::vector<uint8_t> v_classes;
  std::vector<bool> v_is_used_for_cropping;
  DetectionData objectdetection_data;

  // Extract number of detections
  num_of_detections = (uint8_t)out_tensor.num_of_detections_;
  for (uint8_t i = 0; i < num_of_detections; i++) {
    // Extract classes
    uint8_t class_index;
    class_index = (uint8_t)out_tensor.classes_[i];

    // Filter class
    if (class_index != BIRD_CLASS) {
      continue;
    }

    // Extract scores
    float score;
    score = out_tensor.scores_[i];

    // Filter Detections
    if (score < param.threshold_) {
      continue;
    } else {
      v_classes.push_back(class_index);
      v_scores.push_back(score);
      // Extract bounding box co-ordinates
      Rect bbox;
      bbox.m_xmin_ = (uint16_t)(round((out_tensor.bboxes_[i].x_min_) *
                                      (param.input_width_ - 1)));
      bbox.m_ymin_ = (uint16_t)(round((out_tensor.bboxes_[i].y_min_) *
                                      (param.input_height_ - 1)));
      bbox.m_xmax_ = (uint16_t)(round((out_tensor.bboxes_[i].x_max_) *
                                      (param.input_width_ - 1)));
      bbox.m_ymax_ = (uint16_t)(round((out_tensor.bboxes_[i].y_max_) *
                                      (param.input_height_ - 1)));
      v_bbox.push_back(bbox);
      v_is_used_for_cropping.push_back(false);
      detections_above_threshold++;
    }
  }

  objectdetection_data.num_of_detections_ = detections_above_threshold;
  objectdetection_data.v_bbox_ = v_bbox;
  objectdetection_data.v_scores_ = v_scores;
  objectdetection_data.v_classes_ = v_classes;
  objectdetection_data.v_is_used_for_cropping_ = v_is_used_for_cropping;
  if (objectdetection_data.num_of_detections_ > param.max_detections_) {
    objectdetection_data.num_of_detections_ = param.max_detections_;
    objectdetection_data.v_bbox_.resize(param.max_detections_);
    objectdetection_data.v_classes_.resize(param.max_detections_);
    objectdetection_data.v_scores_.resize(param.max_detections_);
    objectdetection_data.v_is_used_for_cropping_.resize(param.max_detections_);
  }

  EdgeAppLibLogDebug(LOG_OD "number of detections=",
                     Format("%hhu", objectdetection_data.num_of_detections_));
  num_of_detections = objectdetection_data.num_of_detections_;
  // check the highest score and set is_used_for_cropping parameter to true
  if (num_of_detections > 0) {
    float highest_score = 0;
    uint8_t highest_score_index = 0;
    for (uint8_t i = 0; i < num_of_detections; i++) {
      float score = objectdetection_data.v_scores_[i];
      if (score > highest_score) {
        highest_score = score;
        highest_score_index = i;
      }
    }

    objectdetection_data.v_is_used_for_cropping_.at(highest_score_index) = true;
    EdgeAppLibLogDebug(LOG_OD
                       "The coordinates in the data with the highest score",
                       Format("%f are used for Crop", highest_score));
  }

  output_objectdetection_data->num_of_detections_ =
      objectdetection_data.num_of_detections_;
  output_objectdetection_data->v_bbox_ = objectdetection_data.v_bbox_;
  output_objectdetection_data->v_scores_ = objectdetection_data.v_scores_;
  output_objectdetection_data->v_classes_ = objectdetection_data.v_classes_;
  output_objectdetection_data->v_is_used_for_cropping_ =
      objectdetection_data.v_is_used_for_cropping_;
  output_objectdetection_data->trace_id_ = trace_id;
  for (int i = 0; i < num_of_detections; i++) {
    uint16_t xmin = objectdetection_data.v_bbox_[i].m_xmin_;
    uint16_t ymin = objectdetection_data.v_bbox_[i].m_ymin_;
    uint16_t xmax = objectdetection_data.v_bbox_[i].m_xmax_;
    uint16_t ymax = objectdetection_data.v_bbox_[i].m_ymax_;
    EdgeAppLibLogDebug(LOG_OD, Format("v_bbox[%d]", i));
    EdgeAppLibLogDebug(LOG_OD "[x_min,y_min,x_max,y_max] =",
                       Format("[%hu,%hu,%hu,%hu]", xmin, ymin, xmax, ymax));
    float score = objectdetection_data.v_scores_[i];
    EdgeAppLibLogDebug(LOG_OD "scores", Format("[%d] = %f", i, score));
    uint8_t class_index = objectdetection_data.v_classes_[i];
    EdgeAppLibLogDebug(LOG_OD "class_indices",
                       Format("[%d] = %hhu", i, class_index));
    bool used_for_cropping = objectdetection_data.v_is_used_for_cropping_[i];
    EdgeAppLibLogDebug(
        LOG_OD "is_used_for_cropping",
        Format("[%d] = %s", i, used_for_cropping ? "true" : "false"));
  }
  EdgeAppLibLogDebug(LOG_OD "trace_id=", Format("%llu", trace_id));
  return;
}

void AnalyzerOd::createSSDOutputFlatbuffer(
    flatbuffers::FlatBufferBuilder *builder,
    const AnalyzerOd::DetectionData *detection_data) {
  std::vector<flatbuffers::Offset<SmartCamera::GeneralObject>> gdata_vector;
  uint8_t numOfDetections = detection_data->num_of_detections_;
  for (uint8_t i = 0; i < numOfDetections; i++) {
    EdgeAppLibLogDebug(
        LOG_OD "[left,top,right,bottom]=",
        Format("[%hu,%hu,%hu,%hu]", detection_data->v_bbox_[i].m_xmin_,
               detection_data->v_bbox_[i].m_ymin_,
               detection_data->v_bbox_[i].m_xmax_,
               detection_data->v_bbox_[i].m_ymax_));
    EdgeAppLibLogDebug(LOG_OD "[class,score]=",
                       Format("[%hhu,%f]", detection_data->v_classes_[i],
                              detection_data->v_scores_[i]));
    auto bbox_data = SmartCamera::CreateBoundingBox2d(
        *builder, detection_data->v_bbox_[i].m_xmin_,
        detection_data->v_bbox_[i].m_ymin_, detection_data->v_bbox_[i].m_xmax_,
        detection_data->v_bbox_[i].m_ymax_);
    auto general_data = SmartCamera::CreateGeneralObject(
        *builder, detection_data->v_classes_[i],
        SmartCamera::BoundingBox_BoundingBox2d, bbox_data.Union(),
        detection_data->v_scores_[i],
        detection_data->v_is_used_for_cropping_[i]);
    gdata_vector.push_back(general_data);
  }

  auto v_od_data = builder->CreateVector(gdata_vector);
  auto od_data = SmartCamera::CreateObjectDetectionData(*builder, v_od_data);
  auto out_data = SmartCamera::CreateObjectDetectionTop(
      *builder, od_data, detection_data->trace_id_);
  builder->Finish(out_data);
  return;
}

/////////////////////////////////
// AnalyzerIc
/////////////////////////////////

AnalyzerBase::ResultCode AnalyzerIc::ValidateParam(const void *param) {
  EdgeAppLibLogDebug(LOG_IC, "ValidateParam");
  AnalyzerBase::ScopeLock lock(this);

  // parse the json parameter
  JSON_Value *root_value;
  root_value = json_parse_string((char *)param);
  JSON_Value_Type type = json_value_get_type(root_value);
  if (type != JSONObject) {
    json_value_free(root_value);
    EdgeAppLibLogError(LOG_IC "ValidateParam:", "Invalid param");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  if (!validating_param_) {
    validating_param_ = new PPLParam;
  }
  AnalyzerBase::ResultCode ret =
      ClassificationParamInit(root_value, validating_param_);
  if (ret != AnalyzerBase::ResultCode::kOk) {
    EdgeAppLibLogError(LOG_IC "ValidateParam: Get json_parse Fail Err:",
                       Format("%d", ret));
    json_value_free(root_value);
    DoClearValidatingParam();
    return ret;
  }
  json_value_free(root_value);
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::SetValidatedParam(const void *param) {
  EdgeAppLibLogDebug(LOG_IC, "SetValidatedParam");
  AnalyzerBase::ScopeLock lock(this);
  if (!validating_param_) {
    return AnalyzerBase::ResultCode::kOtherError;
  }
  // set ppl parameter
  param_ = *validating_param_;
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::ClearValidatingParam() {
  EdgeAppLibLogDebug(LOG_IC, "ClearValidatingParam");
  AnalyzerBase::ScopeLock lock(this);
  DoClearValidatingParam();
  return AnalyzerBase::ResultCode::kOk;
}

void AnalyzerIc::DoClearValidatingParam() {
  if (validating_param_) {
    delete validating_param_;
    validating_param_ = nullptr;
  }
}

AnalyzerBase::ResultCode AnalyzerIc::Analyze(const float *p_data, uint32_t size,
                                             uint64_t trace_id) {
  EdgeAppLibLogDebug(LOG_IC, "Analyze");
  AnalyzerBase::ScopeLock lock(this);

  PPLParam ppl_parameter;
  AnalyzerIc::GetParam(ppl_parameter);
  if (p_data == nullptr) {
    EdgeAppLibLogError(LOG_IC "Analyze:", "Invalid param pdata=nullptr");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }

  // call interface process
  ClassificationOutputTensor classification_output;
  createClassificationData(p_data, size, &classification_output);
  // call analyze process
  ClassificationData output_classification_data;
  analyzeClassificationOutput(classification_output,
                              &output_classification_data, ppl_parameter,
                              trace_id);
  data_ = output_classification_data;
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::Serialize(void **out_buf, uint32_t *size,
                                               const Allocator *allocator) {
  EdgeAppLibLogDebug(LOG_IC, "Serialize");
  AnalyzerBase::ScopeLock lock(this);

  ClassificationData classification_data;
  AnalyzerIc::GetAnalyzedData(classification_data);

  flatbuffers::FlatBufferBuilder *builder =
      new flatbuffers::FlatBufferBuilder();
  createClassificationFlatbuffer(builder, &classification_data);

  uint8_t *buf_ptr = builder->GetBufferPointer();
  if (buf_ptr == nullptr) {
    EdgeAppLibLogError(LOG_IC, "Error could not create Flatbuffer");
    builder->Clear();
    delete (builder);
    return AnalyzerBase::ResultCode::kOtherError;
  }

  uint32_t buf_size = builder->GetSize();
  uint8_t *p_out_param = nullptr;
  p_out_param = (uint8_t *)allocator->Malloc(buf_size);
  if (p_out_param == nullptr) {
    EdgeAppLibLogError(LOG_IC
                       "malloc failed for creating flatbuffer, malloc size=",
                       Format("%u", buf_size));
    builder->Clear();
    delete (builder);
    return AnalyzerBase::ResultCode::kMemoryError;
  }
  EdgeAppLibLogDebug(LOG_IC "p_out_param=", Format("%p", p_out_param));

  // Copy Data
  memcpy(p_out_param, buf_ptr, buf_size);
  *out_buf = p_out_param;
  *size = buf_size;

  // Clean up
  builder->Clear();
  delete (builder);
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::GetNetworkId(
    char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const {
  memcpy(network_id, network_id_, AI_MODEL_BUNDLE_ID_SIZE);

  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::GetAnalyzedData(
    AnalyzerIc::ClassificationData &data) const {
  data = data_;
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::SetNetworkId(const char *network_id) {
  if (network_id == nullptr) {
    EdgeAppLibLogError(LOG_IC "SetNetworkId:",
                       "AI model bundle ID is not available");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }

  size_t id_length = strlen(network_id);
  if (id_length >= sizeof(network_id_)) {
    EdgeAppLibLogError(LOG_IC "SetNetworkId:",
                       "AI model bundle ID is too long");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }

  memcpy(network_id_, network_id, id_length);
  network_id_[id_length] = 0L;  // null terminated.

  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerIc::ClassificationParamInit(
    JSON_Value *root_value, AnalyzerIc::PPLParam *p_cls_param) {
  JSON_Object *object = json_value_get_object(root_value);
  int ret = json_object_has_value(object, "ai_models");
  if (ret) {
    JSON_Object *models = json_object_get_object(object, "ai_models");
    ret = json_object_has_value(models, "classification_bird");
    if (ret) {
      JSON_Object *classification_param =
          json_object_get_object(models, "classification_bird");

      // Check network id
      ret = json_object_has_value(classification_param, "ai_model_bundle_id");
      if (ret) {
        const char *network_id_str =
            json_object_get_string(classification_param, "ai_model_bundle_id");
        AnalyzerBase::ResultCode res = SetNetworkId(network_id_str);
        if (res != AnalyzerBase::ResultCode::kOk) {
          return res;
        }
        EdgeAppLibLogDebug(LOG_IC "ClassificationParamInit ai_model_bundle_id:",
                           network_id_);
      } else {
        EdgeAppLibLogError(LOG_IC "ClassificationParamInit:",
                           "json file does not have ai_model_bundle_id");
        return AnalyzerBase::ResultCode::kInvalidParam;
      }

      // Check param
      ret = json_object_has_value(classification_param, "param");
      if (ret) {
        JSON_Object *param =
            json_object_get_object(classification_param, "param");
        ret = json_object_has_value(param, "max_predictions");
        if (ret) {
          double aux = json_object_get_number(param, "max_predictions");
          if (aux < 0) return AnalyzerBase::ResultCode::kInvalidParamOutOfRange;
          uint16_t maxPredictions =
              (int)json_object_get_number(param, "max_predictions");
          EdgeAppLibLogDebug(LOG_IC "ClassificationParamInit max_predictions:",
                             Format("%hu", maxPredictions));
          p_cls_param->max_predictions_ = maxPredictions;
        } else {
          EdgeAppLibLogError(LOG_IC "ClassificationParamInit:",
                             "json file does not have max_predictions");
          return AnalyzerBase::ResultCode::kInvalidParam;
        }
      } else {
        EdgeAppLibLogError(LOG_IC "ClassificationParamInit:",
                           "json file does not have param");
        return AnalyzerBase::ResultCode::kInvalidParam;
      }
    } else {
      EdgeAppLibLogError(LOG_IC "ClassificationParamInit:",
                         "json file does not have classification_bird");
      return AnalyzerBase::ResultCode::kInvalidParam;
    }
  } else {
    EdgeAppLibLogError(LOG_IC "ClassificationParamInit:",
                       "json file does not have models");
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::GetParam(
    AnalyzerIc::PPLParam &param) const {
  param = param_;
  return ResultCode::kOk;
}

void AnalyzerIc::createClassificationData(
    const float *data_body, uint32_t data_size,
    AnalyzerIc::ClassificationOutputTensor *classification_output) {
  uint16_t size = data_size;
  for (uint16_t i = 0; i < size; i++) {
    float score = data_body[i];
    classification_output->scores_.push_back(score);
  }
  return;
}

void AnalyzerIc::analyzeClassificationOutput(
    AnalyzerIc::ClassificationOutputTensor out_tensor,
    AnalyzerIc::ClassificationData *output_classification_data,
    AnalyzerIc::PPLParam cls_param, uint64_t trace_id) {
  uint16_t num_of_predictions;
  std::vector<ClassificationItem> class_data;

  // Extract number of predictions
  num_of_predictions = out_tensor.scores_.size();

  // Extract scores
  for (uint16_t i = 0; i < num_of_predictions; i++) {
    ClassificationItem item;
    item.index_ = i;
    item.score_ = out_tensor.scores_[i];
    class_data.push_back(item);
  }

  uint16_t num_of_classes = class_data.size();
  if (num_of_classes > 0) {
    std::stable_sort(
        class_data.begin(), class_data.end(),
        [](const ClassificationItem &left, const ClassificationItem &right) {
          return left.score_ > right.score_;
        });
  }

  if (cls_param.max_predictions_ < num_of_predictions) {
    LOG_WARN(
        "Number of classses in the model output tensor is lower than the "
        "expected maxPreditions");

    num_of_predictions = cls_param.max_predictions_;
  }
  for (uint16_t i = 0; i < num_of_predictions; i++) {
    output_classification_data->v_classItem_.push_back(class_data[i]);
    EdgeAppLibLogDebug(
        LOG_IC, Format("Top[%hu] = id: %d  score: %f", i, class_data[i].index_,
                       class_data[i].score_));
  }

  output_classification_data->trace_id_ = trace_id;
  EdgeAppLibLogDebug(LOG_IC, Format("trace_id = %llu", trace_id));
  return;
}

void AnalyzerIc::createClassificationFlatbuffer(
    flatbuffers::FlatBufferBuilder *builder,
    const AnalyzerIc::ClassificationData *classificationData) {
  std::vector<flatbuffers::Offset<SmartCamera::GeneralClassification>>
      gdata_vector;
  uint16_t num_of_classes = classificationData->v_classItem_.size();
  for (uint16_t i = 0; i < num_of_classes; i++) {
    auto item = classificationData->v_classItem_[i];
    EdgeAppLibLogDebug(
        LOG_IC, Format("class = %d, score = %f", item.index_, item.score_));
    auto general_data = SmartCamera::CreateGeneralClassification(
        *builder, item.index_, item.score_);
    gdata_vector.push_back(general_data);
  }

  auto v_bbox = builder->CreateVector(gdata_vector);
  auto class_data = SmartCamera::CreateClassificationData(*builder, v_bbox);
  auto out_data = SmartCamera::CreateClassificationTop(
      *builder, class_data, classificationData->trace_id_);
  builder->Finish(out_data);
  return;
}
