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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(AITRIOS_RECEIVE_DATA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(receive_data STATIC
  ${AITRIOS_RECEIVE_DATA_ROOT_DIR}/src/receive_data.cpp
)

target_include_directories(receive_data PUBLIC
  ${AITRIOS_RECEIVE_DATA_ROOT_DIR}/include
  ${AITRIOS_RECEIVE_DATA_ROOT_DIR}/src
  ${LIBS_DIR}/depend/edge_app/evp_c_sdk
  ${ROOT_DIR}/include
)

target_link_libraries(receive_data
  common log
)
