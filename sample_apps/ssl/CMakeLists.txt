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

cmake_minimum_required(VERSION 3.14)
project(edge_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

add_subdirectory(libs ${CMAKE_BINARY_DIR}/edgeapplib)

set(MAX_MEMORY 10485760)
set(INITIAL_MEMORY 2097152)
include(edge_app)

set(SAMPLE_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_executable(edge_app
  src/sm.cpp
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
)
add_subdirectory(ssl)
add_subdirectory(data_processor/src)
target_link_libraries(edge_app ssl_client_api ssl_server_api data_processor_api edge_app_lib)
