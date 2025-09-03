# Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Define directory paths
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)

# Define SSL client source files
set(SSL_CLIENT_SOURCES
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_client/ssl_client_auth.c
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_client/ssl_client_core.c
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_client/ssl_client_metadata.c
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_client/ssl_client_result.c
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_client/ssl_client_keepalive.c
)

# Create SSL client API library
add_library(ssl_client_api ${SSL_CLIENT_SOURCES})

# Set include directories for SSL client API
target_include_directories(ssl_client_api PUBLIC
  ${ROOT_DIR}/include
  ${SAMPLE_APP_DIR}/utils/include
  ${SAMPLE_APP_DIR}/ssl/src
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_client
  ${SAMPLE_APP_DIR}/ssl/third_party/mbedtls/include
  ${LIBS_DIR}/third_party/parson
)

# Link required libraries
if(SSL_WASM_BUILD)
    find_package(wamr-wasi-socket REQUIRED)
    target_link_libraries(ssl_client_api
      log
      mbedtls
      mbedcrypto
      mbedx509
      parson
      wamr-wasi-socket
    )
else()
    target_link_libraries(ssl_client_api
      log
      mbedtls
      mbedcrypto
      mbedx509
      parson
    )
endif()
