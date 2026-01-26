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
#include "data_processor_api/mock_data_processor_api.hpp"
#include "draw.h"
#include "lp_recog_data_processor/mock_lp_recog_data_processor.hpp"
#include "mock_device.hpp"
#include "mock_draw.hpp"
#include "mock_edgecore.hpp"
#include "mock_sensor.hpp"
#include "send_data/mock_send_data.hpp"
#include "sensor.h"
#include "sm.h"

class EvenFunctionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    reset_mock_core_state();
    reset_mock_outputs();
  }

  void TearDown() override {
    // Ensure SensorCoreExit is called to clean up memory allocated by
    // SensorStreamSetProperty
    EdgeAppLib::SensorCoreExit(DUMMY_HANDLE);
  }
};

TEST_F(EvenFunctionsTest, OnCreateSuccess) {
  int res = onCreate();
  EXPECT_EQ(res, 0);
  onDestroy();
}

// OnCreate is empty so we cannot test it directly.
// TEST_F(EvenFunctionsTest, OnCreateInitFailure) {
//   setEdgeAppLibSensorCoreInitFail();
//   int res = onCreate();
//   EXPECT_EQ(res, -1);
//   EXPECT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 1);
// }

// TEST_F(EvenFunctionsTest, OnCreateOpenStreamFailure) {
//   setEdgeAppLibSensorCoreOpenStreamFail();
//   int res = onCreate();
//   EXPECT_EQ(res, -1);
//   EXPECT_EQ(wasEdgeAppLibSensorCoreInitCalled(), 1);
//   EXPECT_EQ(wasEdgeAppLibSensorCoreOpenStreamCalled(), 1);
//   onDestroy();
// }

// // OnConfigure is empty, so we cannot test it directly.
// TEST_F(EvenFunctionsTest, OnConfigureSuccess) {
//   char topic[] = "mock";
//   void *value = strdup(topic);
//   int valuesize = 10;

//   int res = onConfigure(topic, value, valuesize);
//   EXPECT_EQ(res, 0);
//   EXPECT_EQ(wasDataProcessorConfigureCalled(), 1);
//   EXPECT_EQ(wasEdgeAppLibDataExportSendStateCalled(), 1);
// }

// TEST_F(EvenFunctionsTest, OnConfigureValueNull) {
//   char topic[] = "mock";
//   void *value = NULL;
//   int valuesize = 10;

//   int res = onConfigure(topic, value, valuesize);
//   EXPECT_EQ(res, -1);
// }

// TEST_F(EvenFunctionsTest, OnConfigureDataProcessorConfigureFail) {
//   char topic[] = "mock";
//   void *value = strdup(topic);
//   int valuesize = 10;
//   setDataProcessorConfigureFail();
//   int res = onConfigure(topic, value, valuesize);
//   EXPECT_EQ(res, 0);
//   EXPECT_EQ(wasDataProcessorConfigureCalled(), 1);
//   EXPECT_EQ(wasEdgeAppLibDataExportSendStateCalled(), 1);
// }

TEST_F(EvenFunctionsTest, OnStartLoadModelError) {
  onCreate();
  setLoadModelResult(EdgeAppCoreResultFailure);
  int res = onStart();
  // lp_recog doesn't check load model result directly, returns 0
  EXPECT_EQ(res, 0);
  onDestroy();
  setLoadModelResult(EdgeAppCoreResultSuccess);
}

TEST_F(EvenFunctionsTest, OnIterateSuccess) {
  onCreate();
  onStart();
  int res = onIterate();
  EXPECT_EQ(res, 0);
  // lp_recog uses DataProcessorGetDataType()
  EXPECT_EQ(wasDataProcessorGetDataTypeCalled(), 1);
  // lp_recog uses SendDataSyncMeta
  EXPECT_EQ(wasEdgeAppLibSendDataSyncMetaCalled(), 1);
  onDestroy();
  EXPECT_EQ(wasEdgeAppCoreUnloadModelCalled(), 1);
}

TEST_F(EvenFunctionsTest, OnIterateGetOutputError) {
  onCreate();
  onStart();
  setGetOutputResult(false);
  int res = onIterate();
  EXPECT_EQ(res, -1);  // Should fail when GetOutput fails
  onDestroy();
  setGetOutputResult(true);
}

TEST_F(EvenFunctionsTest, OnIterateProcessError) {
  onCreate();
  onStart();
  setProcessResult(false);
  int res = onIterate();
  EXPECT_EQ(res, -1);  // Should fail when Process fails
  onDestroy();
  setProcessResult(true);
}

// Abort tests are not supported in the current environment.
// TEST_F(EvenFunctionsTest, OnIterateReleaseFrameError) {
//   onCreate();
//   setEdgeAppLibSensorReleaseFrameFail();
//   EXPECT_DEATH({
//     // This block is expected to call abort()
//     onIterate();
//   }, ".*");
// }

// OnStop is empty so we cannot test it directly.
//  TEST_F(EvenFunctionsTest, OnStopSuccess) {
//    onCreate();
//    int res = onStop();
//    EXPECT_EQ(res, 0);
//    EXPECT_EQ(wasEdgeAppLibSensorStopCalled(), 1);
//    onDestroy();
//  }

// TEST_F(EvenFunctionsTest, OnStopStopError) {
//   onCreate();
//   setEdgeAppLibSensorStopFail();
//   int res = onStop();
//   EXPECT_EQ(res, -1);
//   EXPECT_EQ(wasEdgeAppLibSensorStopCalled(), 1);
//   onDestroy();
// }

// TEST_F(EvenFunctionsTest, OnStartSuccess) {
//   onCreate();
//   int res = onStart();
//   EXPECT_EQ(res, 0);
//   EXPECT_EQ(wasEdgeAppLibSensorStartCalled(), 1);
//   EXPECT_EQ(wasEdgeAppLibSensorStreamGetPropertyCalled(), 1);
//   onDestroy();
// }

// TEST_F(EvenFunctionsTest, OnStartStartError) {
//   onCreate();
//   setEdgeAppLibSensorStartFail();
//   int res = onStart();
//   EXPECT_EQ(res, -1);
//   EXPECT_EQ(wasEdgeAppLibSensorStartCalled(), 1);
//   EXPECT_EQ(wasEdgeAppLibSensorStreamGetPropertyCalled(), 0);
//   onDestroy();
// }

// TEST_F(EvenFunctionsTest, OnStartGetPropertyError) {
//   onCreate();
//   setEdgeAppLibSensorStreamGetPropertyFail();
//   int res = onStart();
//   EXPECT_EQ(res, -1);
//   EXPECT_EQ(wasEdgeAppLibSensorStartCalled(), 1);
//   EXPECT_EQ(wasEdgeAppLibSensorStreamGetPropertyCalled(), 1);
//   onDestroy();
// }

TEST_F(EvenFunctionsTest, OnDestroySuccess) {
  reset_mock_core_state();
  reset_mock_outputs();
  int res = onDestroy();
  EXPECT_EQ(res, 0);
  EXPECT_EQ(wasEdgeAppCoreUnloadModelCalled(), 1);
}
