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
#include <stdlib.h>
#include <string.h>

#include "data_processor_utils.hpp"
#include "device.h"
#include "log.h"
#include "parson.h"
#include "sensor.h"
#include "sm_types.h"
#include "sm_utils.hpp"
#include "ssl_client_config.h"
extern EdgeAppLibSensorStream s_stream;

using EdgeAppLib::SensorStreamGetProperty;

#define MODEL_NAME "ssl"

// Forward declarations
static void parse_ssl_client_config(JSON_Object *object,
                                    char **out_config_json);
static void parse_ssl_server_config(JSON_Object *ssl_client_obj);
static void parse_ssl_auth_config(JSON_Object *ssl_client_obj);
static void parse_ssl_metadata_config(JSON_Object *ssl_client_obj);
static void parse_ssl_api_endpoints(JSON_Object *ssl_client_obj);

// Global SSL client configuration
static ssl_client_config_t ssl_config = {0};

DataProcessorResultCode DataProcessorInitialize() {
  LOG_INFO(
      "Successful call, although empty implementation of "
      "DataProcessorInitialize. App will continue to work normally");
  return kDataProcessorOk;
}

DataProcessorResultCode DataProcessorResetState() {
  LOG_INFO(
      "Successful call, although empty implementation of "
      "DataProcessorResetState. App will continue to work normally");
  return kDataProcessorOk;
}

DataProcessorResultCode DataProcessorFinalize() {
  LOG_INFO(
      "Successful call, although empty implementation of "
      "DataProcessorFinalize. App will continue to work normally");
  return kDataProcessorOk;
}

DataProcessorResultCode DataProcessorConfigure(char *config_json,
                                               char **out_config_json) {
  LOG_INFO("config_json:%s\n", config_json);
  JSON_Value *value = json_parse_string(config_json);
  if (value == nullptr) {
    const char *error_msg = "Error parsing custom settings JSON";
    LOG_ERR("%s", error_msg);
    *out_config_json =
        GetConfigureErrorJson(ResponseCodeInvalidArgument, error_msg, "");
    return kDataProcessorInvalidParam;
  }

  JSON_Object *object = json_object(value);

  // extract parameters
  JSON_Object *object_model =
      json_object_dotget_object(object, "ai_models." MODEL_NAME);
  if (object_model == nullptr) {
    const char *error_msg =
        "Error accessing AI model parameters in JSON object.";
    LOG_ERR("%s", error_msg);
    *out_config_json = GetConfigureErrorJson(
        ResponseCodeInvalidArgument, error_msg,
        json_object_dotget_string(object, "res_info.res_id"));
    json_value_free(value);
    return kDataProcessorInvalidParam;
  }

  // Parse SSL client configuration
  parse_ssl_client_config(object, out_config_json);

  LOG_INFO("Setting EdgeAppLibNetwork...");

  DataProcessorResultCode res = kDataProcessorOk;
  if (SetEdgeAppLibNetwork(s_stream, object_model) != 0) {
    res = kDataProcessorInvalidParamSetError;
    *out_config_json = json_serialize_to_string(value);
  }

  json_value_free(value);
  return res;
}

// Function to parse SSL client configuration
static void parse_ssl_client_config(JSON_Object *object,
                                    char **out_config_json) {
  // Parse SSL client configuration
  JSON_Object *ssl_client_obj = json_object_get_object(object, "ssl_client");
  if (ssl_client_obj != nullptr) {
    LOG_INFO("Parsing SSL client configuration...");

    parse_ssl_server_config(ssl_client_obj);
    parse_ssl_auth_config(ssl_client_obj);
    parse_ssl_metadata_config(ssl_client_obj);
    parse_ssl_api_endpoints(ssl_client_obj);

    ssl_config.is_configured = true;
    LOG_INFO("SSL client configuration loaded successfully");
  } else {
    // SSL client configuration is optional, not required
    LOG_INFO("No SSL client configuration provided, using defaults");
    ssl_config.is_configured = false;
  }

  return;
}

// Function to parse SSL server configuration
static void parse_ssl_server_config(JSON_Object *ssl_client_obj) {
  JSON_Object *server_obj = json_object_get_object(ssl_client_obj, "server");
  if (server_obj != nullptr) {
    const char *server_name = json_object_get_string(server_obj, "name");
    const char *server_port = json_object_get_string(server_obj, "port");
    const char *base_path = json_object_get_string(server_obj, "base_path");

    if (server_name)
      snprintf(ssl_config.server_name, sizeof(ssl_config.server_name), "%s",
               server_name);
    if (server_port)
      snprintf(ssl_config.server_port, sizeof(ssl_config.server_port), "%s",
               server_port);
    if (base_path)
      snprintf(ssl_config.base_path, sizeof(ssl_config.base_path), "%s",
               base_path);

    LOG_INFO("Server config: %s%s:%s", ssl_config.server_name,
             ssl_config.base_path, ssl_config.server_port);
  }
}

// Function to parse SSL authentication configuration
static void parse_ssl_auth_config(JSON_Object *ssl_client_obj) {
  JSON_Object *auth_obj =
      json_object_get_object(ssl_client_obj, "authentication");
  if (auth_obj != nullptr) {
    JSON_Object *edge_login_obj =
        json_object_get_object(auth_obj, "edge_login");
    JSON_Object *edge_info_obj = json_object_get_object(auth_obj, "edge_info");

    if (edge_login_obj != nullptr) {
      const char *user_id = json_object_get_string(edge_login_obj, "user_id");
      const char *password = json_object_get_string(edge_login_obj, "password");
      const char *host_id = json_object_get_string(edge_login_obj, "host_id");

      if (user_id)
        snprintf(ssl_config.edge_login_user_id,
                 sizeof(ssl_config.edge_login_user_id), "%s", user_id);
      if (password)
        snprintf(ssl_config.edge_login_password,
                 sizeof(ssl_config.edge_login_password), "%s", password);
      if (host_id)
        snprintf(ssl_config.edge_login_host_id,
                 sizeof(ssl_config.edge_login_host_id), "%s", host_id);
    }

    if (edge_info_obj != nullptr) {
      const char *host_id = json_object_get_string(edge_info_obj, "host_id");
      if (host_id)
        snprintf(ssl_config.edge_info_host_id,
                 sizeof(ssl_config.edge_info_host_id), "%s", host_id);
    }

    LOG_INFO("Auth config: user=%s, login_host=%s, info_host=%s",
             ssl_config.edge_login_user_id, ssl_config.edge_login_host_id,
             ssl_config.edge_info_host_id);
  }
}

// Function to parse SSL metadata configuration
static void parse_ssl_metadata_config(JSON_Object *ssl_client_obj) {
  JSON_Object *metadata_obj =
      json_object_get_object(ssl_client_obj, "metadata");
  if (metadata_obj != nullptr) {
    const char *servicer = json_object_get_string(metadata_obj, "servicer");
    const char *dataset = json_object_get_string(metadata_obj, "dataset");

    if (servicer)
      snprintf(ssl_config.metadata_servicer,
               sizeof(ssl_config.metadata_servicer), "%s", servicer);
    if (dataset)
      snprintf(ssl_config.metadata_dataset, sizeof(ssl_config.metadata_dataset),
               "%s", dataset);

    LOG_INFO("Metadata config: servicer=%s, dataset=%s",
             ssl_config.metadata_servicer, ssl_config.metadata_dataset);
  }
}

// Function to parse SSL API endpoints configuration
static void parse_ssl_api_endpoints(JSON_Object *ssl_client_obj) {
  JSON_Object *endpoints_obj =
      json_object_get_object(ssl_client_obj, "api_endpoints");
  if (endpoints_obj != nullptr) {
    const char *edge_login_endpoint =
        json_object_get_string(endpoints_obj, "edge_login");
    const char *edge_info_endpoint =
        json_object_get_string(endpoints_obj, "edge_info");
    const char *metadata_endpoint =
        json_object_get_string(endpoints_obj, "metadata");
    const char *result_endpoint =
        json_object_get_string(endpoints_obj, "result");

    if (edge_login_endpoint) {
      snprintf(ssl_config.edge_login_endpoint,
               sizeof(ssl_config.edge_login_endpoint), "%s",
               edge_login_endpoint);
      LOG_INFO("Updated edge login endpoint: %s",
               ssl_config.edge_login_endpoint);
    }
    if (edge_info_endpoint) {
      snprintf(ssl_config.edge_info_endpoint,
               sizeof(ssl_config.edge_info_endpoint), "%s", edge_info_endpoint);
      LOG_INFO("Updated edge info endpoint: %s", ssl_config.edge_info_endpoint);
    }
    if (metadata_endpoint) {
      snprintf(ssl_config.metadata_endpoint,
               sizeof(ssl_config.metadata_endpoint), "%s", metadata_endpoint);
      LOG_INFO("Updated metadata endpoint: %s", ssl_config.metadata_endpoint);
    }
    if (result_endpoint) {
      snprintf(ssl_config.result_endpoint, sizeof(ssl_config.result_endpoint),
               "%s", result_endpoint);
      LOG_INFO("Updated result endpoint: %s", ssl_config.result_endpoint);
    }
  }
}

// SSL Client Configuration Getter Functions
extern "C" {

const char *ssl_get_server_name() {
  return ssl_config.is_configured ? ssl_config.server_name : "";
}

const char *ssl_get_server_port() {
  return ssl_config.is_configured ? ssl_config.server_port : "";
}

const char *ssl_get_base_path() {
  return ssl_config.is_configured ? ssl_config.base_path : "";
}

const char *ssl_get_edge_login_user_id() {
  return ssl_config.is_configured ? ssl_config.edge_login_user_id : "";
}

const char *ssl_get_edge_login_password() {
  return ssl_config.is_configured ? ssl_config.edge_login_password : "";
}

const char *ssl_get_edge_login_host_id() {
  return ssl_config.is_configured ? ssl_config.edge_login_host_id : "";
}

const char *ssl_get_edge_info_host_id() {
  return ssl_config.is_configured ? ssl_config.edge_info_host_id : "";
}

const char *ssl_get_metadata_servicer() {
  return ssl_config.is_configured ? ssl_config.metadata_servicer : "";
}

const char *ssl_get_metadata_dataset() {
  return ssl_config.is_configured ? ssl_config.metadata_dataset : "";
}

const char *ssl_get_edge_login_endpoint() {
  return ssl_config.is_configured ? ssl_config.edge_login_endpoint : "";
}

const char *ssl_get_edge_info_endpoint() {
  return ssl_config.is_configured ? ssl_config.edge_info_endpoint : "";
}

const char *ssl_get_metadata_endpoint() {
  return ssl_config.is_configured ? ssl_config.metadata_endpoint : "";
}

const char *ssl_get_result_endpoint() {
  return ssl_config.is_configured ? ssl_config.result_endpoint : "";
}

bool ssl_is_configured() { return ssl_config.is_configured; }
}
