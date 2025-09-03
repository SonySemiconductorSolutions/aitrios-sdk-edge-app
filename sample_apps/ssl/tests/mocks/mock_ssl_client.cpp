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

// Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstring>

#include "ssl_client.h"

// Mock control variables
static bool mock_send_output_tensor_fail = false;
static bool mock_parse_metadata_response_fail = false;
static bool mock_get_match_result_fail = false;
static bool mock_send_result_fail = false;
static bool mock_connect_ssl_server_fail = false;
static bool mock_return_match = false;
static bool mock_ssl_client_cleanup_fail = false;

// Mock control functions
void setMockSendOutputTensorFail(int fail) {
  mock_send_output_tensor_fail = fail;
}

void setMockParseMetadataResponseFail(int fail) {
  mock_parse_metadata_response_fail = fail;
}

void setMockGetMatchResultFail(int fail) { mock_get_match_result_fail = fail; }

void setMockSendResultFail(int fail) { mock_send_result_fail = fail; }

void setMockConnectSSLServerFail(int fail) {
  mock_connect_ssl_server_fail = fail;
}

void setMockReturnMatch(int match) { mock_return_match = match; }

void setMockSSLClientCleanupFail(int fail) {
  mock_ssl_client_cleanup_fail = fail;
}

void resetAllMockSSLClientMocks() {
  mock_send_output_tensor_fail = false;
  mock_parse_metadata_response_fail = false;
  mock_get_match_result_fail = false;
  mock_send_result_fail = false;
  mock_connect_ssl_server_fail = false;
  mock_return_match = false;
  mock_ssl_client_cleanup_fail = false;
}

// Mock implementations for SSL client functions

int connect_ssl_server(void) {
  // Mock connection with configurable success/failure
  if (mock_connect_ssl_server_fail) {
    return -1;  // Mock failure
  }
  // Mock successful connection
  return 0;
}

int send_output_tensor(const char *tensor_data, size_t data_size,
                       char *response, size_t response_size) {
  // Mock send with configurable success/failure
  if (mock_send_output_tensor_fail) {
    return -1;  // Mock failure
  }

  // Mock successful send
  if (response && response_size > 0) {
    // Provide a mock response
    const char *mock_response =
        "{\"status\":\"success\",\"metadata\":\"mock_metadata\"}";
    size_t response_len = strlen(mock_response);
    if (response_len < response_size) {
      strcpy(response, mock_response);
    } else {
      strncpy(response, mock_response, response_size - 1);
      response[response_size - 1] = '\0';
    }
  }
  return 0;
}

const char *get_edge_token(void) {
  // Return mock token
  return "Bearer mock_token_12345";
}

const char *get_edge_id(void) {
  // Return mock edge ID
  return "mock_edge_id_12345";
}

int parse_metadata_response(const char *response) {
  // Mock parsing with configurable success/failure
  if (mock_parse_metadata_response_fail) {
    return -1;  // Mock failure
  }
  // Mock successful parsing
  return 0;
}

int get_match_result(const char *response, char *match_result,
                     size_t buffer_size) {
  // Mock match result retrieval with configurable success/failure
  if (mock_get_match_result_fail) {
    return -1;  // Mock failure
  }

  // Mock successful match result retrieval
  if (match_result && buffer_size > 0) {
    const char *mock_match = mock_return_match ? "MATCH" : "mock_match_result";
    size_t match_len = strlen(mock_match);
    if (match_len < buffer_size) {
      strcpy(match_result, mock_match);
    } else {
      strncpy(match_result, mock_match, buffer_size - 1);
      match_result[buffer_size - 1] = '\0';
    }
  }
  return 0;
}

int send_result(const char *match_result, size_t result_size) {
  // Mock result sending with configurable success/failure
  if (mock_send_result_fail) {
    return -1;  // Mock failure
  }
  // Mock successful result sending
  return 0;
}

int ssl_client_cleanup(void) {
  // Mock cleanup with configurable success/failure
  if (mock_ssl_client_cleanup_fail) {
    return -1;  // Mock failure
  }
  // Mock successful cleanup
  return 0;
}
