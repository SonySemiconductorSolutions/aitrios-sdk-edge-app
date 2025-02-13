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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_device.hpp"
#include "mock_process_format.hpp"
#include "mock_sensor.hpp"
#include "mock_sm_api.hpp"
#include "send_data.h"

using namespace EdgeAppLib;

class SendDataTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // set default.
    setNumOfInfPerMsg(1);
  }

  void TearDown() override {}
};

TEST_F(SendDataTest, SendDataSyncMeta_Normal) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);
}

TEST_F(SendDataTest, SendDataSyncMeta_Normal_SizeZero) {
  uint8_t in_data[1] = {0};
  uint32_t in_size = 0;  // size is zero.
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);
}

TEST_F(SendDataTest, SendDataSyncMeta_Normal_JsonString) {
  const char *in_data = "abcdefg";
  uint32_t in_size = 8;
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  // data type is json string.
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      (void *)in_data, in_size, EdgeAppLibSendDataJson, time_stamp, timeout_ms);
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);
}

TEST_F(SendDataTest, SendDataSyncMeta_Normal_AnotherAiModel) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  // first ai model.
  const char *ai_model_first = "1234";
  setProcessFormatMetaOutput(ai_model_first);
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);

  // different ai model.
  const char *ai_model_second = "5678";
  setProcessFormatMetaOutput(ai_model_second);
  result = SendDataSyncMeta(in_data, in_size, EdgeAppLibSendDataBase64,
                            time_stamp, timeout_ms);
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);
}

TEST_F(SendDataTest, SendDataSyncMeta_Normal_AnotherAiModelInferencesTwo) {
  // number of inferences is 2.
  setNumOfInfPerMsg(2);

  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  const char *ai_model_first = "1234";
  setProcessFormatMetaOutput(ai_model_first);
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  // first, enqueued.
  ASSERT_EQ(result, EdgeAppLibSendDataResultEnqueued);

  const char *ai_model_second = "5678";
  setProcessFormatMetaOutput(ai_model_second);
  result = SendDataSyncMeta(in_data, in_size, EdgeAppLibSendDataBase64,
                            time_stamp, timeout_ms);
  // second, send.
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);
}

TEST_F(SendDataTest, SendDataSyncMeta_Error_InDataNULL) {
  // in_data is NULL.
  uint8_t *in_data = NULL;
  uint32_t in_size = 0;
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  ASSERT_EQ(result, EdgeAppLibSendDataResultInvalidParam);
}

TEST_F(SendDataTest, SendDataSyncMeta_Error_ProcessFormatFail) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  // ProcessFormatMeta will be failed.
  setProcessFormatMetaFail(kProcessFormatResultFailure);
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  resetProcessFormatMetaSuccess();
  ASSERT_EQ(result, EdgeAppLibSendDataResultFailure);
}

TEST_F(SendDataTest, SendDataSyncMeta_Normal_InferencesTwo) {
  // number of inferences is 2.
  setNumOfInfPerMsg(2);

  // the same ai model.
  const char *ai_model_first = "22";
  setProcessFormatMetaOutput(ai_model_first);

  {
    uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
    uint32_t in_size = sizeof(in_data);
    uint64_t time_stamp = 10000;
    int timeout_ms = 10000;
    EdgeAppLibSendDataResult result = SendDataSyncMeta(
        in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
    // first, enqueued.
    ASSERT_EQ(result, EdgeAppLibSendDataResultEnqueued);
  }

  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  // second, send.
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);

  setNumOfInfPerMsg(1);
}

TEST_F(SendDataTest, SendDataSyncMeta_Normal_InferencesMax) {
  // number of inferences is max.
  setNumOfInfPerMsg(100);

  // the same ai model.
  const char *ai_model_first = "100100";
  setProcessFormatMetaOutput(ai_model_first);

  int max_minus_one = MAX_NUMBER_OF_INFERENCE_QUEUE - 1;
  for (int i = 0; i < max_minus_one; i++) {
    uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
    uint32_t in_size = sizeof(in_data);
    uint64_t time_stamp = 10000;
    int timeout_ms = 10000;
    EdgeAppLibSendDataResult result = SendDataSyncMeta(
        in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
    // 99 times, enqueued.
    ASSERT_EQ(result, EdgeAppLibSendDataResultEnqueued);
  }

  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  // at 100th time, send.
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);

  setNumOfInfPerMsg(1);
}

TEST_F(SendDataTest, SendDataSyncMeta_Nornal_InferencesMaxOverSameAIModel) {
  // number of inference is over max.
  setNumOfInfPerMsg(101);

  // the same ai model.
  const char *ai_model_first = "101101";
  setProcessFormatMetaOutput(ai_model_first);

  for (int i = 0; i < MAX_NUMBER_OF_INFERENCE_QUEUE; i++) {
    uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
    uint32_t in_size = sizeof(in_data);
    uint64_t time_stamp = 10000;
    int timeout_ms = 10000;
    EdgeAppLibSendDataResult result = SendDataSyncMeta(
        in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
    // 100 times, enqueued.
    ASSERT_EQ(result, EdgeAppLibSendDataResultEnqueued);
  }

  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  // at 101th time, send.
  ASSERT_EQ(result, EdgeAppLibSendDataResultSuccess);

  setNumOfInfPerMsg(1);
}

TEST_F(SendDataTest,
       SendDataTest_SendDataSyncMeta_Error_InferencesMaxOverDifferentAIModel) {
  // number of inference is over max.
  setNumOfInfPerMsg(101);

  int i = 0;
  char ai_model_first[16];

  for (i; i < MAX_NUMBER_OF_INFERENCE_QUEUE; i++) {
    // different ai model each other.
    snprintf(ai_model_first, 16, "%d", i);
    setProcessFormatMetaOutput(ai_model_first);

    uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
    uint32_t in_size = sizeof(in_data);
    uint64_t time_stamp = 10000;
    int timeout_ms = 10000;
    EdgeAppLibSendDataResult result = SendDataSyncMeta(
        in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
    // 100 times, enqueued.
    ASSERT_EQ(result, EdgeAppLibSendDataResultEnqueued);
  }

  snprintf(ai_model_first, 16, "%d", i);
  setProcessFormatMetaOutput(ai_model_first);

  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;
  EdgeAppLibSendDataResult result = SendDataSyncMeta(
      in_data, in_size, EdgeAppLibSendDataBase64, time_stamp, timeout_ms);
  // at 101th time, failure.
  ASSERT_EQ(result, EdgeAppLibSendDataResultFailure);

  setNumOfInfPerMsg(1);
}
