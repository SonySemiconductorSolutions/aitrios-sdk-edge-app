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

#ifndef LP_RECOG_UTILS_HPP
#define LP_RECOG_UTILS_HPP

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "data_processor_api.hpp"
#include "edgeapp_core.h"
#include "parson.h"
#include "sensor.h"

#define DEFAULT_INPUT_TENSOR_WIDTH_IMX500 (300)
#define DEFAULT_INPUT_TENSOR_HEIGHT_IMX500 (300)
#define DEFAULT_THRESHOLD_IMX500 (0.3)
#define DEFAULT_MAX_DETECTIONS_IMX500 (200)
#define DEFAULT_THRESHOLD_CPU (0.5)

// Constants for license plate recognition processing
const size_t xmin_index = 1;
const size_t ymin_index = 0;
const size_t xmax_index = 3;
const size_t ymax_index = 2;
// Each prediction has 6 values
const size_t lpr_values_per_prediction = 6;
// Limit processing to first 10 predictions
const size_t lpr_max_predictions_to_process = 10;
// xmin, ymin, xmax, ymax
const size_t lpr_prediction_coordinates_count = 4;
// Score is at index 0 in prediction data
const size_t lpr_prediction_score_index = 0;
// Class ID is at index 5 in prediction data
const size_t lpr_prediction_class_index = 5;

typedef struct {
  uint16_t max_detections;
  float threshold;
  uint16_t input_width;
  uint16_t input_height;
  bool bbox_normalized;
} DataProcessorCustomParam_LPD;

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

DataProcessorResultCode ExtractMaxDetectionsIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param);
DataProcessorResultCode ExtractThresholdIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param);
DataProcessorResultCode ExtractInputHeightIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param);
DataProcessorResultCode ExtractInputWidthIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param);
DataProcessorResultCode VerifyConstraintsIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param);
DataProcessorResultCode ExtractBboxNormIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param);

struct LPAnalysisParam {
  EdgeAppLibSensorImageCropProperty *roi;
  EdgeAppCore::Tensor *tensor;
};

struct Prediction {
  float x_min;
  float y_min;
  float x_max;
  float y_max;
  float score;
  int category;
};

struct LPContent {
  std::string kanji = "?";
  std::string number_small_1 = "?";
  std::string number_small_2 = "?";
  std::string number_small_3 = "?";
  std::string hiragana = "?";
  std::string number_large_1 = "?";
  std::string number_large_2 = "?";
  std::string number_large_3 = "?";
  std::string number_large_4 = "?";
  std::string number_large_5 = "?";
};

struct LPDataProcessorAnalyzeParam {
  void *app_specific;
};

/**
 * Post processing function for LPD
 *
 * @param in_data Output tensor as an array of floats.
 * @param in_size Output tensor array of floats size(bytes).
 * @param param Pointer to the app specific parameters.
 * @return DataProcessorResultCode
 */
DataProcessorResultCode LPDDataProcessorAnalyze(
    float *in_data, uint32_t in_size,
    LPDataProcessorAnalyzeParam *param = nullptr);

/**
 * Post processing function for LPR
 *
 * @param in_data Output tensor as an array of floats.
 * @param in_size Output tensor array of floats size(bytes).
 * @param out_data Pointer of postprocessed data (JSON or BASE64).
 * @param out_size Pointer of postprocessed data size.
 * @param param Pointer to the app specific parameters.
 * @return DataProcessorResultCode
 */
DataProcessorResultCode LPRDataProcessorAnalyze(float *in_data,
                                                uint32_t in_size,
                                                char **out_data,
                                                uint32_t *out_size);

Detections *CreateLPDetections(float *in_data, uint32_t in_size,
                               DataProcessorCustomParam_LPD detection_param,
                               EdgeAppCore::Tensor *tensor);

void FilterByParams(Detections **detections,
                    DataProcessorCustomParam_LPD detection_param);

void filter_predictions_by_score(std::vector<Prediction> &predictions,
                                 float threshold);

std::string interpret_predictions(const std::vector<Prediction> &predictions);
bool is_valid_japanese_number_plate(const char *plate_data);

// Helper function declarations
std::vector<Prediction> sort_by_xmin(
    const std::vector<Prediction> &predictions);
LPContent assign_kanji(LPContent lp,
                       const std::vector<int> &upper_categories_int);
LPContent assign_hiragana(LPContent lp, const std::vector<int> &categories_int);
LPContent assign_numbers(LPContent lp, const std::vector<int> &categories_int,
                         bool is_first_line);

#endif  // LP_RECOG_UTILS_HPP
