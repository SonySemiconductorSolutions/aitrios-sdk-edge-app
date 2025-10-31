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
project(receive_data_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(RECEIVE_DATA_ROOT_DIR ${LIBS_DIR}/receive_data)
set(RECEIVE_DATA_ROOT_DIR_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

add_library(sm
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
)
target_include_directories(sm PUBLIC
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/depend/edge_app
  ${ROOT_DIR}/include
  ${LIBS_DIR}/third_party/parson
)

add_subdirectory (
  ${ROOT_DIR}/libs/log/src
  ${CMAKE_CURRENT_BINARY_DIR}/log
)
add_subdirectory (
  ${LIBS_DIR}/common/src
  ${CMAKE_CURRENT_BINARY_DIR}/common
)
add_subdirectory (
  ${RECEIVE_DATA_ROOT_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/receive_data
)

macro(add_test_executable TEST_NAME)
add_executable(${TEST_NAME} ${TEST_NAME}.cpp
  ${MOCKS_DIR}/evp/mock_evp.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
  ${MOCKS_DIR}/device/mock_device.cpp
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
  ${MOCKS_DIR}/sensor/testing_utils.cpp
  ${ARGN}
)
target_link_libraries(${TEST_NAME} receive_data common GTest::gtest_main GTest::gmock_main)
target_include_directories (${TEST_NAME} PUBLIC
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${MOCKS_DIR}
  ${MOCKS_DIR}/device/
  ${MOCKS_DIR}/sensor/
  ${RECEIVE_DATA_ROOT_DIR_TEST_DIR}
  ${LIBS_DIR}/receive_data/include
  ${LIBS_DIR}/receive_data/src
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/sm/src
)
gtest_discover_tests(${TEST_NAME})
endmacro()

add_test_executable(test_receive_data)
