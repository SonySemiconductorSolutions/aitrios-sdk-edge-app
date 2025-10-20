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
project(test_edgeapp_core)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(THIRD_DIR ${ROOT_DIR}/libs/third_party)
set(NN_ROOT_DIR ${LIBS_DIR}/nn)
set(NN_SRC_DIR ${NN_ROOT_DIR}/src)
set(NN_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

add_subdirectory (
  ${ROOT_DIR}/libs/log/src
  ${CMAKE_CURRENT_BINARY_DIR}/log
)
add_subdirectory(${LIBS_DIR}/common/src ${CMAKE_BINARY_DIR}/common)
add_subdirectory(${LIBS_DIR}/draw/src ${CMAKE_BINARY_DIR}/draw)

add_executable(test_edgeapp_core
  ${LIBS_DIR}/nn/src/edgeapp_core.cpp
  test_edgeapp.cpp
  ${MOCKS_DIR}/nn/mock_wasi_nn.c
  ${MOCKS_DIR}/nn/mock_nn.cpp
  ${MOCKS_DIR}/sensor/mock_sensor.cpp
  ${MOCKS_DIR}/data_export/mock_data_export.cpp
  ${MOCKS_DIR}/send_data/mock_send_data.cpp
  ${MOCKS_DIR}/device/mock_device.cpp
  ${MOCKS_DIR}/sensor/testing_utils.cpp
  ${MOCKS_DIR}/sm/mock_sm_api.cpp
  ${MOCKS_DIR}/receive_data/mock_receive_data.cpp
  ${THIRD_DIR}/parson/parson.c
)
target_link_libraries(test_edgeapp_core
  log
  common
  draw
  GTest::gtest_main
  GTest::gmock_main
)
target_include_directories(test_edgeapp_core PUBLIC
  ${LIBS_DIR}/common/include
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/receive_data/include
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/send_data/src/data_export/include
  ${LIBS_DIR}/send_data/src/process_format
  ${MOCKS_DIR}
  ${MOCKS_DIR}/nn/
  ${MOCKS_DIR}/device
  ${MOCKS_DIR}/sensor
  ${MOCKS_DIR}/data_export
  ${MOCKS_DIR}/send_data/process_format
  ${THIRD_DIR}/parson
  ${THIRD_DIR}/wasi_nn
  ${NN_TEST_DIR}
)

gtest_discover_tests(test_edgeapp_core)
