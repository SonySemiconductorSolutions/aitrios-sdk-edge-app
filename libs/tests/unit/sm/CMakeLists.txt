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
project(sm_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(SM_ROOT_DIR ${LIBS_DIR}/sm)
set(SM_SRC_DIR ${SM_ROOT_DIR}/src)
set(SM_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)

include_directories(
  ${ROOT_DIR}/include
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/send_data/src/data_export/include
  ${LIBS_DIR}/receive_data/include
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/depend/edge_app
  ${SM_ROOT_DIR}/include
  ${SM_SRC_DIR}
  ${SM_SRC_DIR}/dtdl_model
  ${SM_TEST_DIR}
  ${MOCKS_DIR}
)

add_library(sensor
  ${LIBS_DIR}/tests/mocks/sensor/mock_sensor.cpp
  ${LIBS_DIR}/tests/mocks/device/mock_device.cpp
  ${LIBS_DIR}/tests/mocks/sensor/testing_utils.cpp
)
target_include_directories(sensor PUBLIC
  ${LIBS_DIR}/tests/mocks/sensor
  ${LIBS_DIR}/tests/mocks/device
)

add_subdirectory(${LIBS_DIR}/common/src ${CMAKE_BINARY_DIR}/common)
add_subdirectory(${LIBS_DIR}/log/src ${CMAKE_BINARY_DIR}/log)

add_library(sm STATIC
  ${MOCKS_DIR}/evp/mock_evp.cpp
  ${MOCKS_DIR}/event_functions/mock_sm.cpp
  ${MOCKS_DIR}/data_export/mock_data_export.cpp
  ${MOCKS_DIR}/receive_data/mock_receive_data.cpp
  ${LIBS_DIR}/common/src/context.cpp
  ${SM_SRC_DIR}/callbacks/configuration.cpp
  ${SM_SRC_DIR}/states/state.cpp
  ${SM_SRC_DIR}/states/state_factory.cpp
  ${SM_SRC_DIR}/states/state_utils.cpp
  ${SM_SRC_DIR}/states/applying.cpp
  ${SM_SRC_DIR}/states/creating.cpp
  ${SM_SRC_DIR}/states/destroying.cpp
  ${SM_SRC_DIR}/states/coolingdown.cpp
  ${SM_SRC_DIR}/states/idle.cpp
  ${SM_SRC_DIR}/states/running.cpp
  ${SM_SRC_DIR}/states/running_thread.cpp
  ${SM_SRC_DIR}/sm_configurator.cpp
  ${SM_SRC_DIR}/sm_context.cpp
  ${SM_SRC_DIR}/sm_api.cpp
)
target_link_libraries(sm GTest::gtest_main GTest::gmock_main)
target_link_libraries(sensor sm)

add_library(fixtures
  ${SM_TEST_DIR}/fixtures/common_fixture.cpp
  ${SM_TEST_DIR}/fixtures/state_fixture.cpp
)

add_subdirectory(dtdl_model)

macro(add_test_executable TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp ${ARGN})
  target_link_libraries(${TEST_NAME} sm fixtures dtdl_model sm GTest::gtest_main GTest::gmock_main)
  gtest_discover_tests(${TEST_NAME})
endmacro()

include(GoogleTest)
add_test_executable(test_idle)
add_test_executable(test_coolingdown)
add_test_executable(test_running)
add_test_executable(test_creating)
add_test_executable(test_destroying)
add_test_executable(test_applying)
add_test_executable(test_state_utils)
add_test_executable(test_sm_core ${SM_ROOT_DIR}/src/sm_core.cpp)
add_test_executable(test_sm_context)
add_test_executable(test_configuration_callback ${SM_ROOT_DIR}/src/sm_core.cpp)
add_test_executable(test_pq_configurator)
add_test_executable(test_state_factory)
add_test_executable(test_running_thread)
add_test_executable(test_sm_api)
add_test_executable(test_sm_configurator)
