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

#include "data_processor_api/mock_data_processor_api.hpp"

#include <string.h>

static int DataProcessorConfigureCalled = 0;
static DataProcessorResultCode DataProcessorConfigureReturn = kDataProcessorOk;
static int DataProcessorAnalyzeCalled = 0;
static DataProcessorResultCode DataProcessorAnalyzeReturn = kDataProcessorOk;
static int DataProcessorJsonFormatCalled = 0;
static DataProcessorResultCode DataProcessorJsonFormatReturn = kDataProcessorOk;
static EdgeAppLibSendDataType DataProcessorGetDataTypeReturn =
    EdgeAppLibSendDataBase64;
static int DataProcessorGetDataTypeCalled = 0;

DataProcessorResultCode DataProcessorConfigure(char *config_json,
                                               char **out_config_json) {
  DataProcessorConfigureCalled = 1;
  if (DataProcessorConfigureReturn != kDataProcessorOk)
    *out_config_json = strdup("testing");
  return DataProcessorConfigureReturn;
}

int wasDataProcessorConfigureCalled() { return DataProcessorConfigureCalled; }
void setDataProcessorConfigureFail() {
  DataProcessorConfigureReturn = kDataProcessorInvalidParam;
}
void resetDataProcessorConfigureSuccess() {
  DataProcessorConfigureReturn = kDataProcessorOk;
}
void resetDataProcessorConfigureCalled() { DataProcessorConfigureCalled = 0; }

DataProcessorResultCode DataProcessorAnalyze(float *in_data, uint32_t in_size,
                                             char **out_data,
                                             uint32_t *out_size) {
  DataProcessorAnalyzeCalled = 1;
  return DataProcessorAnalyzeReturn;
}

EdgeAppLibSendDataType DataProcessorGetDataType() {
  DataProcessorGetDataTypeCalled = 1;
  return EdgeAppLibSendDataBase64;
}

int wasDataProcessorGetDataTypeCalled() {
  return DataProcessorGetDataTypeCalled;
}

int wasDataProcessorAnalyzeCalled() { return DataProcessorAnalyzeCalled; }
void setDataProcessorAnalyzeFail() {
  DataProcessorAnalyzeReturn = kDataProcessorInvalidParam;
}
void resetDataProcessorAnalyzeSuccess() {
  DataProcessorAnalyzeReturn = kDataProcessorOk;
}
void resetDataProcessorAnalyzeCalled() { DataProcessorAnalyzeCalled = 0; }

DataProcessorResultCode DataProcessorJsonFormat(void *in_data, uint32_t in_size,
                                                uint64_t timestamp,
                                                char **out_data,
                                                uint32_t *out_size) {
  DataProcessorJsonFormatCalled = 1;
  return DataProcessorJsonFormatReturn;
}

int wasDataProcessorJsonFormatCalled() { return DataProcessorAnalyzeCalled; }
void setDataProcessorJsonFormatFail() {
  DataProcessorJsonFormatReturn = kDataProcessorInvalidParam;
}
void resetDataProcessorJsonFormatSuccess() {
  DataProcessorJsonFormatReturn = kDataProcessorOk;
}
void resetDataProcessorJsonFormatCalled() { DataProcessorJsonFormatCalled = 0; }
