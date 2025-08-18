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

#include "lp_recog_utils.hpp"

#include <cmath>
#include <map>

#include "data_processor_api.hpp"
#include "data_processor_utils.hpp"
#include "log.h"
#include "parson.h"
#define MAX_DETECTION_DATA_SIZE (sizeof(DetectionData) * UINT16_MAX)

static const std::vector<std::string> LP_CATEGORIES_SPECIAL = {"-", "."};  // 2
static const std::vector<std::string> LP_CATEGORIES_NUMBER = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};  // 2+10=12
static const std::vector<std::string> LP_CATEGORIES_KANJI = {
    "Owari-Komaki", "Ichinomiya", "Kasugai",   "Nagoya",     "Toyohashi",
    "Mikawa",       "Okazaki",    "Toyota",    "Akita",      "Aomori",
    "Hachinohe",    "Chiba",      "Narita",    "Narashino",  "Noda",
    "Kashiwa",      "Sodegaura",  "Ehime",     "Fukui",      "Fukuoka",
    "Chikuhō",      "Kitakyūshū", "Kurume",    "Fukushima",  "Aizu",
    "Kōriyama",     "Iwaki",      "Gifu",      "Hida",       "Gunma",
    "Maebashi",     "Takasaki",   "Fukuyama",  "Hiroshima",  "Asahikawa",
    "Hakodate",     "Kitami",     "Kushiro",   "Muroran",    "Obihiro",
    "Sapporo",      "Himeji",     "Kōbe",      "Mito",       "Tsuchiura",
    "Tsukuba",      "Ishikawa",   "Kanazawa",  "Iwate",      "Hiraizumi",
    "Morioka",      "Kagawa",     "Kagoshima", "Amami",      "Sagami",
    "Shonan",       "Kawasaki",   "Yokohama",  "Kōchi",      "Kumamoto",
    "Kyōto",        "Mie",        "Suzuka",    "Miyagi",     "Sendai",
    "Miyazaki",     "Matsumoto",  "Suwa",      "Nagano",     "Nagasaki",
    "Sasebo",       "Nara",       "Nagaoka",   "Niigata",    "Ōita",
    "Okayama",      "Kurashiki",  "Okinawa",   "Izumi",      "Sakai",
    "Ōsaka",        "Naniwa",     "Saga",      "Kasukabe",   "Koshigaya",
    "Kumagaya",     "Omiya",      "Kawaguchi", "Tokorozawa", "Kawagoe",
    "Shiga",        "Shimane",    "Hamamatsu", "Numazu",     "Fujisan",
    "Izu",          "Shizuoka",   "Tochigi",   "Utsunomiya", "Nasu",
    "Tokushima",    "Adachi",     "Hachiōji",  "Tama",       "Nerima",
    "Suginami",     "Shinagawa",  "Ogasawara", "Setagaya",   "Tottori",
    "Toyama",       "Wakayama",   "Shōnai",    "Yamagata",   "Yamaguchi",
    "Shimonoseki",  "Yamanashi"};  // 2+10+117=129
static const std::vector<std::string> LP_CATEGORIES_HIRAGANA = {
    "sa", "su", "se", "so", "ta", "chi", "tsu", "te", "to",
    "na", "ni",  // 129+11=140
    "nu", "ne", "no", "ha", "hi", "fu",  "ho",  "ma", "mi",
    "mu", "me",  // 140+11=151
    "mo", "ya", "yu", "yo", "ra", "ri",  "ru",  "ro", "re",
    "wa", "a",                                                // 151+11=162
    "i",  "u",  "e",  "ka", "ki", "ku",  "ke",  "ko", "wo"};  // 162+9=171

static const std::vector<std::string> CATEGORIES = [] {
  std::vector<std::string> all_categories;
  all_categories.insert(all_categories.end(), LP_CATEGORIES_SPECIAL.begin(),
                        LP_CATEGORIES_SPECIAL.end());
  all_categories.insert(all_categories.end(), LP_CATEGORIES_NUMBER.begin(),
                        LP_CATEGORIES_NUMBER.end());
  all_categories.insert(all_categories.end(), LP_CATEGORIES_KANJI.begin(),
                        LP_CATEGORIES_KANJI.end());
  all_categories.insert(all_categories.end(), LP_CATEGORIES_HIRAGANA.begin(),
                        LP_CATEGORIES_HIRAGANA.end());
  return all_categories;
}();

std::vector<Prediction> sort_by_xmin(
    const std::vector<Prediction> &predictions) {
  std::vector<Prediction> sorted_predictions = predictions;
  std::sort(sorted_predictions.begin(), sorted_predictions.end(),
            [](const Prediction &a, const Prediction &b) {
              return a.x_min < b.x_min;
            });
  return sorted_predictions;
}

static void split_lines_by_y(const std::vector<Prediction> &predictions,
                             std::vector<Prediction> &upper,
                             std::vector<Prediction> &lower) {
  if (predictions.empty()) return;

  // Sort by y_min ascending
  std::vector<Prediction> sorted = predictions;
  std::sort(sorted.begin(), sorted.end(),
            [](const Prediction &a, const Prediction &b) {
              return a.y_min < b.y_min;
            });

  // First 4 go to upper, rest to lower
  size_t upper_count = std::min<size_t>(4, sorted.size());
  upper.assign(sorted.begin(), sorted.begin() + upper_count);
  lower.assign(sorted.begin() + upper_count, sorted.end());
}

// Helper: assign kanji from upper line
static void assign_kanji(const std::vector<Prediction> &upper, LPContent &lp) {
  for (const auto &p : upper) {
    if (p.category >= 12 && p.category < 12 + LP_CATEGORIES_KANJI.size()) {
      lp.kanji = CATEGORIES[p.category];
      break;
    }
  }
}

// Helper: assign hiragana from lower line
static void assign_hiragana(const std::vector<Prediction> &lower,
                            LPContent &lp) {
  for (const auto &p : lower) {
    int base = 12 + LP_CATEGORIES_KANJI.size();
    if (p.category >= base &&
        p.category < base + LP_CATEGORIES_HIRAGANA.size()) {
      lp.hiragana = CATEGORIES[p.category];
      break;
    }
  }
}

// Helper: assign numbers from upper/lower lines
static void assign_numbers(const std::vector<Prediction> &line, LPContent &lp,
                           bool is_upper) {
  int base = 2;  // numbers start at index 2
  int count = 0;
  for (const auto &p : line) {
    if (is_upper) {
      // Only assign numbers for upper line
      if (p.category >= base && p.category < base + 10) {
        std::string val = CATEGORIES[p.category];
        if (count == 0)
          lp.number_small_1 = val;
        else if (count == 1)
          lp.number_small_2 = val;
        else if (count == 2)
          lp.number_small_3 = val;
        count++;
      }
    } else {
      // For lower line, assign special and numbers
      if (p.category >= 0 && p.category < base + 10) {
        std::string val = CATEGORIES[p.category];
        if (count == 0) {
          lp.number_large_1 = val;
        } else if (count == 1) {
          lp.number_large_2 = val;
        } else if (count == 2) {
          if (lp.number_large_1 == ".") {
            lp.number_large_3 = " ";
            lp.number_large_4 = val;
          } else {
            lp.number_large_3 = val;
          }
        } else if (count == 3) {
          if (lp.number_large_1 == ".") {
            lp.number_large_5 = val;
          } else {
            lp.number_large_4 = val;
          }
        } else if (count == 4) {
          if (lp.number_large_1 != ".") {
            lp.number_large_5 = val;
          }
        }
        count++;
      }
    }
  }
}

static inline uint16_t maybe_scale(float value, int dim, bool normalized) {
  return normalized ? (uint16_t)round(value * dim) : (uint16_t)round(value);
}

Detections *CreateLPDetections(float *in_data, uint32_t in_size,
                               DataProcessorCustomParam_LPD detection_param,
                               EdgeAppCore::Tensor *tensor) {
  Detections *detections = (Detections *)malloc(sizeof(Detections));
  if (detections == NULL) {
    LOG_ERR("Failed to allocate memory for detections.");
    return detections;
  }

  size_t num_preds = tensor->size / lpr_values_per_prediction / sizeof(float);
  LOG_DBG("Number of detections: %d", num_preds);

  float num_detections_f = in_data[num_preds * lpr_prediction_class_index];
  if (num_detections_f < 0 ||
      num_detections_f > MAX_DETECTION_DATA_SIZE / sizeof(DetectionData)) {
    LOG_ERR("num_detections value (%f) is out of valid range [0, %u].",
            num_detections_f, MAX_DETECTION_DATA_SIZE / sizeof(DetectionData));
    free(detections);
    return NULL;
  }

  detections->num_detections = static_cast<uint16_t>(num_detections_f);
  LOG_DBG("Number of Valid detections: %d", detections->num_detections);

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

  for (int i = 0; i < detections->num_detections; i++) {
    LOG_DBG(
        "LPD %zu:"
        " [xmin=%f, ymin=%f, xmax=%f, ymax=%f, score=%f, "
        "class_id=%d]",
        i, in_data[i + num_preds * (1 + xmin_index)],
        in_data[i + num_preds * (1 + ymin_index)], in_data[i + num_preds * 4],
        in_data[i + num_preds * (1 + ymax_index)],
        in_data[i + num_preds * lpr_prediction_score_index],
        static_cast<int>(
            in_data[i + num_preds * lpr_prediction_class_index + 1]));

    detections->detection_data[i].class_id = static_cast<int>(
        in_data[i + num_preds * lpr_prediction_class_index + 1]);
    detections->detection_data[i].score =
        in_data[i + num_preds * lpr_prediction_score_index];

    const int w = detection_param.input_width - 1;
    const int h = detection_param.input_height - 1;
    bool normalized = detection_param.bbox_normalized;

    // Extract bbox coordinates
    detections->detection_data[i].bbox.left =
        maybe_scale((in_data[i + num_preds * (1 + xmin_index)]), w, normalized);
    detections->detection_data[i].bbox.top =
        maybe_scale((in_data[i + num_preds * (1 + ymin_index)]), h, normalized);
    detections->detection_data[i].bbox.right =
        maybe_scale((in_data[i + num_preds * (1 + xmax_index)]), w, normalized);
    detections->detection_data[i].bbox.bottom =
        maybe_scale((in_data[i + num_preds * (1 + ymax_index)]), h, normalized);

    LOG_DBG(
        "LPD %zu: bbox=[left=%d, top=%d, right=%d, bottom=%d], class_id=%d, "
        "score=%f",
        i, detections->detection_data[i].bbox.left,
        detections->detection_data[i].bbox.top,
        detections->detection_data[i].bbox.right,
        detections->detection_data[i].bbox.bottom,
        detections->detection_data[i].class_id,
        detections->detection_data[i].score);
  }

  return detections;
}
void filter_predictions_by_score(std::vector<Prediction> &predictions,
                                 float threshold) {
  predictions.erase(std::remove_if(predictions.begin(), predictions.end(),
                                   [threshold](const Prediction &p) {
                                     return p.score <= threshold;
                                   }),
                    predictions.end());
}

// Main conversion from predictions to formatted string
std::string interpret_predictions(const std::vector<Prediction> &predictions) {
  if (predictions.empty()) return "";
  auto sorted = sort_by_xmin(predictions);

  std::vector<Prediction> upper, lower;
  split_lines_by_y(sorted, upper, lower);
  // Sort each line by x_min
  upper = sort_by_xmin(upper);
  lower = sort_by_xmin(lower);

  LOG_DBG("Upper line predictions:");
  for (const auto &p : upper) {
    LOG_DBG("  [%f, %f, %f, %f] score: %f, category: %d", p.x_min, p.y_min,
            p.x_max, p.y_max, p.score, p.category);
  }
  LOG_DBG("Lower line predictions:");
  for (const auto &p : lower) {
    LOG_DBG("  [%f, %f, %f, %f] score: %f, category: %d", p.x_min, p.y_min,
            p.x_max, p.y_max, p.score, p.category);
  }

  LPContent lp;
  assign_kanji(upper, lp);
  assign_hiragana(lower, lp);
  assign_numbers(upper, lp, true);
  assign_numbers(lower, lp, false);
  // Format output
  std::string upper_text = lp.kanji + " " + lp.number_small_1 +
                           lp.number_small_2 + lp.number_small_3;
  std::string lower_text = lp.hiragana + " " + lp.number_large_1 +
                           lp.number_large_2 + lp.number_large_3 +
                           lp.number_large_4 + lp.number_large_5;
  return upper_text + ", " + lower_text;
}

DataProcessorResultCode ExtractThresholdIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "threshold", &aux) == 0) {
    if (aux < 0.0 || aux > 1.0) {
      LOG_INFO("DataProcessorConfigure: threshold value out of range");
      return kDataProcessorOutOfRange;
    }
    detection_param_pr->threshold = (float)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->threshold = DEFAULT_THRESHOLD_IMX500;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'threshold' parameter "
      "is %f",
      DEFAULT_THRESHOLD_IMX500);
  json_object_set_number(json, "threshold", DEFAULT_THRESHOLD_IMX500);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractMaxDetectionsIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "max_detections", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    detection_param_pr->max_detections = (uint16_t)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->max_detections = DEFAULT_MAX_DETECTIONS_IMX500;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'max_detections' parameter "
      "is %d",
      DEFAULT_MAX_DETECTIONS_IMX500);
  json_object_set_number(json, "max_detections", DEFAULT_MAX_DETECTIONS_IMX500);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractInputHeightIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_height", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    detection_param_pr->input_height = (uint16_t)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->input_height = DEFAULT_INPUT_TENSOR_HEIGHT_IMX500;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_height' parameter "
      "is %d",
      DEFAULT_INPUT_TENSOR_HEIGHT_IMX500);
  json_object_set_number(json, "input_height",
                         DEFAULT_INPUT_TENSOR_HEIGHT_IMX500);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractInputWidthIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_width", &aux) == 0) {
    if (aux < 0) return kDataProcessorOutOfRange;
    detection_param_pr->input_width = (uint16_t)aux;
    return kDataProcessorOk;
  }
  detection_param_pr->input_width = DEFAULT_INPUT_TENSOR_WIDTH_IMX500;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_width' parameter "
      "is %d",
      DEFAULT_INPUT_TENSOR_WIDTH_IMX500);
  json_object_set_number(json, "input_width",
                         DEFAULT_INPUT_TENSOR_WIDTH_IMX500);
  return kDataProcessorInvalidParam;
}

DataProcessorResultCode ExtractBboxNormIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param_pr) {
  bool aux = false;
  if (GetValueBoolean(json, "bbox_normalization", &aux) == 0) {
    detection_param_pr->bbox_normalized = aux;
    return kDataProcessorOk;
  }
  // Default value
  detection_param_pr->bbox_normalized = true;
  return kDataProcessorOk;
}

DataProcessorResultCode VerifyConstraintsIMX500(
    JSON_Object *json, DataProcessorCustomParam_LPD *detection_param_pr) {
  if (detection_param_pr->threshold < 0.0 ||
      detection_param_pr->threshold > 1.0) {
    LOG_WARN("threshold value out of range, set to default threshold");
    detection_param_pr->threshold = DEFAULT_THRESHOLD_IMX500;
    json_object_set_number(json, "threshold", detection_param_pr->threshold);
    return kDataProcessorInvalidParam;
  }
  return kDataProcessorOk;
}

void FilterByParams(Detections **detections,
                    DataProcessorCustomParam_LPD detection_param) {
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
