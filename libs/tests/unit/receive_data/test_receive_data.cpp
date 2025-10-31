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
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "context.hpp"
#include "evp/mock_evp.hpp"
#include "log.h"
#include "map.hpp"
#include "memory_manager.hpp"
#include "receive_data.h"
#include "receive_data_private.h"
#include "receive_data_utils.h"
#include "sm/mock_sm_api.hpp"

#define CUSTOM_SETTINGS "custom_settings"
#define DOWNLOAD_FILENAME "download_filename"
#define DOWNLOAD_FILENAME_WITH_SUFFIX "download_filename.suffix"
#define DOWNLOAD_URL "download_file_url"
#define DOWNLOAD_URL_WITH_SUFFIX "download_file_url.suffix"
#define TEMP_DIR "./tmp"
#define TEMP_FILENAME "./tmp/temp_file"
#define TEMP_FILE_CONTENT "abcd1234\n"
#define TEMP_FILE_HASH \
  "1e534db63466deec283cc815a27b44aa5396e7f4454e6ebef31b33060f7861df"
#define WRONG_TEMP_FILE_HASH \
  "1e534db63466deec283cc815a27b44aa5396e7f4454e6ebef31b33060f7861de"

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

TEST_F(ReceiveDataTest, NullWorkspace) {
  Mock_SetAsyncMode(true);
  Mock_SetNullWorkspace(true);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 1000),
            EdgeAppLibReceiveDataResultFailure);
}

TEST_F(ReceiveDataTest, MapSetFailure) {
  Mock_SetAsyncMode(true);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  void *value = nullptr;
  intptr_t key = 0;
  for (int i = 0; i < MAX_FUTURES_QUEUE; ++i) {
    key += 4;
    map_set((void *)key, nullptr);
  }
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 1000),
            EdgeAppLibReceiveDataResultFailure);
  key = 0;
  for (int i = 0; i < MAX_FUTURES_QUEUE; ++i) {
    key += 4;
    value = map_pop((void *)key);
    if (value) {
      map_set((void *)key, value);
    }
  }
}

struct receive_data_worker_arg {
  pthread_cond_t *cond;
  pthread_mutex_t *mtx;
  EdgeAppLibReceiveDataInfo *info;
  EdgeAppLibReceiveDataResult *res;
  int timeout;
};

static void *receive_data_worker(void *arg) {
  struct receive_data_worker_arg *wa = (struct receive_data_worker_arg *)arg;
  *(wa->res) = EdgeAppLibReceiveData(wa->info, wa->timeout);
  pthread_mutex_lock(wa->mtx);
  pthread_cond_signal(wa->cond);
  pthread_mutex_unlock(wa->mtx);
  return nullptr;
}

TEST_F(ReceiveDataTest, NotReceiveInMainThread) {
  pthread_t t;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
  EdgeAppLibReceiveDataResult res = EdgeAppLibReceiveDataResultSuccess;
  struct receive_data_worker_arg wa = {&cond, &mtx, &info, &res, 500};
  struct timespec ts;

  Mock_SetAsyncMode(true);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  pthread_create(&t, nullptr, receive_data_worker, &wa);

  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2;

  pthread_mutex_lock(&mtx);
  int rc = 0;
  while (rc == 0) {
    rc = pthread_cond_timedwait(&cond, &mtx, &ts);
    if (rc != 0) {
      pthread_mutex_unlock(&mtx);
      pthread_cancel(t);
      pthread_join(t, nullptr);
      return;
    }
  }
  pthread_mutex_unlock(&mtx);

  pthread_join(t, nullptr);
  EXPECT_EQ(res, EdgeAppLibReceiveDataResultSuccess);
}

TEST_F(ReceiveDataTest, NotReceiveInMainThreadTimeout) {
  pthread_t t;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
  EdgeAppLibReceiveDataResult res = EdgeAppLibReceiveDataResultSuccess;
  struct receive_data_worker_arg wa = {&cond, &mtx, &info, &res, 1};
  struct timespec ts;

  Mock_SetAsyncMode(true);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  pthread_create(&t, nullptr, receive_data_worker, &wa);

  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2;

  pthread_mutex_lock(&mtx);
  int rc = 0;
  while (rc == 0) {
    rc = pthread_cond_timedwait(&cond, &mtx, &ts);
    if (rc != 0) {
      pthread_mutex_unlock(&mtx);
      pthread_cancel(t);
      pthread_join(t, nullptr);
      return;
    }
  }
  pthread_mutex_unlock(&mtx);

  pthread_join(t, nullptr);
  EXPECT_EQ(res, EdgeAppLibReceiveDataResultTimeout);
}

TEST_F(ReceiveDataTest, NotReceiveInMainThreadNotimeout) {
  pthread_t t;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
  EdgeAppLibReceiveDataResult res = EdgeAppLibReceiveDataResultSuccess;
  struct receive_data_worker_arg wa = {&cond, &mtx, &info, &res, -1};
  struct timespec ts;

  Mock_SetAsyncMode(true);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);
  pthread_create(&t, nullptr, receive_data_worker, &wa);

  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2;

  pthread_mutex_lock(&mtx);
  int rc = 0;
  while (rc == 0) {
    rc = pthread_cond_timedwait(&cond, &mtx, &ts);
    if (rc != 0) {
      pthread_mutex_unlock(&mtx);
      pthread_cancel(t);
      pthread_join(t, nullptr);
      return;
    }
  }
  pthread_mutex_unlock(&mtx);

  pthread_join(t, nullptr);
  EXPECT_EQ(res, EdgeAppLibReceiveDataResultSuccess);
}

TEST_F(ReceiveDataTest, Denied) {
  Mock_SetAsyncMode(false);
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_EXIT);
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultDenied);
}

TEST_F(ReceiveDataTest, ShouldExit) {
  Mock_SetAsyncMode(false);
  setProcessEventResult(EVP_SHOULDEXIT);
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultFailure);
}

TEST_F(ReceiveDataTest, EvpBlobOperationFailure) {
  Mock_SetAsyncMode(false);
  setEvpBlobOperationResult(EVP_ERROR);
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultFailure);
}

TEST_F(ReceiveDataTest, Failure) {
  Mock_SetAsyncMode(false);
  setEvpBlobCallbackReason((EVP_BLOB_CALLBACK_REASON)999);  // unknown reason
  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultFailure);
}

TEST_F(ReceiveDataTest, GetDataStorePath) {
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);

  EXPECT_STREQ(EdgeAppLibReceiveDataStorePath(), "/tmp/workspace");
}

TEST_F(ReceiveDataTest, GetSuffixDownloadUrlNormal) {
  const char path[] = "http://192.0.2.0:8000/fake_model.tflite";
  char *suffix = GetSuffixFromUrl(path);
  EXPECT_STREQ(suffix, ".tflite");
  ReleaseSuffixString(suffix);
}

TEST_F(ReceiveDataTest, GetSuffixDownloadUrlNoSuffix) {
  const char path[] = "http://192.0.2.0:8000/fake_model";
  EXPECT_EQ(GetSuffixFromUrl(path), nullptr);
}

TEST_F(ReceiveDataTest, GetSuffixDownloadUrlWithQuery) {
  const char path[] = "http://192.0.2.0:8000/fake_model.tflite?q=query";
  char *suffix = GetSuffixFromUrl(path);
  EXPECT_STREQ(suffix, ".tflite");
  ReleaseSuffixString(suffix);
}

TEST_F(ReceiveDataTest, GetSuffixDownloadUrlNoSuffixWithQuery) {
  const char path[] = "http://192.0.2.0:8000/fake_model?q=query";
  EXPECT_EQ(GetSuffixFromUrl(path), nullptr);
}

TEST_F(ReceiveDataTest, GetSuffixDownloadUrlWithQueryWithSlash) {
  const char path[] = "http://192.0.2.0:8000/fake_model.tflite?q=qu/er/y";
  char *suffix = GetSuffixFromUrl(path);
  EXPECT_STREQ(suffix, ".tflite");
  ReleaseSuffixString(suffix);
}

TEST_F(ReceiveDataTest, GetSuffixDownloadUrlNoDomain) {
  const char path[] = "fake_model.tflite";
  char *suffix = GetSuffixFromUrl(path);
  EXPECT_STREQ(suffix, ".tflite");
  ReleaseSuffixString(suffix);
}

TEST_F(ReceiveDataTest, HashCheckNormal) {
  mkdir(TEMP_DIR, 0755);
  FILE *f = fopen(TEMP_FILENAME, "wb");
  if (f) {
    fwrite(TEMP_FILE_CONTENT, 1, strlen(TEMP_FILE_CONTENT), f);
    fclose(f);
  }
  EXPECT_EQ(IsFileHashCorrect(TEMP_FILE_HASH, TEMP_FILENAME), true);
  EXPECT_EQ(IsFileHashCorrect(WRONG_TEMP_FILE_HASH, TEMP_FILENAME), false);
  remove(TEMP_FILENAME);
  rmdir(TEMP_DIR);
}

TEST_F(ReceiveDataTest, HashCheckWrongInput) {
  mkdir(TEMP_DIR, 0755);
  FILE *f = fopen(TEMP_FILENAME, "wb");
  if (f) {
    fwrite(TEMP_FILE_CONTENT, 1, strlen(TEMP_FILE_CONTENT), f);
    fclose(f);
  }
  EXPECT_EQ(IsFileHashCorrect(nullptr, TEMP_FILENAME), false);
  EXPECT_EQ(IsFileHashCorrect(TEMP_FILE_HASH, nullptr), false);
  EXPECT_EQ(IsFileHashCorrect("too_short_hash", TEMP_FILENAME), false);
  EXPECT_EQ(IsFileHashCorrect(TEMP_FILE_HASH, "not_a_real_file_name"), false);
  remove(TEMP_FILENAME);
  rmdir(TEMP_DIR);
}

/*
TEST_F(ReceiveDataTest, HashCheckMallocFailure) {
  mkdir(TEMP_DIR, 0755);
  // How to wrap malloc to make it fail?
  EXPECT_EQ(IsFileHashCorrect(TEMP_FILE_HASH, TEMP_FILENAME), false);
  rmdir(TEMP_DIR);
}
*/

TEST_F(ReceiveDataTest, RemoveOldFileNormal) {
  mkdir("./tmp", 0755);
  FILE *f = fopen("./tmp/fake_model.tflite", "wb");
  if (f) {
    fwrite("abcd1234", 1, 8, f);
    fclose(f);
  }
  f = fopen("./tmp/fake_model_txt", "wb");
  if (f) {
    fwrite("abcd1234", 1, 8, f);
    fclose(f);
  }
  f = fopen("./tmp/fake_model", "wb");
  if (f) {
    fwrite("abcd1234", 1, 8, f);
    fclose(f);
  }
  f = fopen("./tmp/short", "wb");
  if (f) {
    fwrite("abcd1234", 1, 8, f);
    fclose(f);
  }
  EXPECT_EQ(RemoveOutdatedFile("./tmp", "fake_model"), REMOVE_FILE_ATTEMPT * 2);
  remove("./tmp/fake_model_txt");
  remove("./tmp/short");
  rmdir("./tmp");
}

TEST_F(ReceiveDataTest, RemoveOldFileWrongDir) {
  EXPECT_EQ(RemoveOutdatedFile("/not_a_real_dir", "fake_model"),
            OPEN_DIR_FAILED);
}

/*
TEST_F(ReceiveDataTest, RemoveOldFileUnlinkFailure) {
  mkdir("./tmp", 0755);
  FILE *f = fopen("./tmp/fake_model.tflite", "wb");
  if (f) {
    fwrite("abcd1234", 1, 8, f);
    fclose(f);
  }
  // How to wrap unlink to make it fail?
  EXPECT_EQ(RemoveOutdatedFile("./tmp", "fake_model"),
            REMOVE_FILE_ATTEMPT + REMOVE_FILE_FAILED);
  remove("./tmp/fake_model.tflite");
  rmdir("./tmp");
}
*/

TEST_F(ReceiveDataTest, HashMatchSkipDownload) {
  // Test that when local file has same hash as remote, download is skipped
  // and no memory errors occur
  const char *workspace =
      EVP_getWorkspaceDirectory(evp_client, EVP_WORKSPACE_TYPE_DEFAULT);
  ASSERT_NE(workspace, nullptr);

  // Create workspace directory if it doesn't exist
  mkdir(workspace, 0755);

  // Create a test file with known content
  char filepath[256];
  snprintf(filepath, sizeof(filepath), "%s/%s", workspace,
           DOWNLOAD_FILENAME_WITH_SUFFIX);
  FILE *f = fopen(filepath, "w");
  ASSERT_NE(f, nullptr);
  fprintf(f, "test content");
  fclose(f);

  // Calculate hash of the test file
  // Hash of "test content" is:
  // 6ae8a75555209fd6c44157c0aed8016e763ff435a19cf186f76863140143ff72
  const char *test_hash =
      "6ae8a75555209fd6c44157c0aed8016e763ff435a19cf186f76863140143ff72";
  info.hash = strdup(test_hash);

  // Use URL with suffix to raise coverage
  free(info.url);
  info.url = strdup(DOWNLOAD_URL_WITH_SUFFIX);
  info.urllen = strlen(DOWNLOAD_URL_WITH_SUFFIX);

  Mock_SetAsyncMode(false);
  // Even though we set this, blob callback should not be called because of hash
  // match
  setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON_DONE);

  EXPECT_EQ(EdgeAppLibReceiveData(&info, 500),
            EdgeAppLibReceiveDataResultSuccess);

  // Clean up
  free(info.hash);
  info.hash = nullptr;
  unlink(filepath);
}
