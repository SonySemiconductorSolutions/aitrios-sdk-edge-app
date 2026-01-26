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

#ifndef MOCKS_MOCK_LP_RECOG_DATA_PROCESSOR_HPP
#define MOCKS_MOCK_LP_RECOG_DATA_PROCESSOR_HPP

#include <vector>

#include "data_processor_api.hpp"
#include "edgeapp_core.h"

// Forward declaration for LPDataProcessorAnalyzeParam
struct LPDataProcessorAnalyzeParam {
  void *app_specific;
};

// Function declarations for LPD and LPR analyze - both old and new signatures
DataProcessorResultCode LPDDataProcessorAnalyze(
    float *in_data, uint32_t in_size,
    LPDataProcessorAnalyzeParam *param = nullptr);

DataProcessorResultCode LPDDataProcessorAnalyze(
    const std::vector<EdgeAppCore::Tensor> &tensors,
    LPDataProcessorAnalyzeParam *param);

DataProcessorResultCode LPRDataProcessorAnalyze(float *in_data,
                                                uint32_t in_size,
                                                char **out_data,
                                                uint32_t *out_size);

DataProcessorResultCode LPRDataProcessorAnalyze(
    const std::vector<EdgeAppCore::Tensor> &tensors, char **out_data,
    uint32_t *out_size);

// Mock functions for LPD and LPR data processor analyze
void resetLPDDataProcessorAnalyzeCalled();
int wasLPDDataProcessorAnalyzeCalled();
void setLPDDataProcessorAnalyzeFail();
void resetLPDDataProcessorAnalyzeSuccess();

void resetLPRDataProcessorAnalyzeCalled();
int wasLPRDataProcessorAnalyzeCalled();
void setLPRDataProcessorAnalyzeFail();
void resetLPRDataProcessorAnalyzeSuccess();
void setLPRDataProcessorAnalyzeReturnValid(bool valid);

#endif /* MOCKS_MOCK_LP_RECOG_DATA_PROCESSOR_HPP */
