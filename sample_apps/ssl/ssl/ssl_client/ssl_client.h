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

#ifndef SSL_CLIENT_H
#define SSL_CLIENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connects to the SSL (Secure Socket Layer) server, performs edge login
 * and retrieves edge info
 *
 * This function performs two main operations:
 * 1. Edge Login - authenticates with the server and obtains a bearer token
 * 2. Get Edge Info - retrieves edge device information using the token
 *
 * @return 0 on success, negative value on error
 */
int connect_ssl_server(void);

/**
 * @brief Sends output tensor data to the SSL server
 *
 * This function sends output tensor data to the SSL server.
 *
 * @param tensor_data Raw binary tensor data from inference output
 * @param data_size Size of the inference data
 * @param response Buffer to store the server response
 * @param response_size Size of the response buffer
 * @return 0 on success, negative value on error
 */
int send_output_tensor(const char *tensor_data, size_t data_size,
                       char *response, size_t response_size);

/**
 * @brief Gets the current edge token string
 *
 * @return Pointer to the token string (includes "Bearer " prefix),
 *         or empty string if not set
 */
const char *get_edge_token(void);

/**
 * @brief Gets the current edge ID
 *
 * @return Pointer to the edge ID string, or empty string if not set
 */
const char *get_edge_id(void);

/**
 * @brief Parses metadata response from the SSL server
 *
 * @param response The response string from the server
 * @return 0 on success, negative value on error
 */
int parse_metadata_response(const char *response);

/**
 * @brief Gets the match result from a parsed metadata response
 *
 * @param response The response string from the server
 * @param match_result Buffer to store the match result string
 * @param buffer_size Size of the match_result buffer
 * @return 0 on success, negative value on error
 */
int get_match_result(const char *response, char *match_result,
                     size_t buffer_size);

/**
 * @brief Sends the match result to the SSL server result endpoint
 *
 * @param match_result The match result string to send
 * @param result_size Size of the match result data
 * @return 0 on success, negative value on error
 */
int send_result(const char *match_result, size_t result_size);

/**
 * @brief Cleans up the SSL keep-alive connection handler
 *
 * This function should be called when the SSL client is no longer needed
 * to properly close any open connections and free resources.
 *
 * @return 0 on success, negative value on error
 */
int ssl_client_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* SSL_CLIENT_H */
