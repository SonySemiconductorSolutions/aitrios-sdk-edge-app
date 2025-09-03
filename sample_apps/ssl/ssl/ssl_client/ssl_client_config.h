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

#ifndef SSL_CLIENT_CONFIG_H
#define SSL_CLIENT_CONFIG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration field buffer sizes
#define SSL_CONFIG_SERVER_NAME_SIZE 256
#define SSL_CONFIG_SERVER_PORT_SIZE 16
#define SSL_CONFIG_BASE_PATH_SIZE 256
#define SSL_CONFIG_USER_ID_SIZE 256
#define SSL_CONFIG_PASSWORD_SIZE 256
#define SSL_CONFIG_HOST_ID_SIZE 256
#define SSL_CONFIG_SERVICER_SIZE 256
#define SSL_CONFIG_DATASET_SIZE 256
#define SSL_CONFIG_API_ENDPOINT_SIZE 256

// Request/Response buffer sizes
#define SSL_REQUEST_BUFFER_SIZE 8192
#define SSL_RESPONSE_BUFFER_SIZE 8192
#define SSL_TOKEN_BUFFER_SIZE 2048
#define SSL_EDGE_ID_BUFFER_SIZE 256

// Path and URL buffer sizes
#define SSL_PATH_BUFFER_SIZE 512
#define SSL_JSON_PAYLOAD_SIZE 2048

/**
 * @brief SSL Client Configuration Structure
 *
 * This structure holds all SSL client configuration parameters
 * including server settings, authentication credentials, and
 * metadata configuration.
 */
typedef struct {
  // Server settings
  char server_name[SSL_CONFIG_SERVER_NAME_SIZE];
  char server_port[SSL_CONFIG_SERVER_PORT_SIZE];
  char base_path[SSL_CONFIG_BASE_PATH_SIZE];

  // Authentication settings
  char edge_login_user_id[SSL_CONFIG_USER_ID_SIZE];
  char edge_login_password[SSL_CONFIG_PASSWORD_SIZE];
  char edge_login_host_id[SSL_CONFIG_HOST_ID_SIZE];
  char edge_info_host_id[SSL_CONFIG_HOST_ID_SIZE];

  // Metadata settings
  char metadata_servicer[SSL_CONFIG_SERVICER_SIZE];
  char metadata_dataset[SSL_CONFIG_DATASET_SIZE];

  // API endpoints
  char edge_login_endpoint[SSL_CONFIG_API_ENDPOINT_SIZE];
  char edge_info_endpoint[SSL_CONFIG_API_ENDPOINT_SIZE];
  char metadata_endpoint[SSL_CONFIG_API_ENDPOINT_SIZE];
  char result_endpoint[SSL_CONFIG_API_ENDPOINT_SIZE];

  // Configuration loaded flag
  bool is_configured;
} ssl_client_config_t;

bool ssl_is_configured(void);
const char *ssl_get_server_name(void);
const char *ssl_get_server_port(void);
const char *ssl_get_base_path(void);
const char *ssl_get_edge_login_user_id(void);
const char *ssl_get_edge_login_password(void);
const char *ssl_get_edge_login_host_id(void);
const char *ssl_get_edge_info_host_id(void);
const char *ssl_get_metadata_servicer(void);
const char *ssl_get_metadata_dataset(void);
const char *ssl_get_edge_login_endpoint(void);
const char *ssl_get_edge_info_endpoint(void);
const char *ssl_get_metadata_endpoint(void);
const char *ssl_get_result_endpoint(void);

#ifdef __cplusplus
}
#endif

#endif /* SSL_CLIENT_CONFIG_H */
