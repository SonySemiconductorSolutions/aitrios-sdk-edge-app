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

#ifndef SSL_CLIENT_CORE_H
#define SSL_CLIENT_CORE_H

#include <stddef.h>
#include <time.h>

// Include mbedTLS headers for complete struct definitions
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>

#ifdef __cplusplus
extern "C" {
#endif

// SSL context structure implementation
struct ssl_context_impl {
  mbedtls_net_context server_fd;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;

  // Keep-alive connection state
  char connected_server[256];
  char connected_port[16];
  int connection_established;
  time_t last_activity;
  int connection_health;
};

// SSL context structure
typedef struct ssl_context_impl ssl_context_t;

// Core SSL functions - Internal to SSL client library
ssl_context_t *ssl_create_context(void);
void ssl_destroy_context(ssl_context_t *ctx);
int ssl_init_context(ssl_context_t *ctx, const char *server_name);
void ssl_cleanup_context(ssl_context_t *ctx);
int ssl_connect_to_server(ssl_context_t *ctx, const char *server_name,
                          const char *port);
int ssl_send_http_request(ssl_context_t *ctx, const char *request,
                          char *response, size_t response_size);
void ssl_close_connection(ssl_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SSL_CLIENT_CORE_H */
