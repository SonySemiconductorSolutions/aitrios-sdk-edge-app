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
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

#include "log.h"
#include "map.hpp"
#include "memory_manager.hpp"
#include "receive_data_private.h"
#include "sdk.h"

#define MAX_PATH_LEN 256

typedef struct {
  char *download;
  char *filename;

  struct EVP_BlobLocalStore localStore;
} module_vars_t;

typedef struct {
  module_vars_t *ctx;
} blob_cb_data_t;

#ifdef __cplusplus
extern "C" {
#endif

static module_vars_t module_vars;
/*
 * Concurrent calls with the same evp_client_ are not safe,
 * To avoid this, the EdgeAppLibReceiveData should not
 * be called in onIterate untill evp_agent provides a way
 * to assure evp_client_'s security.
 */
static struct EVP_client *evp_client_;

static void blob_cb(EVP_BLOB_CALLBACK_REASON reason, const void *vp,
                    void *userData) {
  assert(userData != nullptr);
  const struct EVP_BlobResultAzureBlob *result;

  blob_cb_data_t *cb_data = (blob_cb_data_t *)userData;
  module_vars_t *module_vars = cb_data->ctx;

  EdgeAppLibReceiveDataFuture *future =
      (EdgeAppLibReceiveDataFuture *)map_pop(module_vars);
  if (future == nullptr) {
    LOG_ERR(
        "State might be corrupted. SendData called but buffer not in "
        "map.");
    return;
  }

  pthread_mutex_lock(&future->mutex);

  switch (reason) {
    case EVP_BLOB_CALLBACK_REASON_DONE:
      future->result = EdgeAppLibReceiveDataResultSuccess;
      result = static_cast<const EVP_BlobResultAzureBlob *>(vp);
      LOG_TRACE(
          "result=%u "
          "http_status=%u error=%d",
          result->result, result->http_status, result->error);
      break;
    case EVP_BLOB_CALLBACK_REASON_EXIT:
      assert(vp == nullptr);
      future->result = EdgeAppLibReceiveDataResultDenied;
      break;
    default:
      future->result = EdgeAppLibReceiveDataResultFailure;
      LOG_CRITICAL(
          "The result of BlobOperation didn't match any "
          "EVP_BLOB_CALLBACK_REASON.");
  }
  pthread_cond_signal(&future->cond);

  pthread_mutex_unlock(&future->mutex);
  pthread_mutex_destroy(&future->mutex);
  pthread_cond_destroy(&future->cond);

  free(future);
  free(module_vars->download);
  free(module_vars->filename);
  module_vars->filename = nullptr;
  module_vars->localStore.filename = nullptr;
}

static EdgeAppLibReceiveDataFuture *InitializeFuture() {
  EdgeAppLibReceiveDataFuture *future = (EdgeAppLibReceiveDataFuture *)xmalloc(
      sizeof(EdgeAppLibReceiveDataFuture));
  if (future == nullptr) {
    LOG_ERR("Error when performing malloc for the future.");
    return nullptr;
  }
  future->result = EdgeAppLibReceiveDataResultUninitialized;
  future->cond = PTHREAD_COND_INITIALIZER;
  future->mutex = PTHREAD_MUTEX_INITIALIZER;

  return future;
}

static EdgeAppLibReceiveDataResult ReceiveDataAwait(
    EdgeAppLibReceiveDataFuture *future, int timeout_ms) {
  pthread_mutex_lock(&future->mutex);

  LOG_TRACE("ReceiveDataAwait waiting for signal");
  EdgeAppLibReceiveDataResult output;
  if (future->result == EdgeAppLibReceiveDataResultEnqueued) {
    int res = 0;
    if (timeout_ms < 0) {
      LOG_DBG("pthread_cond_wait");
      do {
        res = pthread_cond_wait(&future->cond, &future->mutex);
      } while (res == 0 &&
               future->result == EdgeAppLibReceiveDataResultEnqueued);
    } else {
      struct timeval now;
      gettimeofday(&now, nullptr);
      struct timespec time_to_wait;
      time_to_wait.tv_sec = now.tv_sec + timeout_ms / 1000;
      time_to_wait.tv_nsec = (now.tv_usec + (timeout_ms % 1000) * 1000) * 1000;
      time_to_wait.tv_sec += time_to_wait.tv_nsec / 1000000000;
      time_to_wait.tv_nsec %= 1000000000;
      LOG_DBG("pthread_cond_timedwait");
      do {
        res = pthread_cond_timedwait(&future->cond, &future->mutex,
                                     &time_to_wait);
      } while (res == 0 &&
               future->result == EdgeAppLibReceiveDataResultEnqueued);
    }

    LOG_INFO("Result of conditional wait: %d", res);
    output = EdgeAppLibReceiveDataResultFailure;
    if (res == 0)
      output = EdgeAppLibReceiveDataResultSuccess;
    else if (res == ETIMEDOUT)
      output = EdgeAppLibReceiveDataResultTimeout;
  } else {
    output = future->result;
  }
  LOG_TRACE("EdgeAppLibReceiveDataAwait stop waiting");
  pthread_mutex_unlock(&future->mutex);
  return output;
}

static EdgeAppLibReceiveDataFuture *Download_Blob(
    EdgeAppLibReceiveDataInfo *info) {
  char full_path[MAX_PATH_LEN];
  LOG_TRACE("Loading model from: %s", info->url);

  EdgeAppLibReceiveDataFuture *future = InitializeFuture();
  if (future == nullptr) {
    return nullptr;
  }

  const char *workspace =
      EVP_getWorkspaceDirectory(evp_client_, EVP_WORKSPACE_TYPE_DEFAULT);
  if (workspace == nullptr) {
    LOG_ERR("Failed to get workspace directory");
    return future;
  }
  snprintf(full_path, MAX_PATH_LEN, "%s/%s", workspace, info->filename);

  module_vars.download = strdup(info->url);
  module_vars.filename = strdup(full_path);
  module_vars.localStore.filename = module_vars.filename;
  module_vars.localStore.io_cb = nullptr;
  module_vars.localStore.blob_len = 0;

  if (map_set((void *)&(module_vars), future) == -1) {
    LOG_ERR("map_set failed");
    free(module_vars.download);
    free(module_vars.filename);
    future->result = EdgeAppLibReceiveDataResultDenied;
    return future;
  }

  future->result = EdgeAppLibReceiveDataResultEnqueued;

  /* Data used for blob_cb to share instance context and current url
   * used, and free configuration after use it */
  blob_cb_data_t cb_data;
  cb_data.ctx = &module_vars;

  struct EVP_BlobRequestAzureBlob request;
  request.url = module_vars.download;
  EVP_RESULT result =
      EVP_blobOperation(evp_client_, EVP_BLOB_TYPE_AZURE_BLOB, EVP_BLOB_OP_GET,
                        &request, &module_vars.localStore, blob_cb, &cb_data);

  if (result != EVP_OK) {
    LOG_ERR("EVP_blobOperation: result=%d", result);
    free(module_vars.download);
    free(module_vars.filename);
    future->result = EdgeAppLibReceiveDataResultFailure;
  }

  return future;
}

EdgeAppLibReceiveDataResult EdgeAppLibReceiveDataInitialize(void *evp_client) {
  evp_client_ = (EVP_client *)evp_client;
  return EdgeAppLibReceiveDataResultSuccess;
}

EdgeAppLibReceiveDataResult EdgeAppLibReceiveDataUnInitialize() {
  return EdgeAppLibReceiveDataResultSuccess;
}

EdgeAppLibReceiveDataResult EdgeAppLibReceiveData(
    EdgeAppLibReceiveDataInfo *info, int timeout_ms) {
  if (evp_client_ == nullptr) {
    LOG_ERR("EVP client is not initialized");
    return EdgeAppLibReceiveDataResultUninitialized;
  }

  if (info == nullptr || info->url == nullptr || info->filename == nullptr) {
    LOG_ERR("Invalid parameters for EdgeAppLibReceiveData");
    return EdgeAppLibReceiveDataResultInvalidParam;
  }

  LOG_DBG("EdgeAppLibReceiveData: url=%s, filename=%s, timeout_ms=%d",
          info->url, info->filename, timeout_ms);

  EdgeAppLibReceiveDataFuture *future = Download_Blob(info);
  if (future == nullptr) {
    LOG_ERR("Download_Blob failed to initialize future");
    goto error;
  }
  if (future->result != EdgeAppLibReceiveDataResultFailure &&
      future->result != EdgeAppLibReceiveDataResultDenied) {
    ReceiveDataAwait(future, timeout_ms);
  } else {
    LOG_ERR("Download_Blob failed with EdgeAppLibReceiveDataResult: %d",
            future->result);
    goto error;
  }

  return EdgeAppLibReceiveDataResultSuccess;

error:
  return EdgeAppLibReceiveDataResultFailure;
}

#ifdef __cplusplus
}
#endif
