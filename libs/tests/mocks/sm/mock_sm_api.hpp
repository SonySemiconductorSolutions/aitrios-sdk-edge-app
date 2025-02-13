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

#ifndef MOCK_AITRIOS_SM_API_H
#define MOCK_AITRIOS_SM_API_H

void setPortSettings(int method);
void setPortSettingsNoInputTensor(int method);
void setPortSettingsNoInputTensorEnabled(void);
void setPortSettingsNoMetadata(void);
void setPortSettingsNoMetadataEndpoint(void);
void setPortSettingsMetadataEndpoint(const char *endpoint, const char *path);
void setPortSettingsInputTensorEndpoint(const char *endpoint, const char *path);
void setPortSettingsMetadataDisabled(void);
void setPortSettingsInputTensorDisabled(void);
void resetPortSettings(void);
void freePortSettingsValue(void);
void setCodecSettingsFull(void);
void resetCodecSettings(void);
void freeCodecSettingsValue(void);
void setCodecSettingsFormatValue(int num);
void setNumOfInfPerMsg(int num);

#endif /* MOCK_AITRIOS_SM_API_H */
