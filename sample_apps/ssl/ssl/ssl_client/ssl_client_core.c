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

#include "ssl_client_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "ssl_client_core.h"
#include "ssl_client_keepalive.h"

// ========================================
// INTERNAL HELPER FUNCTIONS
// ========================================

// Function to initialize SSL context
static int init_ssl_context(struct ssl_context_impl *ctx,
                            const char *server_name) {
  int ret = 0;

  mbedtls_net_init(&ctx->server_fd);
  mbedtls_ssl_init(&ctx->ssl);
  mbedtls_ssl_config_init(&ctx->conf);
  mbedtls_entropy_init(&ctx->entropy);
  mbedtls_ctr_drbg_init(&ctx->ctr_drbg);

  // Initialize keep-alive connection state
  ctx->connection_established = 0;
  ctx->connection_health = 0;
  ctx->last_activity = 0;
  memset(ctx->connected_server, 0, sizeof(ctx->connected_server));
  memset(ctx->connected_port, 0, sizeof(ctx->connected_port));

  // Initialize the RNG and the session data
  LOG_TRACE("Seeding the random number generator...");

  const char *pers = "ssl_client";
  if ((ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                   &ctx->entropy, (const unsigned char *)pers,
                                   strlen(pers))) != 0) {
    LOG_ERR("mbedtls_ctr_drbg_seed returned %d", ret);
    return ret;
  }
  LOG_TRACE("Random number generator seeded successfully");

  // Setup SSL configuration
  LOG_TRACE("Setting up the SSL/TLS structure...");

  if ((ret = mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    LOG_ERR("Failed to set SSL config defaults: -0x%x", -ret);
    return ret;
  }
  LOG_TRACE("SSL/TLS structure setup completed");

  mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

  if ((ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf)) != 0) {
    LOG_ERR("Failed to setup SSL: -0x%x", -ret);
    return ret;
  }

  if ((ret = mbedtls_ssl_set_hostname(&ctx->ssl, server_name)) != 0) {
    LOG_ERR("Failed to set hostname: -0x%x", -ret);
    return ret;
  }

  return 0;
}

// Function to cleanup SSL context
static void cleanup_ssl_context(struct ssl_context_impl *ctx) {
  // Close connection if it's established
  if (ctx->connection_established) {
    ssl_close_connection((ssl_context_t *)ctx);
  }

  mbedtls_net_free(&ctx->server_fd);
  mbedtls_ssl_free(&ctx->ssl);
  mbedtls_ssl_config_free(&ctx->conf);
  mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
  mbedtls_entropy_free(&ctx->entropy);
}

// Function to connect to server
static int connect_to_server(struct ssl_context_impl *ctx,
                             const char *server_name, const char *port) {
  int ret;

  // Connect to the server
  LOG_INFO("Connecting to tcp/%s/%s...", server_name, port);

  if ((ret = mbedtls_net_connect(&ctx->server_fd, server_name, port,
                                 MBEDTLS_NET_PROTO_TCP)) != 0) {
    LOG_ERR("Failed to connect to server: -0x%x", -ret);
    return ret;
  }
  LOG_TRACE("Connected to server successfully");

  // Set up the SSL/TLS structure
  LOG_TRACE("Setting up the SSL/TLS structure...");

  mbedtls_ssl_set_bio(&ctx->ssl, &ctx->server_fd, mbedtls_net_send,
                      mbedtls_net_recv, NULL);
  LOG_TRACE("SSL/TLS structure setup completed");

  // Start the SSL handshake
  LOG_TRACE("Starting SSL handshake...");

  while ((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      LOG_ERR("SSL handshake failed: -0x%x", -ret);
      return ret;
    }
  }
  LOG_TRACE("SSL handshake completed successfully");

  // Set connection state for keep-alive
  ctx->connection_established = 1;
  ctx->connection_health = 1;
  ctx->last_activity = time(NULL);
  strncpy(ctx->connected_server, server_name,
          sizeof(ctx->connected_server) - 1);
  strncpy(ctx->connected_port, port, sizeof(ctx->connected_port) - 1);
  ctx->connected_server[sizeof(ctx->connected_server) - 1] = '\0';
  ctx->connected_port[sizeof(ctx->connected_port) - 1] = '\0';

  return 0;
}

// Helper function to check if JSON response appears complete
static int is_json_response_complete(const char *response, int total_received) {
  if (total_received <= 1000) return 0;

  const char *search_start = response + (total_received - 300);
  if (search_start < response) search_start = response;

  // Check for JSON completion patterns (production server specific)
  if (strstr(search_start, "}") && (strstr(search_start, "\"status\"") ||
                                    strstr(search_start, "\"success\"") ||
                                    strstr(search_start, "\"error\""))) {
    LOG_TRACE("JSON response appears complete with status fields");
    return 1;
  }

  // Check for balanced JSON braces
  int open_braces = 0, close_braces = 0;
  for (int i = 0; i < total_received; i++) {
    if (response[i] == '{') open_braces++;
    if (response[i] == '}') close_braces++;
  }
  if (open_braces > 0 && open_braces == close_braces) {
    LOG_TRACE(
        "Balanced JSON braces detected (%d open, %d close), likely complete",
        open_braces, close_braces);
    return 1;
  }

  return 0;
}

// Helper function to send HTTP request data
static int send_http_request_data(struct ssl_context_impl *ctx,
                                  const char *request) {
  int ret, len;

  len = strlen(request);
  LOG_INFO("Sending HTTP request (%d bytes)", len);
  LOG_DBG("Request:  ----------------------");
  LOG_DBG("%s", request);
  LOG_DBG("--------------------------------");

  while ((ret = mbedtls_ssl_write(&ctx->ssl, (unsigned char *)request, len)) <=
         0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      LOG_ERR("SSL write failed: -0x%x", -ret);
      return ret;
    }
  }

  LOG_INFO("HTTP request sent successfully (%d bytes)", ret);
  return 0;
}

// Helper function to check if response is complete
static int is_response_complete(const char *response, size_t total_received,
                                int consecutive_no_data,
                                time_t last_data_time) {
  // Check if we have HTTP headers
  if (total_received >= 4) {
    char *body_start = strstr(response, "\r\n\r\n");
    if (body_start) {
      LOG_TRACE("HTTP headers received, checking for complete response");

      // For substantial responses, check if response is already complete
      if (total_received > 1000) {
        if (is_json_response_complete(response, total_received)) {
          LOG_TRACE("Response appears complete based on content analysis");
          return 1;
        }

        // For large responses, assume complete to prevent hanging
        if (total_received > 6000) {
          LOG_TRACE("Large response (%d bytes), assuming complete",
                    total_received);
          return 1;
        }
      }

      // If we have headers and some data, check for JSON completion
      if (total_received > 100) {
        // Look for JSON completion in the entire response
        if (strstr(response, "}") && (strstr(response, "\"status\"") ||
                                      strstr(response, "\"success\"") ||
                                      strstr(response, "\"error\""))) {
          LOG_TRACE(
              "JSON response appears complete with status fields in full "
              "response");
          return 1;
        }
      }
    }
  }

  // Check if we've been waiting too long for more data
  if (total_received > 0 && (time(NULL) - last_data_time) > 5) {
    LOG_WARN("No new data received for 5 seconds, assuming response complete");
    return 1;
  }

  // If we get multiple consecutive EOFs, the response is likely complete
  if (consecutive_no_data >= 3) {
    LOG_TRACE("Multiple consecutive EOFs, assuming response complete");
    return 1;
  }

  return 0;
}

// Helper function to read HTTP response with timeout management
static int read_http_response_with_timeout(struct ssl_context_impl *ctx,
                                           char *response,
                                           size_t response_size) {
  int ret, total_received = 0;
  time_t start_time = time(NULL);
  const int timeout_seconds = 30;
  int want_io_count = 0;
  const int max_want_io_retries = 100;
  int consecutive_no_data = 0;
  time_t last_data_time = start_time;

  memset(response, 0, response_size);
  LOG_TRACE("Starting to read HTTP response...");

  while (total_received < response_size - 1) {
    // Check timeout
    if (time(NULL) - start_time > timeout_seconds) {
      LOG_WARN("Response read timeout after %d seconds", timeout_seconds);
      break;
    }

    // Check retry limit for WANT_READ/WANT_WRITE
    if (want_io_count > max_want_io_retries) {
      LOG_WARN(
          "Too many WANT_READ/WANT_WRITE retries, assuming response complete");
      break;
    }

    ret =
        mbedtls_ssl_read(&ctx->ssl, (unsigned char *)response + total_received,
                         response_size - total_received - 1);

    if (ret > 0) {
      total_received += ret;
      consecutive_no_data = 0;      // Reset counter when we get data
      last_data_time = time(NULL);  // Update last data time
      LOG_DBG("Received %d bytes (total: %d)", ret, total_received);

      // Check if response is complete
      if (is_response_complete(response, total_received, consecutive_no_data,
                               last_data_time)) {
        break;
      }

    } else if (ret == 0) {
      LOG_TRACE("Connection EOF (no more data)");
      consecutive_no_data++;

      // Check if response is complete
      if (is_response_complete(response, total_received, consecutive_no_data,
                               last_data_time)) {
        break;
      }

      // Small delay before trying again
      usleep(10000);  // 10ms delay
      continue;

    } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
      LOG_TRACE("SSL peer close notify received");
      break;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
      LOG_TRACE("SSL want read, waiting for more data...");
      want_io_count++;
      usleep(10000);  // 10ms delay
      continue;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      LOG_TRACE("SSL want write, waiting for more data...");
      want_io_count++;
      usleep(10000);  // 10ms delay
      continue;
    } else {
      if (total_received > 0) {
        LOG_WARN("SSL read error -0x%x, but we have data (%d bytes)", -ret,
                 total_received);
        break;
      } else {
        LOG_ERR("SSL read failed: -0x%x", -ret);
        return ret;
      }
    }
  }

  LOG_TRACE("Finished reading HTTP response, total bytes: %d", total_received);

  // Null-terminate the response
  response[total_received] = '\0';

  // Log the response details
  LOG_INFO("HTTP response received (%d total bytes)", total_received);
  LOG_DBG("Response:  ---------------------");
  LOG_DBG("%s", response);
  LOG_DBG("------------------------");

  return total_received;
}

// Function to send HTTP request and receive response
static int send_http_request(struct ssl_context_impl *ctx, const char *request,
                             char *response, size_t response_size) {
  // Update last activity time for keep-alive
  ctx->last_activity = time(NULL);

  // Send HTTP request
  if (send_http_request_data(ctx, request) < 0) {
    return -1;
  }

  // Receive HTTP response
  return read_http_response_with_timeout(ctx, response, response_size);
}

// ========================================
// INTERNAL SSL OPERATION FUNCTIONS (SSL Client Library)
// ========================================

// Context management functions
ssl_context_t *ssl_create_context(void) {
  return (ssl_context_t *)malloc(sizeof(struct ssl_context_impl));
}

void ssl_destroy_context(ssl_context_t *ctx) { free(ctx); }

int ssl_init_context(ssl_context_t *ctx, const char *server_name) {
  return init_ssl_context((struct ssl_context_impl *)ctx, server_name);
}

void ssl_cleanup_context(ssl_context_t *ctx) {
  cleanup_ssl_context((struct ssl_context_impl *)ctx);
}

// Connection management functions
int ssl_connect_to_server(ssl_context_t *ctx, const char *server_name,
                          const char *port) {
  return connect_to_server((struct ssl_context_impl *)ctx, server_name, port);
}

void ssl_close_connection(ssl_context_t *ctx) {
  struct ssl_context_impl *ctx_impl = (struct ssl_context_impl *)ctx;

  if (ctx_impl && ctx_impl->connection_established) {
    LOG_TRACE("Closing SSL connection");
    mbedtls_ssl_close_notify(&ctx_impl->ssl);
    mbedtls_net_free(&ctx_impl->server_fd);
    ctx_impl->connection_established = 0;
    ctx_impl->connection_health = 0;
    memset(ctx_impl->connected_server, 0, sizeof(ctx_impl->connected_server));
    memset(ctx_impl->connected_port, 0, sizeof(ctx_impl->connected_port));
  }
}

// HTTP operations function
int ssl_send_http_request(ssl_context_t *ctx, const char *request,
                          char *response, size_t response_size) {
  return send_http_request((struct ssl_context_impl *)ctx, request, response,
                           response_size);
}
