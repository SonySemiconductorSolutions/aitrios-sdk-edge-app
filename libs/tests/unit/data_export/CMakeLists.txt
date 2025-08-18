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
set(AITRIOS_DATA_EXPORT_ROOT_DIR ${LIBS_DIR}/send_data/src/data_export)
set(AITRIOS_DATA_EXPORT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

add_library(sm
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
)
target_include_directories(sm PUBLIC
  ${LIBS_DIR}/sm/include
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
  ${AITRIOS_DATA_EXPORT_ROOT_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/data_export
)

add_subdirectory(
  ${LIBS_DIR}/send_data/src/process_format/
  ${CMAKE_CURRENT_BINARY_DIR}/process_format
)

macro(add_test_executable TEST_NAME)
add_executable(${TEST_NAME} ${TEST_NAME}.cpp
  ${MOCKS_DIR}/evp/mock_evp.cpp
  ${AITRIOS_DATA_EXPORT_TEST_DIR}/fixtures/data_export_fixture.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
  ${MOCKS_DIR}/device/mock_device.cpp
  ${MOCKS_DIR}/sensor/mock_sensor.cpp
  ${MOCKS_DIR}/sensor/testing_utils.cpp
  ${ARGN}
)
target_link_libraries(${TEST_NAME} data_export common GTest::gtest_main GTest::gmock_main)
target_include_directories (${TEST_NAME} PUBLIC
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${MOCKS_DIR}
  ${MOCKS_DIR}/device/
  ${MOCKS_DIR}/sensor/
  ${AITRIOS_DATA_EXPORT_TEST_DIR}
  ${LIBS_DIR}/send_data/src/data_export/include
)
gtest_discover_tests(${TEST_NAME})
endmacro()

add_test_executable(test_data_export)
add_test_executable(test_map)
