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

#include "mock_evp.hpp"

#include <stddef.h>
#include <stdlib.h>

#include <chrono>
#include <string>

#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "memory_manager.hpp"

static int setConfigurationCallbackCalled = 0;
static int processEventCalled = 0;
static EVP_RESULT processEventResult = EVP_OK;
static EVP_RESULT sendStateResult = EVP_OK;
static int initializeCalled = 0;
static EVP_BLOB_IO_CALLBACK evp_blob_io_cb = NULL;
static module_vars_t *module_vars = NULL;
static EVP_RESULT evpBlobOperationResult = EVP_OK;
static EVP_BLOB_CALLBACK_REASON evpBlobCallbackReason =
    EVP_BLOB_CALLBACK_REASON_DONE;
static int blobOperationCalled = 0;
static EVP_BLOB_CALLBACK blob_callback = NULL;
static EVP_BLOB_CALLBACK_REASON blob_callback_reason =
    EVP_BLOB_CALLBACK_REASON_DENIED;
static std::string blob_http_request_url = "";
static struct EVP_BlobResultEvp vp = {EVP_BLOB_RESULT_SUCCESS, 201, 0};
static int evpBlobOperationNotCallbackCall = 0;

static module_vars_t *module_vars1 = NULL;
static EVP_RESULT evpsendTelemetryResult = EVP_OK;
static EVP_TELEMETRY_CALLBACK telemetry_cb = NULL;
static EVP_TELEMETRY_CALLBACK_REASON evpTelemetryCallbackReason =
    EVP_TELEMETRY_CALLBACK_REASON_SENT;
static EVP_TELEMETRY_CALLBACK_REASON telemetry_cb_reason =
    EVP_TELEMETRY_CALLBACK_REASON_DENIED;

static EVP_STATE_CALLBACK _state_cb = NULL;
static void *_state_userData = NULL;

EVP_RESULT EVP_setConfigurationCallback(struct EVP_client *h,
                                        EVP_CONFIGURATION_CALLBACK cb,
                                        void *userData) {
  setConfigurationCallbackCalled = 1;
  return EVP_OK;
}

int wasSetConfigurationCallbackCalled() {
  return setConfigurationCallbackCalled;
}

void resetSetConfigurationCallbackCalled() {
  setConfigurationCallbackCalled = 0;
}

int setSendTelemetryResult(EVP_RESULT result) {
  evpsendTelemetryResult = result;
  return 0;
}

void resetSendTelemetryResult() { evpsendTelemetryResult = EVP_OK; }

EVP_RESULT EVP_processEvent(struct EVP_client *h, int timeout_ms) {
  processEventCalled = 1;
  return processEventResult;
}

int wasProcessEventCalled() { return processEventCalled; }

void resetProcessEventCalled() { processEventCalled = 0; }

void setProcessEventResult(EVP_RESULT result) { processEventResult = result; }

struct EVP_client *EVP_initialize(void) {
  initializeCalled = 1;
  return nullptr;
}

EVP_RESULT EVP_sendState(struct EVP_client *h, const char *topic,
                         const void *state, size_t statelen,
                         EVP_STATE_CALLBACK cb, void *userData) {
  _state_cb = cb;
  _state_userData = userData;
  return sendStateResult;
}

EVP_RESULT EVP_blobOperation(struct EVP_client *h, EVP_BLOB_TYPE type,
                             EVP_BLOB_OPERATION op, const void *request,
                             struct EVP_BlobLocalStore *localStore,
                             EVP_BLOB_CALLBACK cb, void *userData) {
  blobOperationCalled = 1;

  module_vars = (module_vars_t *)userData;

  if (type == EVP_BLOB_TYPE_HTTP) {
    blob_http_request_url = ((struct EVP_BlobRequestHttp *)request)->url;
  } else {
    blob_http_request_url.clear();
  }

  if (&vp == NULL) {
    return EVP_ERROR;
  } else if (evpBlobOperationNotCallbackCall == 1) {
    LOG_DBG("Not calling BlobCallback");
    return EVP_OK;
  }

  blob_callback = cb;
  evp_blob_io_cb = localStore->io_cb;

  evp_blob_io_cb(module_vars->blob_buff, localStore->blob_len, module_vars);

  blob_callback_reason = evpBlobCallbackReason;
  LOG_DBG("Calling BlobCallback");
  blob_callback(blob_callback_reason, &vp, module_vars);

  return evpBlobOperationResult;
}

int wasEvpInitializeCalled() { return initializeCalled; };
int wasEvpBlobOperationCalled() { return blobOperationCalled; };
const char *getEvpBlobOperationRequestedUrl() {
  return blob_http_request_url.c_str();
}
void setEvpBlobOperationNotCallbackCall() {
  evpBlobOperationNotCallbackCall = 1;
};
void resetEvpBlobOperationNotCallbackCall() {
  evpBlobOperationNotCallbackCall = 0;
};
void resetEvpBlobOperationCalled() { blobOperationCalled = 0; };
void setEvpBlobOperationResult(EVP_RESULT res) {
  evpBlobOperationResult = res;
};
void resetEvpBlobOperationResult() { evpBlobOperationResult = EVP_OK; };
void setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON reason) {
  evpBlobCallbackReason = reason;
};

EVP_RESULT
EVP_sendTelemetry(struct EVP_client *h,
                  const struct EVP_telemetry_entry *entries, size_t nentries,
                  EVP_TELEMETRY_CALLBACK cb, void *userData) {
  module_vars1 = (module_vars_t *)userData;
  telemetry_cb = cb;
  telemetry_cb_reason = evpTelemetryCallbackReason;
  LOG_DBG("Calling TelemetryCallback");
  telemetry_cb(telemetry_cb_reason, module_vars1);
  return evpsendTelemetryResult;
}

DummyData getDummyData(int size) {
  DummyData result;

  result.array = (int *)malloc(size * sizeof(int));
  if (result.array == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    result.size = 0;
    return result;
  }

  for (int i = 0; i < size; ++i) {
    result.array[i] = i * 2;
  }
  result.size = size * sizeof(int);

  auto currentTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      currentTime.time_since_epoch());
  result.timestamp = duration.count();

  return result;
}

void resetSendState() { sendStateResult = EVP_OK; }

void setSendStateTimeout() { sendStateResult = EVP_TIMEDOUT; }

void setSendStateInvalidParam() { sendStateResult = EVP_INVAL; }

void setSendStateDataTooLarge() { sendStateResult = EVP_TOOBIG; }

void callSendDataCb() {
  if (!_state_cb) {
    LOG_ERR("Calling uninitialized callback");
    return;
  }
  _state_cb(EVP_STATE_CALLBACK_REASON_SENT, _state_userData);
  _state_cb = NULL;
  _state_userData = NULL;
}
