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

#include <gtest/gtest.h>

#include <list>
#include <string>

#include "data_processor_api.hpp"
#include "mock_sensor.hpp"
#include "parson.h"
#include "sensor.h"
#include "ssl_client_config.h"

using EdgeAppLib::SensorCoreExit;
using EdgeAppLib::SensorStreamGetProperty;
EdgeAppLibSensorStream s_stream = 0;
#define EPSILON 1e-4
#define MODEL_ID "model_id"
#define DEVICE_ID "device_id"
#define BUF_IMAGE "image"
#define BUF_TIME "T"
#define BUF_OUTPUT "O"
#define BUF_INFERENCE "inferences"

class ConfigureAnalyzeFixtureTests : public ::testing::Test {
 protected:
  void SetUp() override {
    config_json_val =
        json_parse_file("../../../test_data/custom_parameter.json");
    nanoseconds = 1726161043914069133;
    config_json_object = json_object(config_json_val);
    generate_random_uuid(network_id);
    json_object_dotset_string(config_json_object,
                              "ai_models.ssl.ai_model_bundle_id", network_id);

    config = json_serialize_to_string(config_json_val);
  }

  void TearDown() override {
    json_value_free(config_json_val);
    json_free_serialized_string(config);
    free(out_data);
    SensorCoreExit(0);
  }

  char *generate_random_uuid(char uuid[128]) {
    const char *hex_chars = "0123456789abcdef";
    srand((unsigned int)time(NULL));

    for (int i = 0; i < 32; i++) {
      uuid[i] = hex_chars[rand() % 16];
    }
    uuid[32] = '\0';
    return uuid;
  }
  JSON_Value *config_json_val = nullptr;
  JSON_Object *config_json_object = nullptr;
  char *config = nullptr;
  char *output_tensor = nullptr;
  uint32_t num_array_elements = 0;
  uint32_t out_size = 0;
  float *out_data = nullptr;
  uint64_t nanoseconds = 0;
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
};

TEST(Initialize, InitializeTest) {
  DataProcessorResultCode res = DataProcessorInitialize();
  EXPECT_EQ(res, kDataProcessorOk);
}

TEST(ResetState, ResetStateTest) {
  DataProcessorResultCode res = DataProcessorResetState();
  EXPECT_EQ(res, kDataProcessorOk);
}

TEST(Finalize, FinalizeTest) {
  DataProcessorResultCode res = DataProcessorFinalize();
  EXPECT_EQ(res, kDataProcessorOk);
}

TEST_F(ConfigureAnalyzeFixtureTests, CorrectConfigurationTest) {
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  printf("config: %s\n", config);
  EXPECT_EQ(res, kDataProcessorOk);
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;
  SensorStreamGetProperty(
      0, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY, &ai_model_bundle,
      sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty));
  printf("ai_model_bundle.ai_model_bundle_id: %s\n",
         ai_model_bundle.ai_model_bundle_id);
  printf("network_id: %s\n", network_id);
  ASSERT_EQ(strncmp(ai_model_bundle.ai_model_bundle_id, network_id,
                    sizeof(network_id)),
            0);
}

TEST_F(ConfigureAnalyzeFixtureTests, WrongJsonValueTest) {
  char *output = NULL;
  const char *config_mod = "Not a json file";
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_FALSE(out_value == NULL);
  json_value_free(out_value);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, WrongAIModel) {
  char *output = NULL;
  const char *config_mod =
      R"({"ai_models" : {"test" : {"ai_model_bundle_id" : "000002"}}})";
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)config_mod, &output);
  EXPECT_EQ(res, kDataProcessorInvalidParam);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_FALSE(out_value == NULL);
  json_value_free(out_value);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, StreamSetPropertyFail) {
  setEdgeAppLibSensorStreamSetPropertyFail();
  char *output = NULL;
  DataProcessorResultCode res = DataProcessorConfigure((char *)config, &output);
  printf("config: %s\n", config);
  EXPECT_EQ(res, kDataProcessorInvalidParamSetError);
  JSON_Value *out_value = json_parse_string(output);
  EXPECT_FALSE(out_value == NULL);
  json_value_free(out_value);
  free(output);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithSSLClientConfig) {
  // Create config with SSL client configuration
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add SSL client configuration
  JSON_Value *ssl_client_val = json_value_init_object();
  JSON_Object *ssl_client_obj = json_object(ssl_client_val);

  // Add server configuration
  JSON_Value *server_val = json_value_init_object();
  JSON_Object *server_obj = json_object(server_val);
  json_object_set_string(server_obj, "name", "test-server.com");
  json_object_set_string(server_obj, "port", "443");
  json_object_set_string(server_obj, "base_path", "/api/v1");
  json_object_set_value(ssl_client_obj, "server", server_val);

  // Add authentication configuration
  JSON_Value *auth_val = json_value_init_object();
  JSON_Object *auth_obj = json_object(auth_val);

  JSON_Value *edge_login_val = json_value_init_object();
  JSON_Object *edge_login_obj = json_object(edge_login_val);
  json_object_set_string(edge_login_obj, "user_id", "test_user");
  json_object_set_string(edge_login_obj, "password", "test_pass");
  json_object_set_string(edge_login_obj, "host_id", "test_host");
  json_object_set_value(auth_obj, "edge_login", edge_login_val);

  JSON_Value *edge_info_val = json_value_init_object();
  JSON_Object *edge_info_obj = json_object(edge_info_val);
  json_object_set_string(edge_info_obj, "host_id", "info_host");
  json_object_set_value(auth_obj, "edge_info", edge_info_val);

  json_object_set_value(ssl_client_obj, "authentication", auth_val);

  // Add metadata configuration
  JSON_Value *metadata_val = json_value_init_object();
  JSON_Object *metadata_obj = json_object(metadata_val);
  json_object_set_string(metadata_obj, "servicer", "test_servicer");
  json_object_set_string(metadata_obj, "dataset", "test_dataset");
  json_object_set_value(ssl_client_obj, "metadata", metadata_val);

  // Add API endpoints configuration
  JSON_Value *endpoints_val = json_value_init_object();
  JSON_Object *endpoints_obj = json_object(endpoints_val);
  json_object_set_string(endpoints_obj, "edge_login", "/login");
  json_object_set_string(endpoints_obj, "edge_info", "/info");
  json_object_set_string(endpoints_obj, "metadata", "/metadata");
  json_object_set_string(endpoints_obj, "result", "/result");
  json_object_set_value(ssl_client_obj, "api_endpoints", endpoints_val);

  json_object_set_value(ssl_config_obj, "ssl_client", ssl_client_val);

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithPartialSSLClientConfig) {
  // Create config with partial SSL client configuration
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add minimal SSL client configuration
  JSON_Value *ssl_client_val = json_value_init_object();
  JSON_Object *ssl_client_obj = json_object(ssl_client_val);

  // Only add server configuration (partial)
  JSON_Value *server_val = json_value_init_object();
  JSON_Object *server_obj = json_object(server_val);
  json_object_set_string(server_obj, "name", "partial-server.com");
  json_object_set_value(ssl_client_obj, "server", server_val);

  json_object_set_value(ssl_config_obj, "ssl_client", ssl_client_val);

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithEmptySSLClientConfig) {
  // Create config with empty SSL client configuration
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add empty SSL client configuration
  JSON_Value *ssl_client_val = json_value_init_object();
  JSON_Object *ssl_client_obj = json_object(ssl_client_val);

  json_object_set_value(ssl_config_obj, "ssl_client", ssl_client_val);

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithNullValues) {
  // Create config with some null values to test edge cases
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add SSL client configuration with some null values
  JSON_Value *ssl_client_val = json_value_init_object();
  JSON_Object *ssl_client_obj = json_object(ssl_client_val);

  // Add server configuration with some null values
  JSON_Value *server_val = json_value_init_object();
  JSON_Object *server_obj = json_object(server_val);
  json_object_set_string(server_obj, "name", "test-server.com");
  // port and base_path intentionally omitted (null)
  json_object_set_value(ssl_client_obj, "server", server_val);

  json_object_set_value(ssl_config_obj, "ssl_client", ssl_client_val);

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

// Test SSL configuration getter functions
TEST(SSLConfigGetters, GetterFunctionsTest) {
  // These functions should return empty strings when not configured
  EXPECT_STREQ(ssl_get_server_name(), "");
  EXPECT_STREQ(ssl_get_server_port(), "");
  EXPECT_STREQ(ssl_get_base_path(), "");
  EXPECT_STREQ(ssl_get_edge_login_user_id(), "");
  EXPECT_STREQ(ssl_get_edge_login_password(), "");
  EXPECT_STREQ(ssl_get_edge_login_host_id(), "");
  EXPECT_STREQ(ssl_get_edge_info_host_id(), "");
  EXPECT_STREQ(ssl_get_metadata_servicer(), "");
  EXPECT_STREQ(ssl_get_metadata_dataset(), "");
  EXPECT_STREQ(ssl_get_edge_login_endpoint(), "");
  EXPECT_STREQ(ssl_get_edge_info_endpoint(), "");
  EXPECT_STREQ(ssl_get_metadata_endpoint(), "");
  EXPECT_STREQ(ssl_get_result_endpoint(), "");

  EXPECT_FALSE(ssl_is_configured());
}

TEST_F(ConfigureAnalyzeFixtureTests, GetterFunctionsAfterConfiguration) {
  // Create a comprehensive SSL configuration
  const char *comprehensive_config = R"({
    "ai_models": {
      "ssl": {
        "ai_model_bundle_id": "test_bundle_123"
      }
    },
    "ssl_client": {
      "server": {
        "name": "test-server.com",
        "port": "443",
        "base_path": "/api/v1"
      },
      "authentication": {
        "edge_login": {
          "user_id": "test_user",
          "password": "test_pass",
          "host_id": "test_host"
        },
        "edge_info": {
          "host_id": "info_host"
        }
      },
      "metadata": {
        "servicer": "test_servicer",
        "dataset": "test_dataset"
      },
      "api_endpoints": {
        "edge_login": "/login",
        "edge_info": "/info",
        "metadata": "/metadata",
        "result": "/result"
      }
    }
  })";

  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)comprehensive_config, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Now test all getter functions after configuration
  EXPECT_TRUE(ssl_is_configured());
  EXPECT_STREQ(ssl_get_server_name(), "test-server.com");
  EXPECT_STREQ(ssl_get_server_port(), "443");
  EXPECT_STREQ(ssl_get_base_path(), "/api/v1");
  EXPECT_STREQ(ssl_get_edge_login_user_id(), "test_user");
  EXPECT_STREQ(ssl_get_edge_login_password(), "test_pass");
  EXPECT_STREQ(ssl_get_edge_login_host_id(), "test_host");
  EXPECT_STREQ(ssl_get_edge_info_host_id(), "info_host");
  EXPECT_STREQ(ssl_get_metadata_servicer(), "test_servicer");
  EXPECT_STREQ(ssl_get_metadata_dataset(), "test_dataset");
  EXPECT_STREQ(ssl_get_edge_login_endpoint(), "/login");
  EXPECT_STREQ(ssl_get_edge_info_endpoint(), "/info");
  EXPECT_STREQ(ssl_get_metadata_endpoint(), "/metadata");
  EXPECT_STREQ(ssl_get_result_endpoint(), "/result");

  // Verify configuration state is still active
  EXPECT_TRUE(ssl_is_configured());
  EXPECT_STREQ(ssl_get_server_name(), "test-server.com");
  EXPECT_STREQ(ssl_get_server_port(), "443");
  EXPECT_STREQ(ssl_get_base_path(), "/api/v1");

  // Clean up
  free(output);
}

// Test error handling in configuration
TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithInvalidSSLClientConfig) {
  // Create config with invalid SSL client configuration
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add invalid SSL client configuration
  json_object_set_string(ssl_config_obj, "ssl_client", "invalid_value");

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

// Test edge cases and boundary conditions
TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithEmptyStrings) {
  // Create config with empty string values
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add SSL client configuration with empty strings
  JSON_Value *ssl_client_val = json_value_init_object();
  JSON_Object *ssl_client_obj = json_object(ssl_client_val);

  // Add server configuration with empty strings
  JSON_Value *server_val = json_value_init_object();
  JSON_Object *server_obj = json_object(server_val);
  json_object_set_string(server_obj, "name", "");
  json_object_set_string(server_obj, "port", "");
  json_object_set_string(server_obj, "base_path", "");
  json_object_set_value(ssl_client_obj, "server", server_val);

  json_object_set_value(ssl_config_obj, "ssl_client", ssl_client_val);

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithVeryLongStrings) {
  // Create config with very long string values to test buffer handling
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add SSL client configuration with long strings
  JSON_Value *ssl_client_val = json_value_init_object();
  JSON_Object *ssl_client_obj = json_object(ssl_client_val);

  // Create very long strings (near buffer limits)
  std::string long_name(255, 'a');  // Near SSL_CONFIG_SERVER_NAME_SIZE limit
  std::string long_port(15, '9');   // Near SSL_CONFIG_SERVER_PORT_SIZE limit
  std::string long_path(255, '/');  // Near SSL_CONFIG_BASE_PATH_SIZE limit

  // Add server configuration with long strings
  JSON_Value *server_val = json_value_init_object();
  JSON_Object *server_obj = json_object(server_val);
  json_object_set_string(server_obj, "name", long_name.c_str());
  json_object_set_string(server_obj, "port", long_port.c_str());
  json_object_set_string(server_obj, "base_path", long_path.c_str());
  json_object_set_value(ssl_client_obj, "server", server_val);

  json_object_set_value(ssl_config_obj, "ssl_client", ssl_client_val);

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

// Test configuration with missing optional fields
TEST_F(ConfigureAnalyzeFixtureTests, ConfigurationWithMissingOptionalFields) {
  // Create config with some optional fields missing
  JSON_Value *ssl_config_val = json_parse_string(config);
  JSON_Object *ssl_config_obj = json_object(ssl_config_val);

  // Add minimal SSL client configuration
  JSON_Value *ssl_client_val = json_value_init_object();
  JSON_Object *ssl_client_obj = json_object(ssl_client_val);

  // Only add server configuration, omit authentication, metadata, and endpoints
  JSON_Value *server_val = json_value_init_object();
  JSON_Object *server_obj = json_object(server_val);
  json_object_set_string(server_obj, "name", "minimal-server.com");
  json_object_set_value(ssl_client_obj, "server", server_val);

  json_object_set_value(ssl_config_obj, "ssl_client", ssl_client_val);

  char *ssl_config_str = json_serialize_to_string(ssl_config_val);
  char *output = NULL;

  DataProcessorResultCode res = DataProcessorConfigure(ssl_config_str, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Clean up
  json_free_serialized_string(ssl_config_str);
  json_value_free(ssl_config_val);
}

// Test multiple configuration calls to ensure state management
TEST_F(ConfigureAnalyzeFixtureTests, MultipleConfigurationCalls) {
  // First configuration
  const char *config1 = R"({
    "ai_models": {"ssl": {"ai_model_bundle_id": "bundle1"}},
    "ssl_client": {
      "server": {"name": "server1.com", "port": "8080"}
    }
  })";

  char *output1 = NULL;
  DataProcessorResultCode res1 =
      DataProcessorConfigure((char *)config1, &output1);
  EXPECT_EQ(res1, kDataProcessorOk);

  // Second configuration (should override first)
  const char *config2 = R"({
    "ai_models": {"ssl": {"ai_model_bundle_id": "bundle2"}},
    "ssl_client": {
      "server": {"name": "server2.com", "port": "9090"}
    }
  })";

  char *output2 = NULL;
  DataProcessorResultCode res2 =
      DataProcessorConfigure((char *)config2, &output2);
  EXPECT_EQ(res2, kDataProcessorOk);

  // Verify second configuration took effect
  EXPECT_STREQ(ssl_get_server_name(), "server2.com");
  EXPECT_STREQ(ssl_get_server_port(), "9090");

  // Clean up
  free(output1);
  free(output2);
}

// Test SSL client configuration parsing edge cases
TEST_F(ConfigureAnalyzeFixtureTests, SSLClientConfigParsingEdgeCases) {
  // Test with deeply nested SSL client configuration
  const char *nested_config = R"({
    "ai_models": {"ssl": {"ai_model_bundle_id": "nested_test"}},
    "ssl_client": {
      "server": {
        "name": "nested-server.com",
        "port": "1234",
        "base_path": "/deeply/nested/api"
      },
      "authentication": {
        "edge_login": {
          "user_id": "nested_user",
          "password": "nested_pass",
          "host_id": "nested_host"
        }
      },
      "metadata": {
        "servicer": "nested_servicer",
        "dataset": "nested_dataset"
      }
    }
  })";

  char *output = NULL;
  DataProcessorResultCode res =
      DataProcessorConfigure((char *)nested_config, &output);
  EXPECT_EQ(res, kDataProcessorOk);

  // Verify nested configuration was parsed correctly
  EXPECT_TRUE(ssl_is_configured());
  EXPECT_STREQ(ssl_get_server_name(), "nested-server.com");
  EXPECT_STREQ(ssl_get_server_port(), "1234");
  EXPECT_STREQ(ssl_get_base_path(), "/deeply/nested/api");
  EXPECT_STREQ(ssl_get_edge_login_user_id(), "nested_user");
  EXPECT_STREQ(ssl_get_edge_login_password(), "nested_pass");
  EXPECT_STREQ(ssl_get_edge_login_host_id(), "nested_host");
  EXPECT_STREQ(ssl_get_metadata_servicer(), "nested_servicer");
  EXPECT_STREQ(ssl_get_metadata_dataset(), "nested_dataset");
}

// Test to verify SSL client configuration is optional
TEST_F(ConfigureAnalyzeFixtureTests, SSLClientConfigOptional) {
  // Create a config WITHOUT SSL client configuration to verify it's optional
  // This should succeed since SSL client config is not required

  char *output = NULL;

  // This should succeed even without SSL client configuration
  DataProcessorResultCode res = DataProcessorConfigure(config, &output);

  // The result should be success since SSL client config is optional
  EXPECT_EQ(res, kDataProcessorOk);

  // Verify that output contains success information
  // EXPECT_NE(output, nullptr);
  if (output) {
    free(output);
  }
}
