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

#ifndef SSL_SERVER_H
#define SSL_SERVER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// SSL Server Configuration Constants
#define SSL_SERVER_DEFAULT_PORT "4433"
#define SSL_SERVER_REQUEST_BUFFER_SIZE 8192

// Timeouts and Delays (in microseconds)
#define SSL_SERVER_ACCEPT_DELAY 10000  // 10ms delay between accept attempts

// HTTP Response Template
#define SSL_SERVER_HTTP_RESPONSE \
  "HTTP/1.0 200 OK\r\n"          \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 25\r\n"       \
  "\r\n"                         \
  "Hello from SSL Server!"

/**
 * @brief Starts the SSL server in a new thread
 *
 * This function initializes and starts the SSL server on the configured port.
 * The server runs in a separate thread to avoid blocking the main application.
 * Uses embedded test certificates for SSL/TLS encryption.
 *
 * @return 0 on success, negative value on error
 */
int start_ssl_server(void);

/**
 * @brief Stops the SSL server and cleans up resources
 *
 * This function gracefully shuts down the SSL server, closes any
 * active client connections, and frees allocated mbedTLS resources.
 *
 * @return 0 on success, negative value on error
 */
int stop_ssl_server(void);

#ifdef __cplusplus
}
#endif

#endif /* SSL_SERVER_H */
