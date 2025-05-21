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
#include <time.h>
#include <unistd.h>

#include "evp_c_sdk/sdk.h"
#include "log.h"
#include "memory_manager.hpp"
#include "pthread.h"
#define PORT 8080
#define MAX_CONNECTIONS 5
/** buffer size depends on evp specification */
#define EVP_MQTT_SEND_BUFF_SIZE 131072

#define PENDING_OPERATIONS 10

#define DTDL_TOPIC "edge_app"
#define STATE_TIMEOUT "timeout"
#define STATE_INVALID "invalid"
#define STATE_BIG "big"
#define ACK_FILE "./data.ack"

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
      char *aux = (char *)malloc(EVP_MQTT_SEND_BUFF_SIZE);
      // assuming we receive all data at one call
      int config_len = recv(new_socket, aux, EVP_MQTT_SEND_BUFF_SIZE, 0);
      operations.buffer[operations.next] =
          (RingBufferElement){.config = aux, .config_len = config_len};
      operations.next = (operations.next + 1) % PENDING_OPERATIONS;
      pthread_mutex_unlock(&evp.mutex);
      assert(config_len < EVP_MQTT_SEND_BUFF_SIZE);
      assert(config_len > 0);
      LOG_INFO("Received: %d", config_len);
    } else {
      LOG_ERR("Buffer too small");
    }
    close(new_socket);
  }
  close(server_fd);
  return NULL;
}

struct EVP_client *EVP_initialize(void) {
  LOG_INFO("EVP_initialize");
  assert(!evp.is_initialized);
  evp.is_initialized = true;
  pthread_mutex_init(&evp.mutex, NULL);
  for (int i = 0; i < PENDING_OPERATIONS; ++i)
    operations.buffer[i] = (RingBufferElement){.config = NULL, .config_len = 0};
  int ret = pthread_create(&evp.thread, NULL, entrypoint, NULL);
  assert(ret == 0);
  LOG_INFO("EVP_initialize done");
  return &evp.h;
}

EVP_RESULT EVP_setConfigurationCallback(struct EVP_client *h,
                                        EVP_CONFIGURATION_CALLBACK cb,
                                        void *userData) {
  assert(evp.cb == NULL);
  assert(h = &evp.h);
  evp.cb = cb;
  evp.userData = userData;
  return EVP_OK;
}

EVP_RESULT EVP_processEvent(struct EVP_client *h, int timeout_ms) {
  assert(h == &evp.h);
  EVP_RESULT res = EVP_OK;
  pthread_mutex_lock(&evp.mutex);
  // To reduce cpu load during integration test, sleep a short time.
  usleep(10000);
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
    evp.cb(DTDL_TOPIC, operations.buffer[operations.curr].config,
           operations.buffer[operations.curr].config_len, evp.userData);
    free(operations.buffer[operations.curr].config);
    operations.buffer[operations.curr] =
        (RingBufferElement){.config = NULL, .config_len = 0};
    operations.curr = (operations.curr + 1) % PENDING_OPERATIONS;
  }
  pthread_mutex_unlock(&evp.mutex);
  return res;
}

EVP_RESULT EVP_sendState(struct EVP_client *h, const char *topic,
                         const void *state, size_t statelen,
                         EVP_STATE_CALLBACK cb, void *userData) {
  // specific topics used in unit test
  if (strcmp(topic, STATE_TIMEOUT) == 0) return EVP_TIMEDOUT;
  if (strcmp(topic, STATE_INVALID) == 0) return EVP_INVAL;
  if (strcmp(topic, STATE_BIG) == 0) return EVP_TOOBIG;

  assert(h == &evp.h);
  LOG_INFO("EVP_sendState: sending state");
  LOG_INFO("EVP_sendState: size %d, state %s", (int)statelen, (char *)state);

  // monitor ack file to be removed by the integration test
  struct stat st;
  const int timeout_s = 5;  // 5 seconds timeout
  struct timespec start, current;
  clock_gettime(CLOCK_MONOTONIC, &start);
  while (stat(ACK_FILE, &st) == 0) {
    usleep(100 * 1000);  // 100ms
    clock_gettime(CLOCK_MONOTONIC, &current);
    if ((current.tv_sec - start.tv_sec) > timeout_s) {
      LOG_ERR("Timeout waiting for ACK_FILE to be removed");
      abort();
    }
  }
  // store states in logs
  FILE *f = fopen("state.logs", "a");
  fwrite(state, statelen, 1, f);
  char newline = '\n';
  fwrite(&newline, 1, 1, f);
  fclose(f);
  // create ack file
  FILE *ack = fopen(ACK_FILE, "w");
  if (ack) fclose(ack);

  // call evp state callback
  cb(EVP_STATE_CALLBACK_REASON_SENT, userData);
  LOG_INFO("EVP_sendState: exiting");
  return EVP_OK;
}

EVP_RESULT EVP_blobOperation(struct EVP_client *h, EVP_BLOB_TYPE type,
                             EVP_BLOB_OPERATION op, const void *request,
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

  /* If there is sleep_time file, sleep 15s */
  struct stat st;
  if (stat("./sleep_time", &st) == 0) {
    sleep(15);
  }

  struct EVP_BlobResultEvp vp = {EVP_BLOB_RESULT_SUCCESS, 201, 0};
  cb(EVP_BLOB_CALLBACK_REASON_DONE, &vp, userData);
  return EVP_OK;
}

EVP_RESULT
EVP_sendTelemetry(struct EVP_client *h,
                  const struct EVP_telemetry_entry *entries, size_t nentries,
                  EVP_TELEMETRY_CALLBACK cb, void *userData) {
  if (!entries || nentries < 1 || cb == NULL) {
    return EVP_INVAL;
  }
  cb(EVP_TELEMETRY_CALLBACK_REASON_SENT, userData);
  return EVP_OK;
}
