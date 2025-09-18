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
#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "memory_manager.hpp"
#include "pthread.h"
#include "wasm_export.h"
#define PORT 8080
#define MAX_CONNECTIONS 5
/** buffer size depends on evp specification */
#define EVP_MQTT_SEND_BUFF_SIZE 131072

#define PENDING_OPERATIONS 10

#define DTDL_TOPIC "edge_app"

#define MAX_HANDLES 128

#ifdef __cplusplus
extern "C" {
#endif

struct EVP_client {};

typedef struct {
  struct EVP_client h;
  bool is_initialized;
  EVP_CONFIGURATION_CALLBACK cb;
  void *userData;
  pthread_t thread;
  pthread_mutex_t mutex;
} EVP;

typedef struct {
  void *config;
  int config_len;
} RingBufferElement;

typedef struct {
  RingBufferElement buffer[PENDING_OPERATIONS];
  int next;  // where to store next operation
  int curr;  // first value
} RingBuffer;

static RingBuffer operations;
static EVP evp;

static void *entrypoint(void *args) {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  int buffer_size = EVP_MQTT_SEND_BUFF_SIZE;

  LOG_INFO("EVP background thread entrypoint");

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    LOG_ERR("Socket failed");
    exit(1);
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    LOG_ERR("setsockopt failed");
    exit(1);
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size,
                 sizeof(buffer_size))) {
    LOG_ERR("setsockopt failed");
    exit(1);
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size,
                 sizeof(buffer_size))) {
    LOG_ERR("setsockopt failed");
    exit(1);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    LOG_ERR("bind failed");
    exit(1);
  }
  if (listen(server_fd, MAX_CONNECTIONS) < 0) {
    LOG_ERR("listen failed");
    exit(1);
  }
  while (1) {
    LOG_INFO("Server listening on port %d...", PORT);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      LOG_ERR("accept failed");
      exit(1);
    }
    LOG_INFO("Connection established with client.");

    pthread_mutex_lock(&evp.mutex);
    if (operations.buffer[operations.next].config == NULL) {
      char *aux = (char *)malloc(EVP_MQTT_SEND_BUFF_SIZE + 1);
      // assuming we receive all data at one call
      int config_len = recv(new_socket, aux, EVP_MQTT_SEND_BUFF_SIZE, 0);
      assert(config_len >= 0 && config_len <= EVP_MQTT_SEND_BUFF_SIZE);
      if (config_len > 0) {
        aux[config_len] = '\0';
        // delete the edge_app top layer: {"edge_app":{...}} → {...}
        char *json_start = strstr(aux, "\"" DTDL_TOPIC "\"");
        if (json_start) {
          json_start = strchr(json_start, '{');
          if (json_start) {
            int brace = 1;
            char *p = json_start + 1;
            while (*p && brace > 0) {
              if (*p == '{')
                brace++;
              else if (*p == '}')
                brace--;
              p++;
            }
            if (brace == 0) {
              size_t inner_len = p - json_start;
              char *inner_json = (char *)malloc(inner_len + 1);
              memcpy(inner_json, json_start, inner_len);
              inner_json[inner_len] = '\0';
              free(aux);
              aux = inner_json;
              config_len = inner_len;
            }
          }
        }
        operations.buffer[operations.next] =
            (RingBufferElement){.config = aux, .config_len = config_len};
        operations.next = (operations.next + 1) % PENDING_OPERATIONS;
        pthread_mutex_unlock(&evp.mutex);
        LOG_INFO("Received: %d", config_len);
      } else {
        free(aux);
        pthread_mutex_unlock(&evp.mutex);
        LOG_ERR("recv failed or no data received");
      }
    } else {
      LOG_ERR("Buffer too small");
    }
    close(new_socket);
  }
  close(server_fd);
  return NULL;
}

static void *handle_table[MAX_HANDLES] = {0};

int32_t register_handle(void *ptr) {
  for (int32_t i = 1; i < MAX_HANDLES; ++i) {
    if (handle_table[i] == NULL) {
      handle_table[i] = ptr;
      return i;
    }
  }
  return 0;
}

void *get_pointer(int32_t handle) {
  if (handle > 0 && handle < MAX_HANDLES) {
    return handle_table[handle];
  }
  return NULL;
}

void unregister_handle(int32_t handle) {
  if (handle > 0 && handle < MAX_HANDLES) {
    handle_table[handle] = NULL;
  }
}

struct EVP_client *EVP_initialize_wrapper(wasm_exec_env_t exec_env) {
  LOG_INFO("EVP_initialize");
  assert(!evp.is_initialized);
  evp.is_initialized = true;
  pthread_mutex_init(&evp.mutex, NULL);
  for (int i = 0; i < PENDING_OPERATIONS; ++i)
    operations.buffer[i] = (RingBufferElement){.config = NULL, .config_len = 0};
  int ret = pthread_create(&evp.thread, NULL, entrypoint, NULL);
  assert(ret == 0);
  LOG_INFO("EVP_initialize done");
  return (struct EVP_client *)(uintptr_t)register_handle(&evp.h);
}

const char *EVP_getWorkspaceDirectory_wrapper(wasm_exec_env_t exec_env,
                                              struct EVP_client *h,
                                              EVP_WORKSPACE_TYPE type) {
  const char *workspace = "/tmp/workspace";
  void *native_ws;
  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
  uint64_t wasm_ws = wasm_runtime_module_malloc(
      module_inst, strlen(workspace) + 1, &native_ws);
  if (wasm_ws == 0) {
    LOG_ERR("EVP_getWorkspaceDirectory: malloc failed");
    return NULL;
  }
  sprintf((char *)native_ws, "%s", workspace);
  return (const char *)wasm_ws;
};

EVP_RESULT
EVP_setConfigurationCallback_wrapper(wasm_exec_env_t exec_env,
                                     struct EVP_client *h,
                                     EVP_CONFIGURATION_CALLBACK cb,
                                     void *userData) {
  int32_t handle = (int32_t)((uintptr_t)h & 0xFFFFFFFF);
  struct EVP_client *handle_pointer = (struct EVP_client *)get_pointer(handle);
  int32_t userDataHandle = (int32_t)((uintptr_t)userData & 0xFFFFFFFF);
  void *userData_pointer = (void *)(uintptr_t)userDataHandle;
  int32_t cb_handle = (int32_t)((uintptr_t)cb & 0xFFFFFFFF);
  EVP_CONFIGURATION_CALLBACK cb_handle_pointer =
      (EVP_CONFIGURATION_CALLBACK)(uintptr_t)cb_handle;

  assert(evp.cb == NULL);
  assert(handle_pointer = &evp.h);
  evp.cb = cb_handle_pointer;
  evp.userData = userData_pointer;

  return EVP_OK;
}

EVP_RESULT
EVP_sendState_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                      const char *topic, const void *state, size_t statelen,
                      EVP_STATE_CALLBACK cb, void *userData) {
  int32_t handle = (int32_t)((uintptr_t)h & 0xFFFFFFFF);
  int32_t userDataHandle = (int32_t)((uintptr_t)userData & 0xFFFFFFFF);
  int32_t cb_handle = (int32_t)((uintptr_t)cb & 0xFFFFFFFF);
  uint32_t args[2];
  args[0] = EVP_STATE_CALLBACK_REASON_SENT;
  args[1] = userDataHandle;
  bool ok = wasm_runtime_call_indirect(exec_env, cb_handle, 2, args);
  if (!ok) {
    LOG_ERR("EVP_sendState: wasm_runtime_call_indirect failed");
    return EVP_ERROR;
  }
  LOG_INFO("EVP_sendState: exiting");
  return EVP_OK;
}

EVP_RESULT
EVP_blobOperation_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                          EVP_BLOB_TYPE type, EVP_BLOB_OPERATION op,
                          const void *request,
                          struct EVP_BlobLocalStore *localStore,
                          EVP_BLOB_CALLBACK cb, void *userData) {
  /*
   * quick parameter checks
   */
  if ((type != EVP_BLOB_TYPE_AZURE_BLOB && type != EVP_BLOB_TYPE_EVP &&
       type != EVP_BLOB_TYPE_HTTP && type != EVP_BLOB_TYPE_EVP_EXT &&
       type != EVP_BLOB_TYPE_HTTP_EXT) ||
      !(op == EVP_BLOB_OP_GET || op == EVP_BLOB_OP_PUT) || request == NULL ||
      localStore == NULL || cb == NULL) {
    return EVP_INVAL;
  }

  /* EVP blob get not supported. Check it here to avoid create a ST
   * request in agent side */
  if ((op == EVP_BLOB_OP_GET) &&
      ((type == EVP_BLOB_TYPE_EVP_EXT) || (type == EVP_BLOB_TYPE_EVP))) {
    return EVP_NOTSUP;
  }

  uint32_t handle = (uint32_t)((uintptr_t)h & 0xFFFFFFFF);
  uint32_t userDataHandle = (uint32_t)((uintptr_t)userData & 0xFFFFFFFF);
  uint32_t cb_handle = (uint32_t)((uintptr_t)cb & 0xFFFFFFFF);
  uint32_t request_handle = (uint32_t)((uintptr_t)request & 0xFFFFFFFF);

  // Allocate memory for EVP_BlobResultEvp in WASM memory so WASM can access it
  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
  uint32_t vp_offset = wasm_runtime_module_malloc(
      module_inst, sizeof(struct EVP_BlobResultEvp), NULL);
  struct EVP_BlobResultEvp *vp =
      (struct EVP_BlobResultEvp *)wasm_runtime_addr_app_to_native(module_inst,
                                                                  vp_offset);
  *vp = (struct EVP_BlobResultEvp){EVP_BLOB_RESULT_SUCCESS, 201, 0};

  uint32_t args[3];
  args[0] = EVP_BLOB_CALLBACK_REASON_DONE;
  args[1] = vp_offset;
  args[2] = userDataHandle;

  LOG_INFO("userData:%p, userDataHandle:0x%08X", userData, userDataHandle);

  // Save the file as JPG file
  uint32_t sending_data_addr = *(uint32_t *)wasm_runtime_addr_app_to_native(
      module_inst, userDataHandle + 16);
  LOG_INFO("Wasm address offset: 0x%08X", sending_data_addr);

  char *sending_data =
      (char *)wasm_runtime_addr_app_to_native(module_inst, sending_data_addr);

  LOG_INFO("pointer of sending_data = %p", &sending_data);
  LOG_INFO("value of sending_data = %p", sending_data);
  int *vars_blob_buff_size =
      (int *)wasm_runtime_addr_app_to_native(module_inst, userDataHandle + 20);
  uint32_t *vars_indetifier = (uint32_t *)wasm_runtime_addr_app_to_native(
      module_inst, userDataHandle + 40);

  // Create "image" directory if it does not exist
  struct stat st = {0};
  if (stat("image", &st) == -1) {
    if (mkdir("image", 0755) != 0) {
      LOG_ERR("Failed to create image directory");
      return EVP_ERROR;
    }
  }

  void *ext_request =
      wasm_runtime_addr_app_to_native(module_inst, request_handle);
  uint32_t remote_name_offset = *(uint32_t *)ext_request;
  const char *remote_name_native =
      (const char *)wasm_runtime_addr_app_to_native(module_inst,
                                                    remote_name_offset);

  const char *last_slash = strrchr(remote_name_native, '/');
  char filename[512];
  const char *basename = last_slash ? last_slash + 1 : remote_name_native;
  snprintf(filename, sizeof(filename), "image/%s", basename);

  // Save vars_blob_buff as jpg file
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    LOG_ERR("Failed to open file for writing: %s", filename);
    return EVP_ERROR;
  }

  if (fwrite(sending_data, 1, *vars_blob_buff_size, fp) !=
      *vars_blob_buff_size) {
    LOG_ERR("Failed to write jpg data to file: %s", filename);
    fclose(fp);
    return EVP_ERROR;
  }
  fclose(fp);
  LOG_INFO("Saved jpg file: %s (%zu bytes)", filename, *vars_blob_buff_size);

  bool ok = wasm_runtime_call_indirect(exec_env, cb_handle, 3, args);
  if (!ok) {
    LOG_ERR("EVP_blobOperation: wasm_runtime_call_indirect failed");
    return EVP_ERROR;
  }

  // To avoid pthread_cond_signal deadlock(if EVP_OK returned, DataExportAwait
  // will wait for cond_signal)
  return EVP_ERROR;
}

EVP_RESULT
EVP_sendTelemetry_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                          const struct EVP_telemetry_entry *entries,
                          size_t nentries, EVP_TELEMETRY_CALLBACK cb,
                          void *userData) {
  // Convert the WASM address of entries to a native pointer
  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);

  // Save the file as TXT file
  uint32_t sending_data_addr = *(uint32_t *)wasm_runtime_addr_app_to_native(
      module_inst, (uint32_t)(uintptr_t)entries + 4);
  LOG_INFO("Wasm address offset: 0x%08X", sending_data_addr);

  const char *native_value = (const char *)wasm_runtime_addr_app_to_native(
      module_inst, sending_data_addr);

  LOG_INFO("native_value = %s", native_value);

  // Create "inference" directory if it does not exist
  struct stat st = {0};
  if (stat("inference", &st) == -1) {
    if (mkdir("inference", 0755) != 0) {
      LOG_ERR("Failed to create inference directory.");
      return EVP_ERROR;
    }
  }

  // Extract the value of "T":"..." from native_value (simple string search)
  const char *filename = NULL;
  const char *t_key = "\"T\"";
  const char *t_pos = strstr(native_value, t_key);
  if (t_pos) {
    // Search after "T":
    const char *colon = strchr(t_pos, ':');
    if (colon) {
      // Skip spaces and quotes after the colon
      const char *start = colon + 1;
      while (*start == ' ' || *start == '\"') start++;
      // Until the ending quote, comma, or closing brace
      const char *end = start;
      while (*end && *end != '\"' && *end != ',' && *end != '}') end++;
      static char t_value[256];
      size_t len = end - start;
      if (len > 0 && len < sizeof(t_value)) {
        memcpy(t_value, start, len);
        t_value[len] = '\0';
        filename = t_value;
      }
    }
  }
  if (!filename) {
    LOG_ERR("Failed to extract 'T' value from native_value");
    return EVP_ERROR;
  }

  // Generate file path
  char filepath[512];
  snprintf(filepath, sizeof(filepath), "inference/%s.txt", filename);

  // Save native_value to file
  FILE *fp = fopen(filepath, "w");
  if (!fp) {
    LOG_ERR("Failed to open file for writing: %s", filepath);
    return EVP_ERROR;
  }
  if (fwrite(native_value, 1, strlen(native_value), fp) !=
      strlen(native_value)) {
    LOG_ERR("Failed to write to file: %s", filepath);
    fclose(fp);
    return EVP_ERROR;
  }
  fclose(fp);
  LOG_INFO("Saved inference file: %s", filepath);

  // To avoid pthread_cond_signal deadlock(if EVP_OK returned, DataExportAwait
  // will wait for cond_signal)
  return EVP_ERROR;
}

EVP_RESULT
EVP_processEvent_wrapper(wasm_exec_env_t exec_env, struct EVP_client *h,
                         int timeout_ms) {
  int32_t handle = (int32_t)((uintptr_t)h & 0xFFFFFFFF);
  struct EVP_client *handle_pointer = (struct EVP_client *)get_pointer(handle);
  assert(handle_pointer == &evp.h);

  EVP_RESULT res = EVP_OK;

  pthread_mutex_lock(&evp.mutex);
  if (operations.buffer[operations.curr].config_len == 1) {
    res = EVP_SHOULDEXIT;
    pthread_cancel(evp.thread);
    pthread_join(evp.thread, NULL);
    for (int i = 0; i < PENDING_OPERATIONS; ++i) {
      if (operations.buffer[i].config) free(operations.buffer[i].config);
    }
    pthread_mutex_destroy(&evp.mutex);
  } else if (operations.buffer[operations.curr].config_len != 0) {
    LOG_INFO("EVP_processEvent: %s", operations.buffer[operations.curr].config);

    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    uint32_t DTDL_TOPIC_offset =
        wasm_runtime_module_malloc(module_inst, strlen(DTDL_TOPIC) + 1, NULL);
    char *wasm_str =
        (char *)wasm_runtime_addr_app_to_native(module_inst, DTDL_TOPIC_offset);
    strcpy(wasm_str, DTDL_TOPIC);
    uint32_t config_offset = wasm_runtime_module_malloc(
        module_inst, operations.buffer[operations.curr].config_len, NULL);
    char *config_str =
        (char *)wasm_runtime_addr_app_to_native(module_inst, config_offset);
    memcpy(config_str, operations.buffer[operations.curr].config,
           operations.buffer[operations.curr].config_len);

    int32_t cb_handle = (int32_t)((uintptr_t)evp.cb & 0xFFFFFFFF);
    uint32_t args[4];
    args[0] = DTDL_TOPIC_offset;
    args[1] = config_offset;
    args[2] = operations.buffer[operations.curr].config_len;
    args[3] = (int32_t)((uintptr_t)evp.userData & 0xFFFFFFFF);

    bool ok = wasm_runtime_call_indirect(exec_env, cb_handle, 4, args);
    if (!ok) {
      LOG_ERR("EVP_processEvent: wasm_runtime_call_indirect failed");
      return EVP_ERROR;
    }

    free(operations.buffer[operations.curr].config);
    operations.buffer[operations.curr] =
        (RingBufferElement){.config = NULL, .config_len = 0};
    operations.curr = (operations.curr + 1) % PENDING_OPERATIONS;
  }
  pthread_mutex_unlock(&evp.mutex);
  return res;
}

static NativeSymbol wasm_exported_symbols[] = {
    EXPORT_WASM_API_WITH_SIG2(EVP_initialize, "()i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_setConfigurationCallback, "(iii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_sendState, "(i$*~ii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_blobOperation, "(iiiiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_sendTelemetry, "(iiiii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_processEvent, "(ii)i"),
    EXPORT_WASM_API_WITH_SIG2(EVP_getWorkspaceDirectory, "(ii)i"),
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
