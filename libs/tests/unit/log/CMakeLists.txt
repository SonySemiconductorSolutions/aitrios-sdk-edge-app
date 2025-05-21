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

cmake_minimum_required (VERSION 3.14)
project(data_export_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(LOGS_API_ROOT_DIR ${LIBS_DIR}/log)
set(LOGS_API_SRC_DIR ${LOGS_API_ROOT_DIR}/src)
set(LOGS_API_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(${ROOT_DIR}/cmake/gtest.cmake)

include_directories(
  ${LOGS_API_TEST_DIR}
  ${LOGS_API_SRC_DIR}
  ${LOGS_API_ROOT_DIR}/include
  ${ROOT_DIR}/include
  ${ROOT_DIR}/include/edgeapp
)

add_library(log
  ${LOGS_API_SRC_DIR}/log_internal.cpp
  ${LOGS_API_SRC_DIR}/log_private.cpp
  ${LOGS_API_SRC_DIR}/log.cpp
  ${LOGS_API_SRC_DIR}/log_common.cpp
)
add_executable(test_logapi
  ut_logapi.cpp
)
target_link_libraries(test_logapi
  log
  GTest::gtest_main
  GTest::gmock_main
)

add_library(log_logdisable
  ${LOGS_API_SRC_DIR}/log_internal.cpp
  ${LOGS_API_SRC_DIR}/log_private.cpp
  ${LOGS_API_SRC_DIR}/log.cpp
  ${LOGS_API_SRC_DIR}/log_common.cpp
)
target_compile_definitions(log_logdisable PUBLIC LOGDISABLE)
add_executable(test_logapi_logdisable
  ut_logapi_logdisable.cpp
)
target_link_libraries(test_logapi_logdisable
  log_logdisable
  GTest::gtest_main
  GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(test_logapi)
gtest_discover_tests(test_logapi_logdisable)
