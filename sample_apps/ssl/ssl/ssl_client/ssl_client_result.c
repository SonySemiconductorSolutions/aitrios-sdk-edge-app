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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "parson.h"
#include "ssl_client.h"
#include "ssl_client_config.h"
#include "ssl_client_core.h"
#include "ssl_client_keepalive.h"

// Function to send match result to the SSL server result endpoint
int send_result(const char *match_result, size_t result_size) {
  // Check if SSL client is configured
  if (!ssl_is_configured()) {
    LOG_ERR("SSL client not configured yet.");
    return -1;
  }

  LOG_TRACE("Sending result to SSL server...");

  // Check if we have authentication tokens
  if (strlen(get_edge_token()) == 0 || strlen(get_edge_id()) == 0) {
    LOG_ERR(
        "Edge token or ID not available. Run connect_ssl_server() "
        "first.");
    return -1;
  }

  // Get or create SSL context (handles connection reuse automatically)
  ssl_context_t *ctx =
      ssl_keepalive_get_context(ssl_get_server_name(), ssl_get_server_port());
  if (!ctx) {
    LOG_ERR("Failed to get SSL context for keep-alive");
    return -1;
  }

  // Build the result endpoint path
  char path[SSL_PATH_BUFFER_SIZE];
  snprintf(path, sizeof(path), "%s%s", ssl_get_base_path(),
           ssl_get_result_endpoint());

  // Create JSON payload for result
  JSON_Value *payload_json = json_value_init_object();
  JSON_Object *payload_obj = json_value_get_object(payload_json);

  json_object_set_string(payload_obj, "matchResult", match_result);

  char *payload_string = json_serialize_to_string(payload_json);
  if (!payload_string) {
    LOG_ERR("Failed to serialize result payload");
    json_value_free(payload_json);
    return -1;
  }

  size_t payload_length = strlen(payload_string);

  // Create HTTP request with keep-alive
  char request[SSL_REQUEST_BUFFER_SIZE];
  snprintf(request, sizeof(request),
           "POST %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: EdgeApp\r\n"
           "Accept: */*\r\n"
           "Content-Type: application/json\r\n"
           "Content-Length: %zu\r\n"
           "Authorization: %s\r\n"
           "EdgeId: %s\r\n"
           "SERVICER_ID: %s\r\n"
           "Connection: keep-alive\r\n"
           "\r\n"
           "%s",
           path, ssl_get_server_name(), payload_length, get_edge_token(),
           get_edge_id(), ssl_get_metadata_servicer(), payload_string);

  // Send request and get response
  char response[SSL_RESPONSE_BUFFER_SIZE];
  int ret = ssl_send_http_request(ctx, request, response, sizeof(response));

  // Cleanup JSON but keep SSL context for reuse
  json_free_serialized_string(payload_string);
  json_value_free(payload_json);

  if (ret <= 0) {
    LOG_ERR("Failed to send result to SSL server");
    return -1;
  }

  LOG_INFO("Result sent successfully to SSL server");
  return 0;
}
