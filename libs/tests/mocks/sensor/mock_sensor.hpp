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

#ifndef MOCKS_MOCK_AITRIOS_SENSOR_HPP
#define MOCKS_MOCK_AITRIOS_SENSOR_HPP

#include <cstddef>
#include <cstdint>

#include "sensor.h"

static const int32_t DUMMY_HANDLE = 0x1234;

int wasEdgeAppLibSensorCoreInitCalled();
void setEdgeAppLibSensorCoreInitFail();
void resetEdgeAppLibSensorCoreInitSuccess();
void resetEdgeAppLibSensorCoreInitCalled();

int wasEdgeAppLibSensorCoreExitCalled();
void setEdgeAppLibSensorCoreExitFail();
void resetEdgeAppLibSensorCoreExitSuccess();
void resetEdgeAppLibSensorCoreExitCalled();

int wasEdgeAppLibSensorCoreOpenStreamCalled();
void setEdgeAppLibSensorCoreOpenStreamFail();
void resetEdgeAppLibSensorCoreOpenStreamSuccess();
void resetEdgeAppLibSensorCoreOpenStreamCalled();

int wasEdgeAppLibSensorReleaseFrameCalled();
void setEdgeAppLibSensorReleaseFrameFail();
void resetEdgeAppLibSensorReleaseFrameSuccess();
void resetEdgeAppLibSensorReleaseFrameCalled();

int wasEdgeAppLibSensorGetFrameCalled();
void setEdgeAppLibSensorGetFrameFail();
void resetEdgeAppLibSensorGetFrameSuccess();
void resetEdgeAppLibSensorGetFrameCalled();

int wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled();
void setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
void resetEdgeAppLibSensorFrameGetChannelFromChannelIdSuccess();
void resetEdgeAppLibSensorFrameGetChannelFromChannelIdCalled();

int wasEdgeAppLibSensorChannelGetRawDataCalled();
void setEdgeAppLibSensorChannelGetRawDataFail();
void resetEdgeAppLibSensorChannelGetRawDataSuccess();
void resetEdgeAppLibSensorChannelGetRawDataCalled();

int wasEdgeAppLibSensorChannelGetPropertyCalled();
void setEdgeAppLibSensorChannelGetPropertyFail();
void resetEdgeAppLibSensorChannelGetPropertySuccess();
void resetEdgeAppLibSensorChannelGetPropertyCalled();

void setEdgeAppLibSensorChannelSubFrameCurrentNum(int num);
void setEdgeAppLibSensorChannelSubFrameDivisionNum(int num);

int wasEdgeAppLibSensorStopCalled();
void setEdgeAppLibSensorStopFail();
void resetEdgeAppLibSensorStopSuccess();
void resetEdgeAppLibSensorStopCalled();

int wasEdgeAppLibSensorStartCalled();
void setEdgeAppLibSensorStartFail();
void resetEdgeAppLibSensorStartSuccess();
void resetEdgeAppLibSensorStartCalled();

int wasEdgeAppLibSensorStreamGetPropertyCalled();
void setEdgeAppLibSensorStreamGetPropertyFail();
void resetEdgeAppLibSensorStreamGetPropertySuccess();
void resetEdgeAppLibSensorStreamGetPropertyCalled();

int wasEdgeAppLibSensorStreamSetPropertyCalled();
void setEdgeAppLibSensorStreamSetPropertyFail();
void resetEdgeAppLibSensorStreamSetPropertySuccess();
void resetEdgeAppLibSensorStreamSetPropertyCalled();

int wasEdgeAppLibSensorCoreCloseStreamCalled();
void setEdgeAppLibSensorCoreCloseStreamFail();
void resetEdgeAppLibSensorCoreCloseStreamSuccess();
void resetEdgeAppLibSensorCoreCloseStreamCalled();

int wasEdgeAppLibSensorGetLastErrorCalled();
void setEdgeAppLibSensorGetLastErrorFail();
void resetEdgeAppLibSensorGetLastErrorSuccess();
void resetEdgeAppLibSensorGetLastErrorCalled();

int wasEdgeAppLibSensorGetLastErrorStringCalled();
void setEdgeAppLibSensorGetLastErrorStringFail();
void resetEdgeAppLibSensorGetLastErrorStringSuccess();
void resetEdgeAppLibSensorGetLastErrorStringCalled();

int wasEdgeAppLibSensorGetLastErrorLevelCalled();
void setEdgeAppLibSensorGetLastErrorLevelFail();
void resetEdgeAppLibSensorGetLastErrorLevelSuccess();
void resetEdgeAppLibSensorGetLastErrorLevelCalled();

int wasEdgeAppLibSensorGetLastErrorCauseCalled();
void setEdgeAppLibSensorGetLastErrorCauseFail();
void setEdgeAppLibSensorGetLastErrorCauseFail2(
    EdgeAppLibSensorErrorCause cause);
void resetEdgeAppLibSensorGetLastErrorCauseSuccess();
void resetEdgeAppLibSensorGetLastErrorCauseCalled();

void setEdgeAppLibSensorInputDataTypeEnableChannelFail();
void resetEdgeAppLibSensorInputDataTypeEnableChannelSuccess();

void setEdgeAppLibSensorChannelImageProperty(
    EdgeAppLibSensorImageProperty property);
void resetEdgeAppLibSensorChannelImageProperty();

#endif /* MOCKS_MOCK_AITRIOS_SENSOR_HPP */
