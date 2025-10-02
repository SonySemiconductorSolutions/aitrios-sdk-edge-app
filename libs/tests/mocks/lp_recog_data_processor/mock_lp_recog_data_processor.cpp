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

#include "lp_recog_data_processor/mock_lp_recog_data_processor.hpp"

#include <stdlib.h>
#include <string.h>

// Forward declarations for types used in lp_recog
struct LPDataProcessorAnalyzeParam {
  void *app_specific;
};

// Global variables used by lp_recog
bool LPRDataProcessorAnalyzeReturnValidData = true;
char lpd_imx500_model_id[128];

// Mock functions for LPD and LPR data processor analyze functions
static int LPDDataProcessorAnalyzeCalled = 0;
static DataProcessorResultCode LPDDataProcessorAnalyzeReturn = kDataProcessorOk;
static int LPRDataProcessorAnalyzeCalled = 0;
static DataProcessorResultCode LPRDataProcessorAnalyzeReturn = kDataProcessorOk;

DataProcessorResultCode LPDDataProcessorAnalyze(
    float *in_data, uint32_t in_size, LPDataProcessorAnalyzeParam *param) {
  LPDDataProcessorAnalyzeCalled++;

  // Mock implementation - just return success
  (void)in_data;
  (void)in_size;
  (void)param;

  return LPDDataProcessorAnalyzeReturn;
}

DataProcessorResultCode LPRDataProcessorAnalyze(float *in_data,
                                                uint32_t in_size,
                                                char **out_data,
                                                uint32_t *out_size) {
  LPRDataProcessorAnalyzeCalled++;

  // Create a simple mock response
  if (out_data && out_size) {
    const char *mock_response;
    if (LPRDataProcessorAnalyzeReturnValidData) {
      mock_response =
          "Mock 589, ra 52-04";  // Valid Japanese number plate format
    } else {
      mock_response =
          "Invalid?Plate--123";  // Invalid format (contains ? and --)
    }

    *out_data = (char *)malloc(strlen(mock_response) + 1);
    strcpy(*out_data, mock_response);
    *out_size = strlen(mock_response) + 1;
  }

  return LPRDataProcessorAnalyzeReturn;
}

bool is_valid_japanese_number_plate(const char *plate_data) {
  if (plate_data == nullptr) {
    return false;
  }
  // Simple Checking for invalid patterns that the real function would reject
  if (strstr(plate_data, "?") != nullptr ||
      strstr(plate_data, "--") != nullptr) {
    return false;
  }
  return true;
}

// Reset and check functions for LPD/LPR analyze
void resetLPDDataProcessorAnalyzeCalled() { LPDDataProcessorAnalyzeCalled = 0; }
int wasLPDDataProcessorAnalyzeCalled() { return LPDDataProcessorAnalyzeCalled; }
void setLPDDataProcessorAnalyzeFail() {
  LPDDataProcessorAnalyzeReturn = kDataProcessorInvalidParam;
}
void resetLPDDataProcessorAnalyzeSuccess() {
  LPDDataProcessorAnalyzeReturn = kDataProcessorOk;
}

void resetLPRDataProcessorAnalyzeCalled() { LPRDataProcessorAnalyzeCalled = 0; }
int wasLPRDataProcessorAnalyzeCalled() { return LPRDataProcessorAnalyzeCalled; }
void setLPRDataProcessorAnalyzeFail() {
  LPRDataProcessorAnalyzeReturn = kDataProcessorInvalidParam;
}
void resetLPRDataProcessorAnalyzeSuccess() {
  LPRDataProcessorAnalyzeReturn = kDataProcessorOk;
}
void setLPRDataProcessorAnalyzeReturnValid(bool valid) {
  LPRDataProcessorAnalyzeReturnValidData = valid;
}
