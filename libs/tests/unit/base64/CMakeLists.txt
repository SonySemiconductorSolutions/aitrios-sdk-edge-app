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

cmake_minimum_required(VERSION 3.14)

project(base64_test)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(AITRIOS_DATA_EXPORT_ROOT_DIR ${LIBS_DIR}/send_data/src/data_export)
set(AITRIOS_DATA_EXPORT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

include_directories(${LIBS_DIR}/third_party/base64.c)

add_executable(base64_test test_base64.c ${LIBS_DIR}/third_party/base64.c/base64.c ${LIBS_DIR}/third_party/base64.c/base64.h)

target_link_libraries(base64_test ${LIBS_DIR}/third_party/base64.c)
