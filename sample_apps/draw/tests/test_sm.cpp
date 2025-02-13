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

#include <gtest/gtest.h>

#include "data_export/mock_data_export.hpp"
#include "mock_sensor.hpp"
#include "sensor.h"
#include "sm.h"

class EvenFunctionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetEdgeAppLibSensorCoreInitCalled();
    resetEdgeAppLibSensorCoreOpenStreamCalled();
    resetEdgeAppLibSensorCoreInitSuccess();
    resetEdgeAppLibSensorCoreOpenStreamSuccess();
    resetEdgeAppLibDataExportSendStateCalled();
    resetEdgeAppLibDataExportIsEnabled();
    resetEdgeAppLibSensorChannelGetRawDataCalled();
    resetEdgeAppLibSensorChannelGetRawDataSuccess();
    resetEdgeAppLibSensorFrameGetChannelFromChannelIdSuccess();
    resetEdgeAppLibSensorFrameGetChannelFromChannelIdCalled();
    resetEdgeAppLibSensorStreamGetPropertySuccess();
    resetEdgeAppLibSensorStreamSetPropertySuccess();
    resetEdgeAppLibSensorStopSuccess();
    resetEdgeAppLibSensorStartSuccess();
    resetEdgeAppLibSensorStreamGetPropertyCalled();
    resetEdgeAppLibSensorStreamSetPropertyCalled();
    resetEdgeAppLibSensorStopCalled();
    resetEdgeAppLibSensorStartCalled();
    resetEdgeAppLibSensorCoreCloseStreamSuccess();
    resetEdgeAppLibSensorCoreExitSuccess();
  }

  void TearDown() override {}
};

TEST_F(EvenFunctionsTest, OnCreateSuccess) {
  int res = onCreate();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreOpenStreamCalled(), 1);
  extern EdgeAppLibSensorCore s_core;
  extern EdgeAppLibSensorStream stream_check;
  extern void *s_frame_received_callback;
  EXPECT_EQ(s_core, (EdgeAppLibSensorCore)DUMMY_HANDLE);
  EXPECT_EQ(stream_check, (EdgeAppLibSensorStream)DUMMY_HANDLE);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnCreateInitFailure) {
  setEdgeAppLibSensorCoreInitFail();
  int res = onCreate();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 1);
}

TEST_F(EvenFunctionsTest, OnCreateOpenStreamFailure) {
  setEdgeAppLibSensorCoreOpenStreamFail();
  int res = onCreate();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreOpenStreamCalled(), 1);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnConfigureSuccess) {
  char topic[] = "mock";
  void *value = strdup(topic);
  int valuesize = 10;

  int res = onConfigure(topic, value, valuesize);
  EXPECT_EQ(res, 0);
  free(value);
}

TEST_F(EvenFunctionsTest, OnConfigureValueNull) {
  char topic[] = "mock";
  void *value = NULL;
  int valuesize = 10;

  int res = onConfigure(topic, value, valuesize);
  EXPECT_EQ(res, 0);
}

TEST_F(EvenFunctionsTest, OnIterateSuccess) {
  onCreate();
  int res = onIterate();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorGetFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorChannelGetRawDataCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorReleaseFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportAwaitCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportCleanupCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportSendDataCalled(), 1);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnIterateChannelError) {
  onCreate();
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
  int res = onIterate();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorGetFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorChannelGetRawDataCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibSensorReleaseFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportAwaitCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportCleanupCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportSendDataCalled(), 0);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnIterateRawDataError) {
  onCreate();
  setEdgeAppLibSensorChannelGetRawDataFail();
  int res = onIterate();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorGetFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorChannelGetRawDataCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorReleaseFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportAwaitCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportCleanupCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportSendDataCalled(), 0);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnIterateGetFrameError) {
  onCreate();
  setEdgeAppLibSensorGetFrameFail();
  int res = onIterate();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorGetFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibSensorChannelGetRawDataCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibSensorReleaseFrameCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportAwaitCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportCleanupCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportSendDataCalled(), 0);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnIterateReleaseFrameError) {
  onCreate();
  setEdgeAppLibSensorReleaseFrameFail();
  int res = onIterate();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorGetFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorChannelGetRawDataCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorReleaseFrameCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportAwaitCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportCleanupCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibDataExportSendDataCalled(), 1);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnIterateDataExportDisabled) {
  onCreate();
  setEdgeAppLibDataExportIsEnabledDisabled();
  int res = onIterate();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorGetFrameCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibSensorFrameGetChannelFromChannelIdCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibSensorChannelGetRawDataCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibSensorReleaseFrameCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportAwaitCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportCleanupCalled(), 0);
  EXPECT_EQ(wasEdgeAppLibDataExportSendDataCalled(), 0);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnStopSuccess) {
  onCreate();
  int res = onStop();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorStopCalled(), 1);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnStopStopError) {
  onCreate();
  setEdgeAppLibSensorStopFail();
  int res = onStop();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorStopCalled(), 1);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnStartSuccess) {
  onCreate();
  int res = onStart();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorStartCalled(), 1);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnStartStartError) {
  onCreate();
  setEdgeAppLibSensorStartFail();
  int res = onStart();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorStartCalled(), 1);
  onDestroy();
}

TEST_F(EvenFunctionsTest, OnDestroySuccess) {
  int res = onDestroy();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppLibSensorCoreCloseStreamCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreExitCalled(), 1);
}

TEST_F(EvenFunctionsTest, OnDestroyCloseStreamError) {
  setEdgeAppLibSensorCoreCloseStreamFail();
  int res = onDestroy();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreCloseStreamCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreExitCalled(), 0);
}

TEST_F(EvenFunctionsTest, OnDestroyCoreExit) {
  setEdgeAppLibSensorCoreExitFail();
  int res = onDestroy();
  EXPECT_EQ(res, -1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreCloseStreamCalled(), 1);
  EXPECT_EQ(wasEdgeAppLibSensorCoreExitCalled(), 1);
}
