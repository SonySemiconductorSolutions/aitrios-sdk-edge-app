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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "parson.h"
#include "ssl_client_config.h"
#include "ssl_client_core.h"
#include "ssl_client_keepalive.h"

// Global variables for authentication
static char nidp_edge_token_str[SSL_TOKEN_BUFFER_SIZE] = {0};
static char nidp_edge_id[SSL_EDGE_ID_BUFFER_SIZE] = {0};

// Function to perform Edge Login
static int edge_login(void) {
  ssl_context_t *ctx;
  char path[SSL_PATH_BUFFER_SIZE];
  char request[SSL_REQUEST_BUFFER_SIZE];
  char response[SSL_RESPONSE_BUFFER_SIZE];
  int ret;

  LOG_TRACE("Starting Edge Login...");
  LOG_INFO("Using server: %s", ssl_get_server_name());
  LOG_INFO("Using port: %s", ssl_get_server_port());
  LOG_INFO("Using username: %s", ssl_get_edge_login_user_id());

  // Construct the login path
  const char *login_endpoint = ssl_get_edge_login_endpoint();
  if (strlen(login_endpoint) == 0) {
    LOG_ERR("Edge login endpoint not configured");
    return -1;
  }
  snprintf(path, sizeof(path), "%s%s", ssl_get_base_path(), login_endpoint);
  LOG_INFO("Constructed path: %s", path);

  // Initialize keep-alive if not already done
  if (ssl_keepalive_init() != 0) {
    LOG_ERR("Failed to initialize keep-alive");
    return -1;
  }

  // Get or create SSL context for keep-alive
  ctx = ssl_keepalive_get_context(ssl_get_server_name(), ssl_get_server_port());
  if (!ctx) {
    LOG_ERR("Failed to get SSL context for keep-alive");
    return -1;
  }

  // Request sample
  // {
  //   "userId": "edge",
  //   "password": "password0123456789",
  //   "hostId": "xxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  // }

  // Create JSON payload
  JSON_Value *json_value = json_value_init_object();
  JSON_Object *json_object = json_value_get_object(json_value);
  json_object_set_string(json_object, "userId", ssl_get_edge_login_user_id());
  json_object_set_string(json_object, "password",
                         ssl_get_edge_login_password());
  json_object_set_string(json_object, "hostId", ssl_get_edge_login_host_id());

  char *json_string = json_serialize_to_string(json_value);
  size_t json_length = strlen(json_string);

  // Create HTTP POST request with keep-alive
  snprintf(request, sizeof(request),
           "POST %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: EdgeApp\r\n"
           "Accept: */*\r\n"
           "Content-Type: application/json\r\n"
           "Content-Length: %zu\r\n"
           "Connection: keep-alive\r\n"
           "\r\n"
           "%s",
           path, ssl_get_server_name(), json_length, json_string);

  // Send request and receive response
  ret = ssl_send_http_request(ctx, request, response, sizeof(response));

  // Cleanup JSON but keep SSL context for reuse
  json_free_serialized_string(json_string);
  json_value_free(json_value);

  if (ret <= 0) {
    LOG_ERR("Failed to receive login response");
    return -1;
  }

  // Parse response to extract token
  char *json_start = strstr(response, "\r\n\r\n");
  if (!json_start) {
    LOG_ERR("Could not find JSON body start (\\r\\n\\r\\n)");
    return -1;
  }

  json_start += 4;  // Skip "\r\n\r\n"
  LOG_TRACE("Found JSON body start");

  JSON_Value *response_json = json_parse_string(json_start);
  if (!response_json) {
    LOG_ERR("Failed to parse JSON");
    return -1;
  }

  LOG_TRACE("JSON parsed successfully");
  JSON_Object *response_obj = json_value_get_object(response_json);
  if (!response_obj) {
    LOG_ERR("Failed to get response object");
    json_value_free(response_json);
    return -1;
  }

  LOG_TRACE("Got response object");
  JSON_Object *user_session =
      json_object_get_object(response_obj, "userSession");
  if (!user_session) {
    LOG_ERR("userSession object not found");
    json_value_free(response_json);
    return -1;
  }

  LOG_TRACE("Found userSession object");
  const char *token = json_object_get_string(user_session, "token");
  if (!token) {
    LOG_ERR("Token field not found in userSession");
    json_value_free(response_json);
    return -1;
  }

  LOG_INFO("Found token: %s", token);
  snprintf(nidp_edge_token_str, sizeof(nidp_edge_token_str), "Bearer %s",
           token);
  LOG_TRACE("Edge Login successful. Token obtained.");
  json_value_free(response_json);
  return 0;
}

// Function to get edge info
static int get_edge_info(void) {
  ssl_context_t *ctx;
  char path[SSL_PATH_BUFFER_SIZE];
  char request[SSL_REQUEST_BUFFER_SIZE];
  char response[SSL_RESPONSE_BUFFER_SIZE];
  int ret;

  LOG_TRACE("Getting Edge Info...");

  // Construct the edge info path
  const char *info_endpoint = ssl_get_edge_info_endpoint();
  if (strlen(info_endpoint) == 0) {
    LOG_ERR("Edge info endpoint not configured");
    return -1;
  }
  snprintf(path, sizeof(path), "%s%s/%s", ssl_get_base_path(), info_endpoint,
           ssl_get_edge_info_host_id());

  // Get or create SSL context (handles connection reuse automatically)
  ctx = ssl_keepalive_get_context(ssl_get_server_name(), ssl_get_server_port());
  if (!ctx) {
    LOG_ERR("Failed to get SSL context for keep-alive");
    return -1;
  }

  // Create HTTP GET request with keep-alive
  snprintf(request, sizeof(request),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: EdgeApp\r\n"
           "Accept: */*\r\n"
           "Content-Type: application/json\r\n"
           "Authorization: %s\r\n"
           "Connection: keep-alive\r\n"
           "\r\n",
           path, ssl_get_server_name(), nidp_edge_token_str);

  // Send request and receive response
  ret = ssl_send_http_request(ctx, request, response, sizeof(response));

  // Keep SSL context for reuse, don't cleanup

  if (ret <= 0) {
    LOG_ERR("Failed to receive edge info response");
    return -1;
  }

  // Parse response to extract edge ID
  char *json_start = strstr(response, "\r\n\r\n");
  if (!json_start) {
    LOG_ERR("Could not find JSON body start (\\r\\n\\r\\n)");
    return -1;
  }

  json_start += 4;  // Skip "\r\n\r\n"
  LOG_TRACE("Found JSON body start");
  LOG_DBG("JSON body:  ---------------------");
  LOG_DBG("%s", json_start);
  LOG_DBG("--------------------------------");

  JSON_Value *response_json = json_parse_string(json_start);
  if (!response_json) {
    LOG_ERR("Failed to parse JSON response");
    return -1;
  }

  LOG_TRACE("JSON parsed successfully");
  JSON_Object *response_obj = json_value_get_object(response_json);

  // Check for error message
  JSON_Object *error_obj = json_object_get_object(response_obj, "errorMessage");
  if (error_obj) {
    const char *error_code = json_object_get_string(error_obj, "code");
    const char *error_message = json_object_get_string(error_obj, "message");
    LOG_ERR("Server returned error - Code: %s, Message: %s",
            error_code ? error_code : "unknown",
            error_message ? error_message : "unknown");
    json_value_free(response_json);
    return -1;
  }

  // Get edge ID
  const char *edge_id = json_object_get_string(response_obj, "edgeId");
  if (!edge_id) {
    LOG_ERR("Response does not contain 'edgeId' field");
    json_value_free(response_json);
    return -1;
  }

  strncpy(nidp_edge_id, edge_id, sizeof(nidp_edge_id) - 1);
  nidp_edge_id[sizeof(nidp_edge_id) - 1] = '\0';
  LOG_INFO("Edge info retrieved. Edge ID: %s", nidp_edge_id);
  json_value_free(response_json);
  return 0;
}

// Main connect_ssl_server function
int connect_ssl_server(void) {
  // Check if SSL client is configured
  if (!ssl_is_configured()) {
    LOG_ERR("SSL client not configured yet.");
    return -1;
  }

  LOG_TRACE("Starting connect_ssl_server process...");

  // Step 1: Edge Login
  if (edge_login() != 0) {
    LOG_ERR("Edge Login failed");
    return -1;
  }

  // Step 2: Get edge info
  if (get_edge_info() != 0) {
    LOG_ERR("Get edge info failed");
    return -1;
  }

  LOG_TRACE("connect_ssl_server completed successfully");
  return 0;
}

// Getter functions for global variables
const char *get_edge_token(void) { return nidp_edge_token_str; }

const char *get_edge_id(void) { return nidp_edge_id; }
