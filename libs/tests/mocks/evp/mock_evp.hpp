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

#ifndef MOCKS_MOCK_EVP_HPP
#define MOCKS_MOCK_EVP_HPP

#include "evp_c_sdk/sdk.h"

EVP_RESULT EVP_setConfigurationCallback(struct EVP_client *h,
                                        EVP_CONFIGURATION_CALLBACK cb,
                                        void *userData);
int wasSetConfigurationCallbackCalled();
void resetSetConfigurationCallbackCalled();

EVP_RESULT EVP_processEvent(struct EVP_client *h, int timeout_ms);
int wasProcessEventCalled();
void resetProcessEventCalled();
void setProcessEventResult(EVP_RESULT result);

struct EVP_client *EVP_initialize(void);

EVP_RESULT EVP_sendState(struct EVP_client *h, const char *topic,
                         const void *state, size_t statelen,
                         EVP_STATE_CALLBACK cb, void *userData);

int wasEvpInitializeCalled();
int wasEvpBlobOperationCalled();
const char *getEvpBlobOperationRequestedUrl();
void setEvpBlobOperationNotCallbackCall();
void resetEvpBlobOperationNotCallbackCall();
void resetEvpBlobOperationCalled();
void setEvpBlobOperationResult(EVP_RESULT res);
void resetEvpBlobOperationResult();
void setEvpBlobCallbackReason(EVP_BLOB_CALLBACK_REASON reason);
int setSendTelemetryResult(EVP_RESULT res);
void resetSendTelemetryResult();

EVP_RESULT EVP_blobOperation(struct EVP_client *h, EVP_BLOB_TYPE type,
                             EVP_BLOB_OPERATION op, const void *request,
                             struct EVP_BlobLocalStore *localStore,
                             EVP_BLOB_CALLBACK cb, void *userData);
void Mock_SetAsyncMode(int enable);
void Mock_SetCallbackTest(int enable);
void Mock_SetNullWorkspace(int is_null);
EVP_RESULT
EVP_sendTelemetry(struct EVP_client *h,
                  const struct EVP_telemetry_entry *entries, size_t nentries,
                  EVP_TELEMETRY_CALLBACK cb, void *userData);

typedef struct {
  int *array;
  int size;
  uint64_t timestamp;
} DummyData;

DummyData getDummyData(int size);

void resetSendState();
void setSendStateTimeout();
void setSendStateInvalidParam();
void setSendStateDataTooLarge();
void callSendDataCb();

#endif /* MOCKS_MOCK_EVP_HPP */
