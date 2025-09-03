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

#include "ssl_client_keepalive.h"

#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#include "log.h"
#include "mbedtls/net_sockets.h"
#include "ssl_client_core.h"

// Global SSL context for keep-alive
static ssl_context_t *g_ssl_context = NULL;
static char g_keepalive_server_name[256] = {0};
static char g_keepalive_server_port[16] = {0};
static int g_keepalive_initialized = 0;

int ssl_keepalive_init(void) {
  if (g_keepalive_initialized) {
    LOG_WARN("Keep-alive already initialized");
    return 0;
  }

  LOG_TRACE("Initializing SSL keep-alive handler");
  g_ssl_context = NULL;
  memset(g_keepalive_server_name, 0, sizeof(g_keepalive_server_name));
  memset(g_keepalive_server_port, 0, sizeof(g_keepalive_server_port));
  g_keepalive_initialized = 1;

  return 0;
}

void ssl_keepalive_cleanup(void) {
  if (!g_keepalive_initialized) {
    return;
  }

  LOG_TRACE("Cleaning up SSL context and keep-alive handler");

  if (g_ssl_context) {
    ssl_close_connection(g_ssl_context);
    ssl_cleanup_context(g_ssl_context);
    ssl_destroy_context(g_ssl_context);
    g_ssl_context = NULL;
  }

  memset(g_keepalive_server_name, 0, sizeof(g_keepalive_server_name));
  memset(g_keepalive_server_port, 0, sizeof(g_keepalive_server_port));
  g_keepalive_initialized = 0;
}

void *ssl_keepalive_get_context(const char *server_name, const char *port) {
  if (!g_keepalive_initialized) {
    LOG_ERR("Keep-alive not initialized");
    return NULL;
  }

  // Check if connection is already established and can be reused
  if (g_ssl_context) {
    if (ssl_keepalive_can_reuse_connection(g_ssl_context, server_name, port) ==
        0) {
      LOG_INFO("[OK] Connection already established and alive to %s:%s",
               server_name, port);
      return g_ssl_context;
    }
  }

  // Create new SSL context
  LOG_INFO("[NEW] Creating new SSL context for %s:%s", server_name, port);
  g_ssl_context = ssl_create_context();
  if (!g_ssl_context) {
    LOG_ERR("Failed to create SSL context");
    return NULL;
  }

  // Initialize SSL context
  if (ssl_init_context(g_ssl_context, server_name) != 0) {
    LOG_ERR("Failed to initialize SSL context");
    ssl_destroy_context(g_ssl_context);
    g_ssl_context = NULL;
    return NULL;
  }

  // Connect to server
  LOG_INFO(
      "[SSL] Establishing new SSL connection to %s:%s (SSL handshake required)",
      server_name, port);
  if (ssl_connect_to_server(g_ssl_context, server_name, port) != 0) {
    LOG_ERR("Failed to connect to server");
    ssl_cleanup_context(g_ssl_context);
    ssl_destroy_context(g_ssl_context);
    g_ssl_context = NULL;
    return NULL;
  }

  // Store server info
  strncpy(g_keepalive_server_name, server_name,
          sizeof(g_keepalive_server_name) - 1);
  strncpy(g_keepalive_server_port, port, sizeof(g_keepalive_server_port) - 1);
  g_keepalive_server_name[sizeof(g_keepalive_server_name) - 1] = '\0';
  g_keepalive_server_port[sizeof(g_keepalive_server_port) - 1] = '\0';

  LOG_INFO("[OK] SSL connection established for keep-alive to %s:%s",
           server_name, port);
  return g_ssl_context;
}

// Keep-alive specific connection management functions
int ssl_keepalive_can_reuse_connection(ssl_context_t *ctx,
                                       const char *server_name,
                                       const char *port) {
  struct ssl_context_impl *impl = (struct ssl_context_impl *)ctx;

  if (!impl || !impl->connection_established) {
    return -1;
  }

  // Check if we can reuse this connection
  if (strcmp(impl->connected_server, server_name) != 0 ||
      strcmp(impl->connected_port, port) != 0) {
    LOG_TRACE("Cannot reuse connection - different server/port");
    return -1;
  }

  // Check connection health
  if (ssl_keepalive_check_connection_health(ctx) != 0) {
    LOG_TRACE("Connection health check failed, cannot reuse");
    return -1;
  }

  // Update last activity time
  impl->last_activity = time(NULL);
  LOG_TRACE("Reusing existing SSL connection");
  return 0;
}

int ssl_keepalive_check_connection_health(ssl_context_t *ctx) {
  struct ssl_context_impl *ctx_impl = (struct ssl_context_impl *)ctx;

  if (!ctx_impl || !ctx_impl->connection_established) {
    LOG_TRACE("Connection health check: context not established");
    return -1;
  }

  LOG_TRACE("Performing TCP socket connection health check...");

  // Check TCP socket health by accessing the socket fd directly
  int sock = ctx_impl->server_fd.fd;
  if (sock < 0) {
    LOG_TRACE("Connection health check: invalid socket fd");
    ctx_impl->connection_health = 0;
    return -1;
  }

  LOG_TRACE("Connection health check: socket fd = %d", sock);

  // Check if socket is still valid by trying to get socket info
  // This will fail if the socket is closed or invalid
  int optval;
  socklen_t optlen = sizeof(optval);
  int getsockopt_ret = getsockopt(sock, SOL_SOCKET, SO_TYPE, &optval, &optlen);
  if (getsockopt_ret < 0) {
    LOG_TRACE("Socket health check failed: getsockopt error = %d",
              getsockopt_ret);
    ctx_impl->connection_health = 0;
    return -1;
  }

  LOG_TRACE("Connection health check: TCP socket healthy");
  ctx_impl->connection_health = 1;
  return 0;
}

// ========================================
// PUBLIC EXPORT FUNCTIONS (External API)
// ========================================

// SSL client cleanup function
int ssl_client_cleanup(void) {
  LOG_TRACE("Cleaning up SSL client keep-alive connections");
  ssl_keepalive_cleanup();
  return 0;
}
