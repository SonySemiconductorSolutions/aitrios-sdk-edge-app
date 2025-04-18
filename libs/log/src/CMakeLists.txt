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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(LOGS_API_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(LOGS_API_SRC_DIR ${LOGS_API_ROOT_DIR}/src)

add_library(log STATIC
  ${LOGS_API_SRC_DIR}/log_internal.cpp
  ${LOGS_API_SRC_DIR}/log_private.cpp
  ${LOGS_API_SRC_DIR}/log.cpp
  ${LOGS_API_SRC_DIR}/log_common.cpp
)

target_include_directories(log PRIVATE
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${LOGS_API_ROOT_DIR}/include
)

if(NOT DEFINED LOGDISABLE)
  target_compile_definitions(log PUBLIC LOGSDISABLE)
endif()
