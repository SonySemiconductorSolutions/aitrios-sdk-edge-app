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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(AITRIOS_SEND_DATA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(edge-app-lib-send-data STATIC
  ${AITRIOS_SEND_DATA_ROOT_DIR}/src/send_data.cpp
)

set(send_data_headers
  ${ROOT_DIR}/include/edgeapp/send_data.h
  ${ROOT_DIR}/include/edgeapp/send_data_types.h
)

set_property(TARGET edge-app-lib-send-data PROPERTY PUBLIC_HEADER ${send_data_headers})

target_include_directories(edge-app-lib-send-data INTERFACE
  $<BUILD_INTERFACE:${ROOT_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(edge-app-lib-send-data PRIVATE
  ${ROOT_DIR}/include/edgeapp
  ${AITRIOS_SEND_DATA_ROOT_DIR}/include
  ${AITRIOS_SEND_DATA_ROOT_DIR}/src
  ${AITRIOS_SEND_DATA_ROOT_DIR}/src/process_format
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/third_party/parson
)

add_subdirectory(process_format)

target_link_libraries(edge-app-lib-send-data
  edge-app-lib-data-export
  edge-app-lib-process-format
  edge-app-lib-common
  edge-app-lib-log
)
