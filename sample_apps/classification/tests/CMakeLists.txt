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
project(data_processor_utils)

set(UNIT_TEST 1)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(SAMPLE_APP_TEST_DIR ${SAMPLE_APP_DIR}/tests)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

add_library(send_data
  ${LIBS_DIR}/tests/mocks/send_data/mock_send_data.cpp
)
target_include_directories(send_data PUBLIC
  ${LIBS_DIR}/send_data/src/process_format
  ${ROOT_DIR}/include
  ${LIBS_DIR}/tests/mocks/send_data/process_format
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/common/include
)

include_directories(
  ${MOCKS_DIR}
)

add_subdirectory (
  ${LIBS_DIR}/log/src
  ${CMAKE_CURRENT_BINARY_DIR}/libs/log/src
)

add_subdirectory (
  ${SAMPLE_APP_DIR}/classification/data_processor/src
  ${CMAKE_CURRENT_BINARY_DIR}/sample_apps/classification/data_processor/src
)

add_executable(test_event_functions
  test_sm.cpp
  ${MOCKS_DIR}/sensor/mock_sensor.cpp
  ${MOCKS_DIR}/device/mock_device.cpp
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
  ${MOCKS_DIR}/sensor/testing_utils.cpp
  ${MOCKS_DIR}/data_processor_api/mock_data_processor_api.cpp
  ${MOCKS_DIR}/data_export/mock_data_export.cpp
  ${SAMPLE_APP_DIR}/classification/src/sm.cpp
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
)

target_include_directories (test_event_functions PUBLIC
  ${SAMPLE_APP_DIR}/utils/include
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/third_party/flatbuffers/include
  ${LIBS_DIR}/tests/unit/data_export/
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/send_data/src/data_export/include
  ${LIBS_DIR}/sm/include
  ${MOCKS_DIR}/sensor
  ${MOCKS_DIR}/device
)

target_link_libraries(
  test_event_functions
  log
  send_data
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_event_functions)
