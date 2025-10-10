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

#include "ssl_server.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "log.h"
#include "mbedtls/build_info.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509.h"
#include "ssl_server_certs.h"

// Global variables
static bool g_server_running = false;
static pthread_t g_server_thread = 0;
static pthread_mutex_t g_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Single client management
static struct {
  mbedtls_net_context fd;
  mbedtls_ssl_context ssl;
  bool active;
} g_client = {0};

// Global SSL configuration
static mbedtls_ssl_config g_ssl_conf;
static bool g_ssl_conf_initialized = false;

// Forward declarations
// Main server logic
static void *ssl_server_thread_func(void *arg);

// Client connection management
static int handle_client_connection(mbedtls_net_context *client_fd);
static void cleanup_client(void);

// SSL operations
static int perform_ssl_handshake(mbedtls_ssl_context *ssl);

// HTTP protocol handling
static int handle_http_request_response(mbedtls_ssl_context *ssl);
static int read_http_request(mbedtls_ssl_context *ssl, char *buffer,
                             size_t buffer_size);
static int send_http_response(mbedtls_ssl_context *ssl, const char *response);

// Utility functions
static int load_certificates_and_keys(mbedtls_x509_crt *srvcert,
                                      mbedtls_pk_context *pkey,
                                      mbedtls_ctr_drbg_context *ctr_drbg);

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

// Start SSL server in background thread
int start_ssl_server(void) {
  LOG_TRACE("start_ssl_server() called");

  if (g_server_running) {
    LOG_WARN("SSL server already running");
    return 0;  // Already running
  }

  LOG_TRACE("SSL server not running, starting...");

  g_server_running = true;
  LOG_TRACE("Creating SSL server thread...");

  // Create server thread
  if (pthread_create(&g_server_thread, NULL, ssl_server_thread_func, NULL) !=
      0) {
    LOG_ERR("Failed to create SSL server thread");
    g_server_running = false;
    return -1;
  }

  LOG_INFO("SSL server started successfully");
  return 0;
}

// Stop SSL server and cleanup resources
//
// Shutdown Strategy:
// 1. Set g_server_running = false
// 2. Main loop will exit within 1 second (next select() timeout)
// 3. Thread completes cleanup and exits
// 4. pthread_join() completes quickly
//
// This approach ensures clean shutdown without hanging threads.
int stop_ssl_server(void) {
  LOG_TRACE("stop_ssl_server() called");

  if (!g_server_running) {
    LOG_WARN("SSL server not running");
    return 0;  // Not running
  }

  LOG_TRACE("Stopping SSL server...");
  g_server_running = false;

  // Wait for server thread to finish
  if (g_server_thread != 0) {
    LOG_TRACE("Waiting for server thread to finish...");
    int res = pthread_join(g_server_thread, NULL);
    if (res == 0) {
      LOG_TRACE("Server thread finished successfully");
    } else {
      LOG_ERR("pthread_join failed: %d", res);
      return -1;
    }

    g_server_thread = 0;
    LOG_TRACE("Server thread finished");
  }

  LOG_TRACE("Cleaning up client connection...");
  cleanup_client();

  LOG_TRACE("SSL server stopped successfully");
  return 0;
}

// ============================================================================
// MAIN SERVER LOGIC
// ============================================================================

// SSL server thread function
static void *ssl_server_thread_func(void *arg) {
  (void)arg;  // Unused parameter

  LOG_TRACE("SSL server thread starting...");

  int ret;

  // Network components
  mbedtls_net_context listen_fd;

  // mbedTLS cryptographic components
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_config conf;
  mbedtls_x509_crt srvcert;
  mbedtls_pk_context pkey;

  LOG_TRACE("Initializing mbedTLS components...");

  mbedtls_net_init(&listen_fd);
  mbedtls_ssl_init(&g_client.ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&srvcert);
  mbedtls_pk_init(&pkey);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  // 1. Seed the random number generator
  LOG_TRACE("Seeding random number generator...");
  const char *pers = "ssl_server";
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char *)pers,
                                   strlen(pers))) != 0) {
    LOG_ERR("Failed to seed RNG: %d", ret);
    goto cleanup;
  }

  // 2. Load certificates and private key
  LOG_TRACE("Loading certificates and private key...");
  if ((ret = load_certificates_and_keys(&srvcert, &pkey, &ctr_drbg)) != 0) {
    LOG_ERR("Failed to load certificates: %d", ret);
    goto cleanup;
  }

  // 3. Setup the listening TCP socket
  const char *port = SSL_SERVER_DEFAULT_PORT;
  LOG_INFO("Binding to port: %s", port);
  if ((ret = mbedtls_net_bind(&listen_fd, NULL, port, MBEDTLS_NET_PROTO_TCP)) !=
      0) {
    LOG_ERR("Failed to bind to port %s: %d", port, ret);
    goto cleanup;
  }

  // 4. Setup SSL configuration
  LOG_TRACE("Setting up SSL configuration...");
  if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    LOG_ERR("Failed to setup SSL config: %d", ret);
    goto cleanup;
  }

  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
  mbedtls_ssl_conf_ca_chain(&conf, srvcert.next, NULL);

  if ((ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey)) != 0) {
    LOG_ERR("Failed to set own cert: %d", ret);
    goto cleanup;
  }

  // Copy SSL configuration to global variable for client use
  memcpy(&g_ssl_conf, &conf, sizeof(mbedtls_ssl_config));
  g_ssl_conf_initialized = true;

  // Main server loop
  while (g_server_running) {
    mbedtls_net_context client_fd;
    mbedtls_net_init(&client_fd);

    LOG_TRACE("Waiting for client connection...");

    fd_set read_fds;
    struct timeval timeout;

    // Initialize the file descriptor set
    FD_ZERO(&read_fds);
    FD_SET(listen_fd.fd, &read_fds);  // Add listening socket to monitor

    // Set timeout to 1 second
    // This thread will wake up every 1 second to check if shutdown was
    // requested
    timeout.tv_sec = 1;   // 1 second
    timeout.tv_usec = 0;  // 0 microseconds

    // POSIX select() system call - the kernel monitors the socket and wakes us
    // up This call puts the thread to sleep until:
    // 1. A connection arrives (select_ret > 0)
    // 2. Timeout occurs (select_ret == 0)
    // 3. Error occurs (select_ret < 0)
    //
    // The kernel handles all the low-level network monitoring efficiently,
    // and this thread only wakes up when there's actual work to do.
    int select_ret = select(listen_fd.fd + 1, &read_fds, NULL, NULL, &timeout);

    if (select_ret < 0) {
      LOG_ERR("select() failed: %d", select_ret);
      mbedtls_net_free(&client_fd);
      continue;
    } else if (select_ret == 0) {
      // Timeout occurred - no connection arrived within 1 second
      mbedtls_net_free(&client_fd);
      continue;
    }

    // Connection available, try to accept
    if ((ret = mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL)) !=
        0) {
      if (ret == MBEDTLS_ERR_NET_ACCEPT_FAILED) {
        LOG_WARN("Accept failed after select() indicated connection ready: %d",
                 ret);
        mbedtls_net_free(&client_fd);
        continue;
      }
      LOG_ERR("Failed to accept client: %d", ret);
      mbedtls_net_free(&client_fd);
      continue;
    }

    LOG_TRACE("Client connection accepted, handling...");
    handle_client_connection(&client_fd);
  }
  LOG_TRACE("Exiting main server loop");

cleanup:
  LOG_TRACE("Cleaning up mbedTLS resources...");
  mbedtls_net_free(&listen_fd);
  mbedtls_ssl_free(&g_client.ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_x509_crt_free(&srvcert);
  mbedtls_pk_free(&pkey);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  LOG_TRACE("mbedTLS resources cleaned up");

  LOG_TRACE("SSL server thread exiting");
  return NULL;
}

// ============================================================================
// CLIENT CONNECTION MANAGEMENT
// ============================================================================

// Handle client connection
static int handle_client_connection(mbedtls_net_context *client_fd) {
  int ret;
  LOG_TRACE("Handling new client connection...");

  if (g_client.active) {
    LOG_WARN("Client already connected, rejecting new connection");
    mbedtls_net_free(client_fd);
    return -1;
  }

  mbedtls_ssl_init(&g_client.ssl);

  LOG_TRACE("Setting client SSL configuration...");
  if ((ret = mbedtls_ssl_setup(&g_client.ssl, &g_ssl_conf)) != 0) {
    LOG_ERR("Failed to setup client SSL context: %d", ret);
    mbedtls_ssl_free(&g_client.ssl);
    mbedtls_net_free(client_fd);
    return -1;
  }

  LOG_TRACE("Setting client BIO functions...");
  mbedtls_ssl_set_bio(&g_client.ssl, client_fd, mbedtls_net_send,
                      mbedtls_net_recv, NULL);

  // 5. Perform SSL handshake
  if ((ret = perform_ssl_handshake(&g_client.ssl)) != 0) {
    LOG_ERR("SSL handshake failed");
    mbedtls_ssl_free(&g_client.ssl);
    mbedtls_net_free(client_fd);
    return -1;
  }

  // 6. Setup client
  pthread_mutex_lock(&g_clients_mutex);
  g_client.fd = *client_fd;
  g_client.active = true;
  pthread_mutex_unlock(&g_clients_mutex);

  LOG_TRACE("Client connected successfully");

  // 7. Handle HTTP request and response
  if ((ret = handle_http_request_response(&g_client.ssl)) != 0) {
    LOG_ERR("HTTP handling failed");
  }

  // Close the SSL connection after handling the request
  LOG_TRACE("Closing SSL connection after request handling");
  mbedtls_ssl_close_notify(&g_client.ssl);

  // Clean up client resources
  cleanup_client();

  LOG_TRACE("Client connection closed and cleaned up");
  return 0;
}

// Cleanup client resources
static void cleanup_client(void) {
  LOG_TRACE("Cleaning up client");
  pthread_mutex_lock(&g_clients_mutex);
  if (g_client.active) {
    mbedtls_ssl_free(&g_client.ssl);
    mbedtls_net_free(&g_client.fd);
    g_client.active = false;
    LOG_TRACE("Client cleaned up");
  }
  pthread_mutex_unlock(&g_clients_mutex);
}

// ============================================================================
// SSL OPERATIONS
// ============================================================================

// Perform SSL handshake with retry logic
static int perform_ssl_handshake(mbedtls_ssl_context *ssl) {
  int ret;
  int handshake_attempts = 0;

  LOG_TRACE("Performing SSL handshake...");
  while ((ret = mbedtls_ssl_handshake(ssl)) != 0) {
    handshake_attempts++;
    LOG_TRACE("SSL handshake attempt %d returned: %d", handshake_attempts, ret);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
      LOG_TRACE("SSL handshake wants to read more data");
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      LOG_TRACE("SSL handshake wants to write more data");
    } else if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
               ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      LOG_ERR("SSL handshake failed with error: %d", ret);
      return ret;
    }

    // Add a small delay to prevent tight loop
    usleep(1000);  // 1ms delay
  }

  LOG_INFO("SSL handshake completed successfully after %d attempts",
           handshake_attempts);
  return 0;
}

// ============================================================================
// HTTP PROTOCOL HANDLING
// ============================================================================

// Handle complete HTTP request/response cycle
static int handle_http_request_response(mbedtls_ssl_context *ssl) {
  char request_buffer[SSL_SERVER_REQUEST_BUFFER_SIZE];
  int ret;

  // Read HTTP request
  ret = read_http_request(ssl, request_buffer, sizeof(request_buffer));
  if (ret <= 0) {
    return ret;
  }

  // Send HTTP response
  const char *http_response = SSL_SERVER_HTTP_RESPONSE;
  return send_http_response(ssl, http_response);
}

// Read HTTP request from SSL connection
static int read_http_request(mbedtls_ssl_context *ssl, char *buffer,
                             size_t buffer_size) {
  int bytes_read;

  LOG_TRACE("Reading HTTP request from client...");
  do {
    memset(buffer, 0, buffer_size);
    bytes_read =
        mbedtls_ssl_read(ssl, (unsigned char *)buffer, buffer_size - 1);

    if (bytes_read == MBEDTLS_ERR_SSL_WANT_READ ||
        bytes_read == MBEDTLS_ERR_SSL_WANT_WRITE) {
      continue;
    }

    if (bytes_read <= 0) {
      if (bytes_read == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        LOG_INFO("Connection was closed gracefully");
      } else if (bytes_read == MBEDTLS_ERR_NET_CONN_RESET) {
        LOG_WARN("Connection was reset by peer");
      } else {
        LOG_ERR("Failed to read HTTP request: %d", bytes_read);
      }
      return bytes_read;
    }

    // Data received successfully
    break;
  } while (1);

  buffer[bytes_read] = '\0';
  LOG_INFO("Received HTTP request (%d bytes): %s", bytes_read, buffer);
  return bytes_read;
}

// Send HTTP response over SSL connection
static int send_http_response(mbedtls_ssl_context *ssl, const char *response) {
  int ret;

  LOG_TRACE("Sending HTTP response...");
  while ((ret = mbedtls_ssl_write(ssl, (const unsigned char *)response,
                                  strlen(response))) <= 0) {
    if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
      LOG_ERR("Failed to send HTTP response: peer closed connection");
      return ret;
    }

    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      LOG_ERR("Failed to send HTTP response: %d", ret);
      return ret;
    }

    // SSL wants to read/write more, continue loop
    LOG_TRACE("SSL write returned WANT_READ/WANT_WRITE: %d, continuing...",
              ret);
  }

  LOG_INFO("HTTP response sent successfully (%d bytes)", ret);
  return 0;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Load certificates and private key
static int load_certificates_and_keys(mbedtls_x509_crt *srvcert,
                                      mbedtls_pk_context *pkey,
                                      mbedtls_ctr_drbg_context *ctr_drbg) {
  int ret;

  LOG_TRACE("Loading mbedTLS test certificates and keys...");

  // Load server certificate
  if ((ret = mbedtls_x509_crt_parse(srvcert,
                                    (const unsigned char *)mbedtls_test_srv_crt,
                                    mbedtls_test_srv_crt_len)) != 0) {
    LOG_ERR("Failed to parse mbedTLS server certificate: %d", ret);
    return ret;
  }

  // Load CA certificates
  if ((ret = mbedtls_x509_crt_parse(srvcert,
                                    (const unsigned char *)mbedtls_test_cas_pem,
                                    mbedtls_test_cas_pem_len)) != 0) {
    LOG_ERR("Failed to parse mbedTLS CA certificates: %d", ret);
    return ret;
  }

  // Load private key
  if ((ret = mbedtls_pk_parse_key(pkey,
                                  (const unsigned char *)mbedtls_test_srv_key,
                                  mbedtls_test_srv_key_len, NULL, 0,
                                  mbedtls_ctr_drbg_random, ctr_drbg)) != 0) {
    LOG_ERR("Failed to parse mbedTLS private key: %d", ret);
    return ret;
  }

  return 0;
}
