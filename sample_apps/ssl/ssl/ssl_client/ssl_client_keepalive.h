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

#ifndef SSL_CLIENT_KEEPALIVE_H
#define SSL_CLIENT_KEEPALIVE_H

#include <stddef.h>

#include "ssl_client_core.h"  // For ssl_context_t type definition

#ifdef __cplusplus
extern "C" {
#endif

// Keep-alive connection handler functions
int ssl_keepalive_init(void);
void ssl_keepalive_cleanup(void);

// Get or create SSL context for keep-alive
void *ssl_keepalive_get_context(const char *server_name, const char *port);

// Keep-alive specific connection management functions
int ssl_keepalive_can_reuse_connection(ssl_context_t *ctx,
                                       const char *server_name,
                                       const char *port);
int ssl_keepalive_check_connection_health(ssl_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SSL_CLIENT_KEEPALIVE_H */
