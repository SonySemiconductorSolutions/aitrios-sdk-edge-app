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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base64.h"
#include "log.h"
#include "parson.h"
#include "ssl_client.h"
#include "ssl_client_config.h"
#include "ssl_client_core.h"
#include "ssl_client_keepalive.h"

// Buffer sizes for metadata operations
#define FEATURE_STR_BUFFER_SIZE 8192

// Function to encode tensor data to base64
static int encode_tensor_to_base64(const unsigned char *data, size_t data_size,
                                   char *feature_str, size_t feature_str_size) {
  LOG_TRACE("Processing metadata data");

  // Check if we have valid data
  if (!data || data_size == 0) {
    LOG_ERR("Invalid tensor data: data=%p, size=%zu", data, data_size);
    return -1;
  }

  // Calculate required buffer size for base64 encoding
  unsigned int base64_size = b64e_size((unsigned int)data_size);

  if (base64_size >= feature_str_size) {
    LOG_ERR("Buffer too small for base64 encoding: required=%u, available=%zu",
            base64_size, feature_str_size);
    return -1;
  }

  // Convert binary tensor data to base64 string
  unsigned int encoded_size =
      b64_encode(data, (unsigned int)data_size, (unsigned char *)feature_str);

  LOG_INFO("Converted tensor data to base64: %zu bytes -> %u base64 chars",
           data_size, encoded_size);

  return 0;
}

// Helper function to create metadata JSON payload
static char *create_metadata_payload(const char *feature_str) {
  // Request sample
  // {
  //   "primaryProbeData": {
  //     "primaryData": "base64_encoded_tensor_data"
  //   }
  // }

  JSON_Value *payload_json = json_value_init_object();
  JSON_Object *payload_obj = json_value_get_object(payload_json);

  // Set primaryProbeData
  JSON_Value *probe_data_val = json_value_init_object();
  JSON_Object *probe_data_obj = json_value_get_object(probe_data_val);

  // Use the base64 feature string directly (this is what the server expects)
  json_object_set_string(probe_data_obj, "primaryData", feature_str);

  json_object_set_value(payload_obj, "primaryProbeData", probe_data_val);

  char *payload_string = json_serialize_to_string(payload_json);
  json_value_free(payload_json);

  LOG_INFO("Created identification payload (%zu bytes)",
           strlen(payload_string));
  return payload_string;
}

// Helper function to send metadata request and get response
static int send_metadata_request(const char *payload_string, char *response,
                                 size_t response_size) {
  ssl_context_t *ctx;
  char path[SSL_PATH_BUFFER_SIZE];
  char request[SSL_REQUEST_BUFFER_SIZE];
  int ret;

  const char *metadata_endpoint = ssl_get_metadata_endpoint();
  if (strlen(metadata_endpoint) == 0) {
    LOG_ERR("Metadata endpoint not configured");
    return -1;
  }
  snprintf(path, sizeof(path), "%s%s", ssl_get_base_path(), metadata_endpoint);

  // Get or create SSL context (handles connection reuse automatically)
  ctx = ssl_keepalive_get_context(ssl_get_server_name(), ssl_get_server_port());
  if (!ctx) {
    LOG_ERR("Failed to get SSL context for keep-alive");
    return -1;
  }

  // Create HTTP POST request with keep-alive
  size_t payload_length = strlen(payload_string);
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

  ret = ssl_send_http_request(ctx, request, response, response_size);

  // Keep SSL context for reuse, don't cleanup

  return ret;
}

// Helper function to parse metadata response
int parse_metadata_response(const char *response) {
  char *json_start = strstr(response, "\r\n\r\n");
  if (!json_start) {
    LOG_ERR("Could not find JSON body in metadata response");
    return -1;
  }

  json_start += 4;
  JSON_Value *response_json = json_parse_string(json_start);
  if (!response_json) {
    LOG_ERR("Failed to parse metadata response JSON");
    return -1;
  }

  JSON_Object *response_obj = json_value_get_object(response_json);
  if (!response_obj) {
    LOG_ERR("Failed to get metadata response object");
    json_value_free(response_json);
    return -1;
  }

  // Check match result
  const char *match_result =
      json_object_get_string(response_obj, "matchResult");
  if (!match_result) {
    LOG_ERR("'matchResult' field not found in response");
    json_value_free(response_json);
    return -1;
  }

  LOG_INFO("Match Result: %s", match_result);

  if (strcmp(match_result, "MATCH") == 0) {
    LOG_TRACE("Identify match found:");

    // Extract external references
    JSON_Object *ext_ref =
        json_object_get_object(response_obj, "externalReference");
    JSON_Object *ref_data =
        ext_ref ? json_object_get_object(ext_ref, "referenceData") : NULL;
    JSON_Array *ext_refs =
        ref_data ? json_object_get_array(ref_data, "externalReferences") : NULL;

    if (ext_refs) {
      size_t ref_count = json_array_get_count(ext_refs);
      for (size_t i = 0; i < ref_count; i++) {
        JSON_Object *ref = json_array_get_object(ext_refs, i);
        const char *claims_ref_id =
            ref ? json_object_get_string(ref, "claimsReferenceId") : NULL;
        if (claims_ref_id) {
          LOG_INFO("  - %s", claims_ref_id);
        }
      }
    }
  } else {
    LOG_TRACE("No match found");
  }

  json_value_free(response_json);
  return 0;
}

// Function to extract match result from response
int get_match_result(const char *response, char *match_result,
                     size_t buffer_size) {
  char *json_start = strstr(response, "\r\n\r\n");
  if (!json_start) {
    LOG_ERR("Could not find JSON body in metadata response");
    return -1;
  }

  json_start += 4;
  JSON_Value *response_json = json_parse_string(json_start);
  if (!response_json) {
    LOG_ERR("Failed to parse metadata response JSON");
    return -1;
  }

  JSON_Object *response_obj = json_value_get_object(response_json);
  if (!response_obj) {
    LOG_ERR("Failed to get metadata response object");
    json_value_free(response_json);
    return -1;
  }

  // Get match result
  const char *result = json_object_get_string(response_obj, "matchResult");
  if (!result) {
    LOG_ERR("'matchResult' field not found in response");
    json_value_free(response_json);
    return -1;
  }

  // Copy match result to buffer
  if (strlen(result) >= buffer_size) {
    LOG_ERR("Match result buffer too small");
    json_value_free(response_json);
    return -1;
  }

  strncpy(match_result, result, buffer_size - 1);
  match_result[buffer_size - 1] = '\0';  // Ensure null termination

  json_value_free(response_json);
  return 0;
}

// Main function to send output tensor to the SSL server
int send_output_tensor(const char *tensor_data, size_t data_size,
                       char *response, size_t response_size) {
  // Check if SSL client is configured
  if (!ssl_is_configured()) {
    LOG_ERR("SSL client not configured yet.");
    return -1;
  }

  LOG_TRACE("Processing output tensor...");

  // Check if we have authentication tokens
  if (strlen(get_edge_token()) == 0 || strlen(get_edge_id()) == 0) {
    LOG_ERR(
        "Edge token or ID not available. Run connect_ssl_server() "
        "first.");
    return -1;
  }

  LOG_INFO("Processing raw tensor data (%zu bytes)", data_size);

  // Encode tensor data to base64
  char feature_str[FEATURE_STR_BUFFER_SIZE] = {0};
  if (encode_tensor_to_base64((const unsigned char *)tensor_data, data_size,
                              feature_str, sizeof(feature_str)) != 0) {
    LOG_ERR("Failed to encode tensor data to base64");
    return -1;
  }

  // Create JSON payload
  char *payload_string = create_metadata_payload(feature_str);
  if (!payload_string) {
    LOG_ERR("Failed to create metadata payload - configuration error");
    return -1;
  }

  // Send identification request
  int ret = send_metadata_request(payload_string, response, response_size);

  // Cleanup payload
  json_free_serialized_string(payload_string);

  if (ret <= 0) {
    LOG_ERR("Failed to receive metadata response");
    return -1;
  }

  LOG_TRACE("Metadata sent successfully");
  return 0;
}
