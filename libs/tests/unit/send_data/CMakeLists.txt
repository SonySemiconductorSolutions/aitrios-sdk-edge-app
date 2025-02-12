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
project(send_data_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(SEND_DATA_ROOT_DIR ${LIBS_DIR}/send_data)
set(SEND_DATA_SRC_DIR ${SEND_DATA_ROOT_DIR}/src)
set(SEND_DATA_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

include_directories(
  ${ROOT_DIR}/include
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/send_data/src/data_export/include
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/third_party/flatbuffers/include
  ${LIBS_DIR}/third_party/base64.c
  ${LIBS_DIR}/sm/include
  ${SEND_DATA_ROOT_DIR}/src/process_format
  ${SEND_DATA_ROOT_DIR}/include
  ${SEND_DATA_SRC_DIR}
  ${SEND_DATA_TEST_DIR}
  ${MOCKS_DIR}
  ${MOCKS_DIR}/sm
  ${MOCKS_DIR}/device
)

add_library(data_export
  ${LIBS_DIR}/tests/mocks/data_export/mock_data_export.cpp
)
target_include_directories(data_export PUBLIC
  ${LIBS_DIR}/tests/mocks/data_export
)

add_library(sensor
  ${LIBS_DIR}/tests/mocks/sensor/mock_sensor.cpp
  ${LIBS_DIR}/tests/mocks/sensor/testing_utils.cpp
)
target_include_directories(sensor PUBLIC
  ${LIBS_DIR}/tests/mocks/sensor
)

add_library(sm
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
)
target_include_directories(sm PUBLIC
  ${LIBS_DIR}/sm/include
  ${ROOT_DIR}/include
  ${LIBS_DIR}/third_party/parson
)

add_library(device
  ${MOCKS_DIR}/device/mock_device.cpp
)
target_include_directories(device PUBLIC
  ${LIBS_DIR}/tests/mocks/device
)

add_library(process_format
  ${MOCKS_DIR}/send_data/process_format/mock_process_format.cpp
)
target_include_directories(process_format PUBLIC
  ${LIBS_DIR}/tests/mocks/send_data/process_format
)

add_subdirectory(${LIBS_DIR}/common/src ${CMAKE_BINARY_DIR}/common)
add_subdirectory(${LIBS_DIR}/log/src ${CMAKE_BINARY_DIR}/log)

add_library(send_data STATIC
  ${SEND_DATA_SRC_DIR}/send_data.cpp
)
target_link_libraries(send_data log GTest::gtest_main GTest::gmock_main)
target_link_libraries(sensor send_data)

macro(add_test_executable TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp ${LIBS_DIR}/third_party/parson/parson.c ${LIBS_DIR}/third_party/base64.c/base64.c ${ARGN})
  target_link_libraries(${TEST_NAME} send_data data_export sensor sm device process_format GTest::gtest_main GTest::gmock_main)
  gtest_discover_tests(${TEST_NAME})
endmacro()

include(GoogleTest)
add_test_executable(test_send_data)
