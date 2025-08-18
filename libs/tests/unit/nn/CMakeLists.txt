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
project(nn_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(NN_ROOT_DIR ${LIBS_DIR}/nn)
set(NN_SRC_DIR ${NN_ROOT_DIR}/src)
set(NN_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include(${ROOT_DIR}/cmake/gtest.cmake)


add_subdirectory (
  ${NN_ROOT_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/nn
)

add_subdirectory (
  ${LIBS_DIR}/draw/src
  ${CMAKE_CURRENT_BINARY_DIR}/draw
)

add_subdirectory (
  ${LIBS_DIR}/log/src
  ${CMAKE_CURRENT_BINARY_DIR}/log
)

add_executable(test_nn
  test_nn.cpp
  ${MOCKS_DIR}/nn/mock_wasi_nn.c
)
target_link_libraries(test_nn
  nn
  GTest::gtest_main
  GTest::gmock_main
)
target_include_directories(test_nn PUBLIC
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/third_party/wasi_nn
  ${NN_TEST_DIR}
)

gtest_discover_tests(test_nn)
