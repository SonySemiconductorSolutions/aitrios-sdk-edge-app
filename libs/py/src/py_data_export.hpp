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

#include <string>

#include "data_export_types.h"
#include "sensor.h"

void sendInputTensorSync(EdgeAppLibSensorFrame *frame);

void sendMetadata(EdgeAppLibSensorFrame *frame);

EdgeAppLibDataExportResult sendState(const std::string &topic,
                                     const std::string &state);

std::string getPortSettingsStr();

std::string getWorkspaceDirectory();

std::string formatTimestamp(uint64_t timestamp);

std::string getFileSuffix(EdgeAppLibDataExportDataType dataType);

EdgeAppLibDataExportResult sendFile(EdgeAppLibDataExportDataType dataType,
                                    const std::string &filePath,
                                    const std::string &url, int timeout_ms);

EdgeAppLibDataExportResult sendTelemetry(void *data, int datalen,
                                         int timeout_ms);
