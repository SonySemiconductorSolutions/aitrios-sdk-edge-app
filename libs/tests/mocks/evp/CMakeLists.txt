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
project(evp_mock)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)

set(LOGS_API_ROOT_DIR ${LIBS_DIR}/log)
set(LOGS_API_SRC_DIR ${LOGS_API_ROOT_DIR}/src)

set(PROJECT_SOURCES

   mock_evp_wrapper.cpp

)

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})

add_library(log STATIC
  ${LOGS_API_SRC_DIR}/log_internal.cpp
  ${LOGS_API_SRC_DIR}/log_private.cpp
  ${LOGS_API_SRC_DIR}/log.cpp
  ${LOGS_API_SRC_DIR}/log_common.cpp
)
set_target_properties(log PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(log PRIVATE
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${LOGS_API_ROOT_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE log)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_PREFIX_PATH}/include
    ${ROOT_DIR}/wasm-micro-runtime/core/iwasm/include
    ${ROOT_DIR}/include
    ${ROOT_DIR}/libs/common/include
    ${ROOT_DIR}/libs/send_data/src/data_export/include
    ${ROOT_DIR}/libs/depend/edge_app
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
