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

#ifndef _ANALYZER_SWITCH_DNN_H_
#define _ANALYZER_SWITCH_DNN_H_

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include <vector>

#include "parson.h"
#include "sensor.h"
#include "switch_dnn_classification_generated.h"
#include "switch_dnn_objectdetection_generated.h"

/**
 * @class AnalyzerBase
 * @brief The base class of analysis
 */
class AnalyzerBase {
 public:
  /**
   * @enum AnalyzerBase::ResultCode
   * @brief The error code of analysis process
   */
  enum class ResultCode {
    kOk,
    kInvalidParam,
    kMemoryError,
    kOtherError,
    kInvalidParamOutOfRange
  };

  /**
   * @class Allocator
   * @brief The base class of allocator
   */
  class Allocator {
   public:
    virtual void *Malloc(size_t size) const = 0;
    virtual void Free(void *buf) const = 0;
  };

 public:
  /**
   * @brief Constructor
   */
  AnalyzerBase();

  /**
   * @brief Destructor
   */
  virtual ~AnalyzerBase();

  /**
   * @brief Validate parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode ValidateParam(const void *param) = 0;

  /**
   * @brief Set parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode SetValidatedParam(const void *param) = 0;

  /**
   * @brief Clear parameter
   * @return ResultCode
   */
  virtual ResultCode ClearValidatingParam() = 0;

  /**
   * @brief Analyze the Output Tensor
   * @param [in] p_data output tensor data
   * @param [in] size output tensor size
   * @param [in] trace_id id for linking the post-processing of each AI
   * @return ResultCode
   */
  virtual ResultCode Analyze(const float *p_data, uint32_t size,
                             uint64_t trace_id) = 0;

  /**
   * @brief Serialize analyzed data
   * @param [out] out_buf serialized data
   * @param [out] size serialized data size
   * @param [in] allocator instance of Allocator
   * @return ResultCode
   */
  virtual ResultCode Serialize(void **out_buf, uint32_t *size,
                               const Allocator *allocator) = 0;

  /**
   * @brief Get network id
   * @param [out] network_id network id
   * @return ResultCode
   */
  virtual ResultCode GetNetworkId(
      char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const = 0;

 protected:
  /**
   * @brief Clear parameter
   */
  virtual void DoClearValidatingParam() = 0;

 protected:
  /**
   * @brief Lock mutex
   * @return int : 0 is success, other value is failure
   */
  int Lock();

  /**
   * @brief Unlock mutex
   * @return int : 0 is success, other value is failure
   */
  int Unlock();

  /**
   * @brief Create formatted string
   * @return pointer to formatted string
   */
  char *Format(const char *format, ...);

  /**
   * @class ScopeLock
   * @brief Control the mutex in the Analyze class
   */
  class ScopeLock {
   private:
    AnalyzerBase *instance_;

   public:
    /**
     * @brief Constructor
     */
    explicit ScopeLock(AnalyzerBase *instance) : instance_(instance) {
      instance_->Lock();
    }
    /**
     * @brief Destructor
     */
    ~ScopeLock() { instance_->Unlock(); }
  };

 private:
  pthread_mutex_t mutex_;
  char print_buffer_[64];
};

/**
 * @class AnalyzerCommon
 * @brief Common class for analysis
 */
class AnalyzerCommon : public AnalyzerBase {
 public:
  /**
   * @brief Constructor
   */
  AnalyzerCommon() : AnalyzerBase() {}

  /**
   * @brief Destructor
   */
  virtual ~AnalyzerCommon() {}

  /**
   * @brief Validate parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode ValidateParam(const void *param) override;

  /**
   * @brief Set parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode SetValidatedParam(const void *param) override;

  /**
   * @brief Clear parameter
   * @return ResultCode
   */
  virtual ResultCode ClearValidatingParam() override;

  /**
   * @brief Analyze the Output Tensor
   * @param [in] p_data output tensor data
   * @param [in] size output tensor size
   * @param [in] trace_id id for linking the post-processing of each AI
   * @return ResultCode
   */
  virtual ResultCode Analyze(const float *p_data, uint32_t size,
                             uint64_t trace_id) override;

  /**
   * @brief Serialize analyzed data
   * @param [out] out_buf serialized data
   * @param [out] size serialized data size
   * @param [in] allocator instance of Allocator
   * @return ResultCode
   */
  virtual ResultCode Serialize(void **out_buf, uint32_t *size,
                               const Allocator *allocator) override;

  /**
   * @brief Get network id
   * @param [out] network_id network id
   * @return ResultCode
   */
  virtual ResultCode GetNetworkId(
      char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const override;

 protected:
  /**
   * @brief Clear parameter
   */
  virtual void DoClearValidatingParam() override;
};

/**
 * @class AnalyzerOd
 * @brief Object Detection analysis class
 */
class AnalyzerOd : public AnalyzerBase {
 private:
  /**
   * @struct OutputTensorBbox
   * @brief Bounding box coordinate of Output Tensor
   */
  typedef struct tagOutputTensorBbox {
    float x_min_ = 0.0;
    float y_min_ = 0.0;
    float x_max_ = 0.0;
    float y_max_ = 0.0;
  } OutputTensorBbox;

  /**
   * @struct DetectionOutputTensor
   * @brief Output Tensor data
   */
  typedef struct tagDetectionOutputTensor {
    float num_of_detections_ = 0.0;
    std::vector<OutputTensorBbox> bboxes_;
    std::vector<float> scores_;
    std::vector<float> classes_;
  } DetectionOutputTensor;

 public:
  /**
   * @struct PPLParam
   * @brief Object Detection PPL Parameter
   */
  typedef struct tagPPLParam {
    uint16_t max_detections_ = 0;
    uint16_t input_width_ = 0;
    uint16_t input_height_ = 0;
    float threshold_ = 0.0;
    uint8_t force_switch_ = 0;
  } PPLParam;

  /**
   * @struct Rect
   * @brief Rectangle coordinate
   */
  typedef struct tagRect {
    uint16_t m_xmin_ = 0;
    uint16_t m_ymin_ = 0;
    uint16_t m_xmax_ = 0;
    uint16_t m_ymax_ = 0;
  } Rect;

  /**
   * @struct DetectionData
   * @brief Analyzed data
   */
  typedef struct tagDetectionData {
    uint8_t num_of_detections_ = 0;
    uint64_t trace_id_ = 0;
    std::vector<Rect> v_bbox_;
    std::vector<float> v_scores_;
    std::vector<uint8_t> v_classes_;
    std::vector<bool> v_is_used_for_cropping_;
  } DetectionData;

 private:
  DetectionData data_;
  PPLParam param_;
  PPLParam *validating_param_ = nullptr;
  char network_id_[AI_MODEL_BUNDLE_ID_SIZE] = {0};

 public:
  /**
   * @brief Constructor
   */

  AnalyzerOd() : AnalyzerBase() {}

  /**
   * @brief Destructor
   */
  virtual ~AnalyzerOd() {
    if (validating_param_) {
      delete validating_param_;
    }
  }

  /**
   * @brief Validate parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode ValidateParam(const void *param) override;

  /**
   * @brief Set parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode SetValidatedParam(const void *param) override;

  /**
   * @brief Clear parameter
   * @return ResultCode
   */
  virtual ResultCode ClearValidatingParam() override;

  /**
   * @brief Analyze the Output Tensor
   * @param [in] p_data output tensor data
   * @param [in] size output tensor size
   * @param [in] trace_id id for linking the post-processing of each AI
   * @return ResultCode
   */
  virtual ResultCode Analyze(const float *p_data, uint32_t size,
                             uint64_t trace_id) override;

  /**
   * @brief Serialize analyzed data
   * @param [out] out_buf serialized data
   * @param [out] size serialized data size
   * @param [in] allocator instance of Allocator
   * @return ResultCode
   */
  virtual ResultCode Serialize(void **out_buf, uint32_t *size,
                               const Allocator *allocator) override;

  /**
   * @brief Get network id
   * @param [out] network_id network id
   * @return ResultCode
   */
  virtual ResultCode GetNetworkId(
      char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const override;

  /**
   * @brief Get analyzed data
   * @param [out] data analyzed data
   * @return ResultCode
   */
  ResultCode GetAnalyzedData(DetectionData &data) const;

  /**
   * @brief Get input tensor width and height
   * @param [out] width input tensor width
   * @param [out] height input tensor height
   * @return ResultCode
   */
  ResultCode GetInputTensorSize(uint16_t &width, uint16_t &height) const;

  /**
   * @brief Get parameter
   * @param [out] param parameter
   * @return ResultCode
   */
  ResultCode GetParam(PPLParam &param) const;

 protected:
  /**
   * @brief Clear parameter
   */
  virtual void DoClearValidatingParam() override;

 private:
  /**
   * @brief Set network id
   * @param [in] network_id network id
   * @return ResultCode
   */
  ResultCode SetNetworkId(const char *network_id);

  /**
   * @brief Initialize detection parameter
   * @param [in] root_value parameter json
   * @param [out] p_param parsed parameter
   * @return ResultCode
   */
  ResultCode ObjectDetectionParamInit(JSON_Value *root_value,
                                      AnalyzerOd::PPLParam *p_param);

  /**
   * @brief Parse Output Tensor data
   * @param [in] data_body Output Tensor data
   * @param [in] num_elements number of elements
   * @param [out] objectdetection_output parsed output tensor data
   */
  void createDetectionData(
      const float *data_body, uint32_t num_elements,
      AnalyzerOd::DetectionOutputTensor *objectdetection_output);

  /**
   * @brief Analyze Output Tensor data
   * @param [in] out_tensor parsed Output Tensor data
   * @param [out] output_objectdetection_data analyzed Output Tensor data
   * @param [in] param parameter
   * @param [in] trace_id id for linking the post-processing of each AI
   */
  void analyzeDetectionOutput(
      AnalyzerOd::DetectionOutputTensor out_tensor,
      AnalyzerOd::DetectionData *output_objectdetection_data,
      AnalyzerOd::PPLParam param, uint64_t trace_id);

  /**
   * @brief Create Flatbuffers serialized data
   * @param [out] builder Flatbuffers builder
   * @param [in] detection_data analyzed Output Tensor data
   */
  void createSSDOutputFlatbuffer(
      flatbuffers::FlatBufferBuilder *builder,
      const AnalyzerOd::DetectionData *detection_data);
};

/**
 * @class AnalyzerIc
 * @brief Image Classification analysis class
 */
class AnalyzerIc : public AnalyzerBase {
 private:
  /**
   * @struct ClassificationOutputTensor
   * @brief Output Tensor data
   */
  typedef struct tagClassificationOutputTensor {
    std::vector<float> scores_;
  } ClassificationOutputTensor;

 public:
  /**
   * @struct PPLParam
   * @brief Image Classification PPL Parameter
   */
  typedef struct tagPPLParam {
    uint16_t max_predictions_ = 0;
  } PPLParam;

  /**
   * @struct ClassificationItem
   * @brief Parsed Output Tensor data
   */
  typedef struct tagClassificationItem {
    int index_ = 0;
    float score_ = 0.0;
  } ClassificationItem;

  /**
   * @struct ClassificationData
   * @brief Analyzed data
   */
  typedef struct tagClassificationData {
    std::vector<ClassificationItem> v_classItem_;
    uint64_t trace_id_ = 0;
  } ClassificationData;

 private:
  ClassificationData data_;
  PPLParam param_;
  PPLParam *validating_param_ = nullptr;
  char network_id_[AI_MODEL_BUNDLE_ID_SIZE] = {0};

 public:
  /**
   * @brief Constructor
   */
  AnalyzerIc() : AnalyzerBase() {}

  /**
   * @brief Destructor
   */
  virtual ~AnalyzerIc() {
    if (validating_param_) {
      delete validating_param_;
    }
  }

  /**
   * @brief Validate parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode ValidateParam(const void *param) override;

  /**
   * @brief Set parameter
   * @param [in] param parameter
   * @return ResultCode
   */
  virtual ResultCode SetValidatedParam(const void *param) override;

  /**
   * @brief Clear parameter
   * @return ResultCode
   */
  virtual ResultCode ClearValidatingParam() override;

  /**
   * @brief Analyze the Output Tensor
   * @param [in] p_data output tensor data
   * @param [in] size output tensor size
   * @param [in] trace_id id for linking the post-processing of each AI
   * @return ResultCode
   */
  virtual ResultCode Analyze(const float *p_data, uint32_t size,
                             uint64_t trace_id) override;

  /**
   * @brief Serialize analyzed data
   * @param [out] out_buf serialized data
   * @param [out] size serialized data size
   * @param [in] allocator instance of Allocator
   * @return ResultCode
   */
  virtual ResultCode Serialize(void **out_buf, uint32_t *size,
                               const Allocator *allocator) override;

  /**
   * @brief Get network id
   * @param [out] network_id network id
   * @return ResultCode
   */
  virtual ResultCode GetNetworkId(
      char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const override;

  /**
   * @brief Get analyzed data
   * @param [out] data analyzed data
   * @return ResultCode
   */
  ResultCode GetAnalyzedData(ClassificationData &data) const;

  /**
   * @brief Get parameter
   * @param [out] param parameter
   * @return ResultCode
   */
  ResultCode GetParam(PPLParam &param) const;

 protected:
  /**
   * @brief Clear parameter
   */
  virtual void DoClearValidatingParam() override;

 private:
  /**
   * @brief Set network id
   * @param [in] network_id network id
   * @return ResultCode
   */
  ResultCode SetNetworkId(const char *network_id);

  /**
   * @brief Initialize parameter
   * @param [in] root_value parameter json
   * @param [out] p_cls_param parsed parameter
   * @return ResultCode
   */
  ResultCode ClassificationParamInit(JSON_Value *root_value,
                                     PPLParam *p_cls_param);

  /**
   * @brief Parse Output Tensor data
   * @param [in] data_body Output Tensor data
   * @param [in] data_size total number of classes
   * @param [out] classification_output parsed output tensor data
   */
  void createClassificationData(
      const float *data_body, uint32_t data_size,
      ClassificationOutputTensor *classification_output);

  /**
   * @brief Analyze Output Tensor data
   * @param [in] out_tensor parsed Output Tensor data
   * @param [out] output_classification_data analyzed Output Tensor data
   * @param [in] cls_param parameter
   * @param [in] trace_id id for linking the post-processing of each AI
   */
  void analyzeClassificationOutput(
      ClassificationOutputTensor out_tensor,
      ClassificationData *output_classification_data, PPLParam cls_param,
      uint64_t trace_id);

  /**
   * @brief Create Flatbuffers serialized data
   * @param [out] builder Flatbuffers builder
   * @param [in] classificationData analyzed Output Tensor data
   */
  void createClassificationFlatbuffer(
      flatbuffers::FlatBufferBuilder *builder,
      const ClassificationData *classificationData);
};

#endif  // _ANALYZER_SWITCH_DNN_H_
