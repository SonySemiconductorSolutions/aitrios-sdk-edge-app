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
#include "receive_data_private.h"

#ifdef __cplusplus
extern "C" {
#endif

EdgeAppLibReceiveDataResult initialize_result =
    EdgeAppLibReceiveDataResultSuccess;
EdgeAppLibReceiveDataResult uninitialize_result =
    EdgeAppLibReceiveDataResultSuccess;
EdgeAppLibReceiveDataResult receive_data_result =
    EdgeAppLibReceiveDataResultSuccess;

void setReceiveDataInitializeResult(EdgeAppLibReceiveDataResult result) {
  initialize_result = result;
}

void resetReceiveDataInitializeResult() {
  initialize_result = EdgeAppLibReceiveDataResultSuccess;
}

void setReceiveDataUninitializeResult(EdgeAppLibReceiveDataResult result) {
  uninitialize_result = result;
}

void resetReceiveDataUninitializeResult() {
  uninitialize_result = EdgeAppLibReceiveDataResultSuccess;
}

void setReceiveDataResult(EdgeAppLibReceiveDataResult result) {
  receive_data_result = result;
}

void resetReceiveDataResult() {
  receive_data_result = EdgeAppLibReceiveDataResultSuccess;
}

EdgeAppLibReceiveDataResult EdgeAppLibReceiveDataInitialize(void *evp_client) {
  return initialize_result;
}

EdgeAppLibReceiveDataResult EdgeAppLibReceiveDataUnInitialize() {
  return uninitialize_result;
}

EdgeAppLibReceiveDataResult EdgeAppLibReceiveData(
    EdgeAppLibReceiveDataInfo *info, int timeout_ms) {
  return receive_data_result;
}

const char *EdgeAppLibReceiveDataStorePath() { return "/tmp/edge_model_test"; }

#ifdef __cplusplus
}
#endif
