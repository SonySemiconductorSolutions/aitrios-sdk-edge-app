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
#include "evp/mock_evp.hpp"
#include "log.h"
#include "map.hpp"
#include "memory_manager.hpp"
#include "receive_data.h"
#include "receive_data_private.h"
#include "sm/mock_sm_api.hpp"

#define CUSTOM_SETTINGS "custom_settings"
#define DOWNLOAD_FILENAME "download_filename"
#define DOWNLOAD_URL "download_file_url"
class ReceiveDataTest : public ::testing::Test {
 public:
  EdgeAppLibReceiveDataInfo info{};
  struct EVP_client *evp_client;

  void SetUp() override {
    info.filename = strdup(DOWNLOAD_FILENAME);
    info.filenamelen = strlen(DOWNLOAD_FILENAME);
    info.url = strdup(DOWNLOAD_URL);
    info.urllen = strlen(DOWNLOAD_URL);
    evp_client = EVP_initialize();
    EdgeAppLibReceiveDataInitialize(evp_client);
  }

  void TearDown() override {
    free(info.filename);
    free(info.url);
    info.filename = nullptr;
    info.url = nullptr;
    EdgeAppLibReceiveDataUnInitialize();
  }
};

TEST_F(ReceiveDataTest, SyncSuccess) {
  Mock_SetAsyncMode(false);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultSuccess);
}

TEST_F(ReceiveDataTest, AsyncSuccess) {
  Mock_SetAsyncMode(true);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 1000),
            EdgeAppLibReceiveDataResultSuccess);
}

TEST_F(ReceiveDataTest, AsyncSuccessNotimeout) {
  Mock_SetAsyncMode(true);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  EXPECT_EQ(EdgeAppLibReceiveData(&info, -1),
            EdgeAppLibReceiveDataResultSuccess);
}

TEST_F(ReceiveDataTest, Denied) {
  Mock_SetAsyncMode(false);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_EXIT);

  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultDenied);
}

TEST_F(ReceiveDataTest, Failure) {
  Mock_SetAsyncMode(false);
  setEvpBlobCallbackReason((EVP_BLOB_CALLBACK_REASON)999);  // unknown reason
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultFailure);
}
