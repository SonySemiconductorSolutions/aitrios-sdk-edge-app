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
#include "wasm_export.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EVP_client *EVP_initialize_wrapper(wasm_exec_env_t exec_env) {
  return EVP_initialize();
}

EVP_RESULT
EVP_setConfigurationCallback_wrapper(wasm_exec_env_t exec_env,
                                     struct EVP_client *h,
                                     EVP_CONFIGURATION_CALLBACK cb,
                                     void *userData) {
  return EVP_setConfigurationCallback(h, cb, userData);
}

EVP_RESULT
EVP_sendState_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                      const char *topic, const void *state, size_t statelen,
                      EVP_STATE_CALLBACK cb, void *userData) {
  return EVP_sendState(h, topic, state, statelen, cb, userData);
}

EVP_RESULT
EVP_blobOperation_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                          EVP_BLOB_TYPE type, EVP_BLOB_OPERATION op,
                          const void *request,
                          struct EVP_BlobLocalStore *localStore,
                          EVP_BLOB_CALLBACK cb, void *userData) {
  return EVP_blobOperation(h, type, op, request, localStore, cb, userData);
}

EVP_RESULT
EVP_sendTelemetry_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                          const struct EVP_telemetry_entry *entries,
                          size_t nentries, EVP_TELEMETRY_CALLBACK cb,
                          void *userData) {
  return EVP_sendTelemetry(h, entries, nentries, cb, userData);
}

EVP_RESULT
EVP_processEvent_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                         int timeout_ms) {
  return EVP_processEvent(h, timeout_ms);
}

static NativeSymbol wasm_exported_symbols[] = {
    EXPORT_WASM_API_WITH_SIG2(EVP_initialize, "()i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_setConfigurationCallback, "(iii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_sendState, "(i$*~ii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_blobOperation, "(iiiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_sendTelemetry, "(iiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_processEvent, "(ii)i"),
};

uint32_t get_native_lib(const char **p_module_name,
                        NativeSymbol **p_native_symbols) {
  *p_module_name = "env";
  *p_native_symbols = wasm_exported_symbols;
  return sizeof(wasm_exported_symbols) / sizeof(NativeSymbol);
}

#ifdef __cplusplus
}
#endif
