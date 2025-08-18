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
#pragma once

#include "edgeapp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reset global mock state (call before each test)
void reset_mock_core_state(void);
void reset_mock_outputs(void);
void set_mock_tensor_data(const float *values, size_t count);
void setLoadModelResult(EdgeAppCoreResult result);
void setITsendResult(EdgeAppCoreResult result);
void setUnloadModelResult(EdgeAppCoreResult result);
void setGetOutputResult(bool result);
void setGetInputResult(bool result);
void setProcessResult(bool result);
int wasEdgeAppCoreLoadModelCalled();
int wasEdgeAppCoreProcessCalled();
int wasEdgeAppCoreGetOutputCalled();
int wasEdgeAppCoreGetInputCalled();
int wasEdgeAppCoreUnloadModelCalled();
int wasEdgeAppCoreSendInputTensorCalled();

#ifdef __cplusplus
}
#endif
