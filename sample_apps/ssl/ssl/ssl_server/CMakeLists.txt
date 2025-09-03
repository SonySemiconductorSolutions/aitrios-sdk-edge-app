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

# Define SSL server source files
set(SSL_SERVER_SOURCES
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_server/ssl_server.c
  ${SAMPLE_APP_DIR}/ssl/ssl/ssl_server/ssl_server_certs.c
)

# Create SSL server API library
add_library(ssl_server_api ${SSL_SERVER_SOURCES})

# Set include directories for SSL server API
target_include_directories(ssl_server_api PUBLIC
    ${ROOT_DIR}/include
    ${SAMPLE_APP_DIR}/utils/include
    ${SAMPLE_APP_DIR}/ssl/ssl/ssl_server
    ${SAMPLE_APP_DIR}/ssl/third_party/mbedtls/include
    ${SAMPLE_APP_DIR}/ssl/third_party/mbedtls/tests/include/test
)

# Link required libraries
if(SSL_WASM_BUILD)
    find_package(wamr-wasi-socket REQUIRED)
    target_link_libraries(ssl_server_api
        log
        mbedtls
        mbedx509
        mbedcrypto
        pthread
        wamr-wasi-socket
    )
else()
    target_link_libraries(ssl_server_api
        log
        mbedtls
        mbedx509
        mbedcrypto
        pthread
    )
endif()
