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
project(draw_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(AITRIOS_DRAW_ROOT_DIR ${LIBS_DIR}/draw)
set(AITRIOS_DRAW_SRC_DIR ${DRAW_API_ROOT_DIR}/src)
set(AITRIOS_DRAW_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(${ROOT_DIR}/cmake/gtest.cmake)

add_subdirectory (
  ${ROOT_DIR}/libs/log/src
  ${CMAKE_CURRENT_BINARY_DIR}/log
)
add_subdirectory (
  ${AITRIOS_DRAW_ROOT_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/draw
)

add_executable(test_draw
  test_draw.cpp
)
target_link_libraries(test_draw
  draw
  GTest::gtest_main
  GTest::gmock_main
)
target_include_directories(test_draw PUBLIC
  ${ROOT_DIR}/include
  ${AITRIOS_DRAW_TEST_DIR}
)

gtest_discover_tests(test_draw)
