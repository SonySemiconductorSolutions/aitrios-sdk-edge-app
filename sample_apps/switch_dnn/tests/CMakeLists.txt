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
project(switch_dnn)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(APP_DIR ${ROOT_DIR}/sample_apps/switch_dnn)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

add_subdirectory(${ROOT_DIR}/libs/log/src ${CMAKE_BINARY_DIR}/src/log)

add_library(send_data
  ${LIBS_DIR}/tests/mocks/send_data/mock_send_data.cpp
)
target_include_directories(send_data PUBLIC
  ${LIBS_DIR}/send_data/src/process_format
  ${ROOT_DIR}/include
  ${LIBS_DIR}/tests/mocks/send_data/process_format
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/common/include
  ${MOCKS_DIR}/sensor
  ${MOCKS_DIR}/device
)

add_executable(test_switch_dnn
  ${APP_DIR}/tests/test_switch_dnn.cpp
  ${APP_DIR}/src/sm.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
  ${SAMPLE_APP_DIR}/utils/sm_utils.cpp
  ${LIBS_DIR}/tests/mocks/sensor/testing_utils.cpp
)

target_include_directories (test_switch_dnn PUBLIC
  ${ROOT_DIR}/include
  ${APP_DIR}/include/schemas
  ${SAMPLE_APP_DIR}/include
  ${APP_DIR}/data_processor/include
  ${LIBS_DIR}/send_data/src/data_export/include
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/third_party/flatbuffers/include
)

add_executable(test_analyzer
  ${APP_DIR}/tests/test_analyzer.cpp
  ${APP_DIR}/data_processor/src/switch_dnn_analyzer.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
)

target_include_directories (test_analyzer PUBLIC
  ${ROOT_DIR}/include
  ${APP_DIR}/include/schemas
  ${APP_DIR}/data_processor/include
  ${LIBS_DIR}/log/include
  ${LIBS_DIR}/third_party/parson
  ${SAMPLE_APP_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBS_DIR}/third_party/flatbuffers/include
)

target_link_libraries(
  test_switch_dnn
  log
  send_data
  GTest::gtest_main
  GTest::gmock_main
)

target_link_libraries(
  test_analyzer
  log
  GTest::gtest_main
  GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(test_switch_dnn)
gtest_discover_tests(test_analyzer)
