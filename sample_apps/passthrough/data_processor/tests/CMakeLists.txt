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
project(passthrough_data_processor_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

add_executable(test_passthrough_data_processor
  test_passthrough_data_processor.cpp
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
  ${LIBS_DIR}/tests/mocks/sensor/testing_utils.cpp
  ${LIBS_DIR}/third_party/base64.c/base64.c
  ${MOCKS_DIR}/device/mock_device.cpp
)

add_subdirectory (
  ${LIBS_DIR}/log/src
  ${CMAKE_CURRENT_BINARY_DIR}/libs/log/src
)

add_subdirectory (
  ${SAMPLE_APP_DIR}/passthrough/data_processor/src
  ${CMAKE_CURRENT_BINARY_DIR}/sample_apps/passthrough/data_processor/src
)

target_include_directories (test_passthrough_data_processor PUBLIC
    ${LIBS_DIR}/third_party/parson
    ${LIBS_DIR}/tests/mocks/sensor
    ${LIBS_DIR}/third_party/base64.c
)

add_library(sensor
  ${LIBS_DIR}/tests/mocks/sensor/mock_sensor.cpp
  ${LIBS_DIR}/tests/mocks/device/mock_device.cpp
  ${LIBS_DIR}/tests/mocks/sensor/testing_utils.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
)
target_include_directories(sensor PUBLIC
  ${LIBS_DIR}/tests/mocks/sensor
  ${SAMPLE_APP_DIR}/utils/include
  ${LIBS_DIR}/sm/include
  ${ROOT_DIR}/include
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/depend/edge_app
  ${MOCKS_DIR}/device
)

target_link_libraries(test_passthrough_data_processor
  data_processor_api
  sensor
  stdc++
  log
  GTest::gtest_main
)

gtest_discover_tests(test_passthrough_data_processor)
