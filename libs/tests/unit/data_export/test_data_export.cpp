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

#include "context.hpp"
#include "data_export.h"
#include "data_export_private.h"
#include "evp/mock_evp.hpp"
#include "fixtures/data_export_fixture.hpp"
#include "log.h"
#include "map.hpp"
#include "memory_manager.hpp"
#include "sm/mock_sm_api.hpp"
#include "sm_api.hpp"

#define PORTNAME_META ((char *)"metadata")
#define CUSTOM_SETTINGS "custom_settings"

using namespace EdgeAppLib;

class EdgeAppLibDataExportApiTestCb : public CommonTest {
 public:
  void SetUp() override {
    CommonTest::SetUp();
    evp_client = EVP_initialize();
    DataExportInitialize(context, evp_client);
  }

  void TearDown() override {
    CommonTest::TearDown();
    map_clear();
  }

  DummyData dummy_data = {0};
};

class EdgeAppLibDataExportApiTest : public EdgeAppLibDataExportApiTestCb {
 public:
  void SetUp() override {
    EdgeAppLibDataExportApiTestCb::SetUp();
    map_clear();
  }

  void TearDown() override { EdgeAppLibDataExportApiTestCb::TearDown(); }
};

class MockStateMachineContext : public Context {
 public:
  struct EVP_client *evp_client;
};

TEST_F(CommonTest, InitializesTestSuccess) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_EQ(wasEvpInitializeCalled(), 0);
}

TEST_F(CommonTest, UnInitializesTestSuccess) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  res = DataExportUnInitialize();
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataAwaitCleanupTestSuccess) {
  dummy_data = getDummyData(5);
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 1);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);

  EdgeAppLibDataExportResult res;
  res = DataExportAwait(future, 5000);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);

  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_FALSE(DataExportHasPendingOperations());

  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest,
       SendDataAwaitCleanupTestSuccessBlobStorage) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettings(1);
  EdgeAppLibDataExportFuture *future =
      DataExportSendData(PORTNAME_META, EdgeAppLibDataExportMetadata,
                         (void *)dummy_data.array, dummy_data.size, 0);
  res = DataExportAwait(future, 5000);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);

  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_FALSE(DataExportHasPendingOperations());
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, DataExportSendDataNonsupportSendMethod) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettings(5);
  EdgeAppLibDataExportFuture *future =
      DataExportSendData(PORTNAME_META, EdgeAppLibDataExportMetadata,
                         (void *)dummy_data.array, dummy_data.size, 0);
  res = DataExportAwait(future, 5000);
  EXPECT_EQ(res, EdgeAppLibDataExportResultFailure);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultFailure);

  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_FALSE(DataExportHasPendingOperations());
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataAwaitCleanupTestTelemetry) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettings(0);
  EdgeAppLibDataExportFuture *future =
      DataExportSendData(PORTNAME_META, EdgeAppLibDataExportMetadata,
                         (void *)dummy_data.array, dummy_data.size, 0);
  res = DataExportAwait(future, 5000);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);

  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_FALSE(DataExportHasPendingOperations());
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataAwaitCleanupTestFailCallback) {
  dummy_data = getDummyData(5);
  setEvpBlobOperationNotCallbackCall();
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportRaw, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 1);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultEnqueued);
  future->result = EdgeAppLibDataExportResultFailure;
  EdgeAppLibDataExportResult res;
  res = DataExportAwait(future, 5000);
  EXPECT_EQ(res, EdgeAppLibDataExportResultFailure);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultFailure);

  future->is_processed = true;
  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_TRUE(DataExportHasPendingOperations());
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataAwaitCleanupTestAwaitTimeout) {
  dummy_data = getDummyData(5);
  setEvpBlobOperationNotCallbackCall();
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 1);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultEnqueued);
  EdgeAppLibDataExportResult res;
  res = DataExportAwait(future, 1);
  EXPECT_EQ(res, EdgeAppLibDataExportResultTimeout);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultEnqueued);

  future->is_processed = true;
  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_TRUE(DataExportHasPendingOperations());

  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTestCb, SendStateTest) {
  MockStateMachineContext *mock_context = new MockStateMachineContext;
  mock_context->evp_client = EVP_initialize();
  EdgeAppLibDataExportResult res =
      DataExportInitialize(mock_context, evp_client);

  EdgeAppLibDataExportResult result = DataExportSendState(
      CUSTOM_SETTINGS, (void *)dummy_data.array, dummy_data.size);
  EXPECT_EQ(result, EdgeAppLibDataExportResultSuccess);
  delete mock_context;
}

TEST_F(EdgeAppLibDataExportApiTestCb, SendStateTestOtherTopic) {
  MockStateMachineContext *mock_context = new MockStateMachineContext;
  mock_context->evp_client = EVP_initialize();
  EdgeAppLibDataExportResult res =
      DataExportInitialize(mock_context, evp_client);

  EdgeAppLibDataExportResult result = DataExportSendState(
      "not-custom-settings", (void *)dummy_data.array, dummy_data.size);
  EXPECT_EQ(result, EdgeAppLibDataExportResultSuccess);
  delete mock_context;
}

TEST_F(EdgeAppLibDataExportApiTest, StopSelfTest) {
  MockStateMachineContext *context = new MockStateMachineContext;
  context->SetNextState(STATE_CREATING);
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  context->SetNextState(STATE_RUNNING);
  ASSERT_FALSE(context->IsPendingNotification());
  res = DataExportStopSelf();
  ASSERT_EQ(context->GetNextState(), STATE_IDLE);
  ASSERT_TRUE(context->IsPendingNotification());
  delete context;
}

TEST_F(EdgeAppLibDataExportApiTest, BlockingAwaitAlreadyProcessed) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 1);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);

  res = DataExportAwait(future, -1);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);
  callSendDataCb();
  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);

  free(dummy_data.array);
}

static void *send_signal(void *args) {
  EdgeAppLibDataExportFuture *future = (EdgeAppLibDataExportFuture *)args;
  usleep(500000);
  pthread_mutex_lock(&future->mutex);
  future->result = EdgeAppLibDataExportResultSuccess;
  pthread_cond_signal(&future->cond);
  pthread_mutex_unlock(&future->mutex);
  return NULL;
}

TEST_F(EdgeAppLibDataExportApiTest, BlockingAwaitEnqueued) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  resetEvpBlobOperationCalled();
  setEvpBlobOperationNotCallbackCall();
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultEnqueued);

  pthread_t thread;
  pthread_create(&thread, NULL, send_signal, future);
  res = DataExportAwait(future, -1);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);

  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);

  // NOTE: to force future to be released
  future->is_processed = true;
  DataExportCleanup(future);
  pthread_join(thread, nullptr);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataEnqueues) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  resetEvpBlobOperationCalled();
  setEvpBlobOperationNotCallbackCall();
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultEnqueued);

  future->is_processed = true;
  res = DataExportCleanup(future);
  EXPECT_EQ(res, EdgeAppLibDataExportResultSuccess);

  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataErrorMalloc) {
  __setMaxAllocations(0);
  dummy_data = getDummyData(5);
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  ASSERT_EQ(future, nullptr);
  __setMaxAllocations(-1);

  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataTooManyOperations) {
  setEvpBlobOperationNotCallbackCall();
  EdgeAppLibDataExportFuture *future = nullptr;
  for (int i = 1; i <= MAX_FUTURES_QUEUE + 1; ++i) {
    if (future) {
      future->is_processed = true;
      DataExportCleanup(future);
    }
    future = DataExportSendData(PORTNAME_META, EdgeAppLibDataExportMetadata,
                                (void *)(uintptr_t)i, 0, 0);
  }
  ASSERT_EQ(future->result, EdgeAppLibDataExportResultDenied);
  future->is_processed = true;
  DataExportCleanup(future);

  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, DoubleUninitialized) {
  DataExportUnInitialize();
  // FIXME: make unitialize part of fixture
  DataExportUnInitialize();
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataFailure) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  resetEvpBlobOperationCalled();
  setEvpBlobOperationResult(EVP_ERROR);
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultFailure);

  res = DataExportCleanup(future);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataDenied) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_EXIT);
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultDenied);

  res = DataExportCleanup(future);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, EvpBlobCallbackDenied) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DENIED);
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultFailure);

  res = DataExportCleanup(future);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataFailNoMetadata) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettingsNoMetadata();
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(future, nullptr);

  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataMetadataUsesCorrectEndpoint) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettingsMetadataEndpoint("my_metadata_endpoint", "my_metadata_path");
  EdgeAppLibDataExportFuture *future =
      DataExportSendData(PORTNAME_META, EdgeAppLibDataExportMetadata,
                         (void *)dummy_data.array, dummy_data.size, 0);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 1);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);
  EXPECT_STREQ(getEvpBlobOperationRequestedUrl(),
               "my_metadata_endpoint/my_metadata_path/19700101000000000.txt");

  res = DataExportCleanup(future);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataMetadataUsesTelemetry) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettings(0);
  EdgeAppLibDataExportFuture *future =
      DataExportSendData(PORTNAME_META, EdgeAppLibDataExportMetadata,
                         (void *)dummy_data.array, dummy_data.size, 0);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);
  res = DataExportCleanup(future);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataRawUsesCorrectEndpoint) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettingsInputTensorEndpoint("my_input_tensor_endpoint",
                                     "my_input_tensor_path");
  EdgeAppLibDataExportFuture *future =
      DataExportSendData(PORTNAME_META, EdgeAppLibDataExportRaw,
                         (void *)dummy_data.array, dummy_data.size, 0);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 1);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultSuccess);
  EXPECT_STREQ(
      getEvpBlobOperationRequestedUrl(),
      "my_input_tensor_endpoint/my_input_tensor_path/19700101000000000.jpg");

  res = DataExportCleanup(future);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataMetadataDisabled) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettingsMetadataDisabled();
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportMetadata, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 0);
  EXPECT_EQ(future, nullptr);

  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataRawDisabled) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettingsInputTensorDisabled();
  EdgeAppLibDataExportFuture *future = DataExportSendData(
      PORTNAME_META, EdgeAppLibDataExportRaw, (void *)dummy_data.array,
      dummy_data.size, dummy_data.timestamp);
  EXPECT_EQ(wasEvpBlobOperationCalled(), 0);
  EXPECT_EQ(future, nullptr);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, IsEnabled) {
  setPortSettings(0);
  EXPECT_TRUE(DataExportIsEnabled(EdgeAppLibDataExportMetadata));
  EXPECT_TRUE(DataExportIsEnabled(EdgeAppLibDataExportRaw));
}

TEST_F(EdgeAppLibDataExportApiTest, IsEnabledMetadataDisabled) {
  setPortSettingsMetadataDisabled();
  EXPECT_FALSE(DataExportIsEnabled(EdgeAppLibDataExportMetadata));
  EXPECT_TRUE(DataExportIsEnabled(EdgeAppLibDataExportRaw));
}

TEST_F(EdgeAppLibDataExportApiTest, IsEnabledNoMetadata) {
  // port setting JSON object missing is equivalent to it being disabled
  setPortSettingsNoMetadata();
  EXPECT_FALSE(DataExportIsEnabled(EdgeAppLibDataExportMetadata));
  EXPECT_TRUE(DataExportIsEnabled(EdgeAppLibDataExportRaw));
}

TEST_F(EdgeAppLibDataExportApiTest, IsEnabledRawDisabled) {
  setPortSettingsInputTensorDisabled();
  EXPECT_TRUE(DataExportIsEnabled(EdgeAppLibDataExportMetadata));
  EXPECT_FALSE(DataExportIsEnabled(EdgeAppLibDataExportRaw));
}

TEST_F(CommonTest, FormatTimestamp) {
  char str[32] = {0};

  DataExportFormatTimestamp(str, sizeof(str), 0);
  ASSERT_STREQ(str, "19700101000000000");

  DataExportFormatTimestamp(str, sizeof(str), 1726161043914069133);
  ASSERT_STREQ(str, "20240912171043914");
}

TEST_F(CommonTest, FileSuffix) {
  char str[32] = {0};
  setCodecSettingsFormatValue(3);
  DataExportFileSuffix(str, sizeof(str), EdgeAppLibDataExportRaw);
  ASSERT_STREQ(str, "");
  setCodecSettingsFormatValue(0);
  DataExportFileSuffix(str, sizeof(str), EdgeAppLibDataExportRaw);
  ASSERT_STREQ(str, ".bin");
  setCodecSettingsFormatValue(1);
  DataExportFileSuffix(str, sizeof(str), EdgeAppLibDataExportRaw);
  ASSERT_STREQ(str, ".jpg");
  setCodecSettingsFormatValue(2);
  DataExportFileSuffix(str, sizeof(str), EdgeAppLibDataExportRaw);
  ASSERT_STREQ(str, ".bmp");
  DataExportFileSuffix(str, sizeof(str), EdgeAppLibDataExportMetadata);
  ASSERT_STREQ(str, ".txt");
}

TEST_F(EdgeAppLibDataExportApiTest, SendDataMetadataUsesTelemetryFail) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  dummy_data = getDummyData(5);
  setPortSettings(0);
  setSendTelemetryResult(EVP_ERROR);
  EdgeAppLibDataExportFuture *future =
      DataExportSendData(PORTNAME_META, EdgeAppLibDataExportMetadata,
                         (void *)dummy_data.array, dummy_data.size, 0);
  EXPECT_EQ(future->result, EdgeAppLibDataExportResultFailure);
  res = DataExportCleanup(future);
  free(dummy_data.array);
}

TEST_F(EdgeAppLibDataExportApiTest, DataExportGetPortSettings) {
  EdgeAppLibDataExportResult res = DataExportInitialize(context, evp_client);
  setPortSettings(0);
  JSON_Object *object = DataExportGetPortSettings();
  ASSERT_TRUE(object != nullptr);
  ASSERT_EQ(json_object_get_number(object, "metadata.method"), 0);
  json_object_clear(object);
}
