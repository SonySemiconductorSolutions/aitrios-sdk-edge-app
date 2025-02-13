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
project(sensor_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(PARSON_SRC_DIR ${LIBS_DIR}/third_party/parson)
set(AITRIOS_SENSOR_ROOT_DIR ${LIBS_DIR}/sensor)
set(AITRIOS_SENSOR_SRC_DIR ${AITRIOS_SENSOR_ROOT_DIR}/src)
set(AITRIOS_SENSOR_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

include_directories(
  ${ROOT_DIR}/include
  ${LIBS_DIR}/sensor/src
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/third_party
  ${MOCKS_DIR}/device
)

add_subdirectory(${LIBS_DIR}/log/src ${CMAKE_BINARY_DIR}/src/log)
add_subdirectory(${LIBS_DIR}/sensor/src ${CMAKE_BINARY_DIR}/src/sensor)
add_subdirectory(${LIBS_DIR}/common/src  ${CMAKE_BINARY_DIR}/src/common)

set(COMMON_SRC
  ${PARSON_SRC_DIR}/parson.c
  ${MOCKS_DIR}/device/mock_device.cpp
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
  ${MOCKS_DIR}/send_data/process_format/mock_process_format.cpp
)

macro(add_test_executable test_name)
  add_executable(${test_name} ${test_name}.cpp sensor_unit_test.cpp mocks.cpp ${COMMON_SRC} ${AITRIOS_SENSOR_SRC} ${ARGN})
  target_include_directories(${test_name} PRIVATE ${LIBS_DIR}/log/include ${LIBS_DIR}/common/include)
  target_link_libraries(${test_name} sensor log common GTest::gtest_main GTest::gmock_main)
  gtest_discover_tests(${test_name})
endmacro()

add_test_executable(test_sensor_wrapper)
add_test_executable(test_sensor_wrapper_error)

add_test_executable(test_sensor)
