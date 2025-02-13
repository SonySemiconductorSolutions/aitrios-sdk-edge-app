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
project(context_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
include(${ROOT_DIR}/cmake/gtest.cmake)

add_subdirectory(${LIBS_DIR}/common/src ${CMAKE_BINARY_DIR}/common)

add_executable(test_context
test_context.cpp
)
target_link_libraries(test_context
  common
  GTest::gtest_main
  GTest::gmock_main
)

add_executable(test_memory_manager
test_memory_manager.cpp
)
target_link_libraries(test_memory_manager
  common
  GTest::gtest_main
  GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(test_context)
gtest_discover_tests(test_memory_manager)
