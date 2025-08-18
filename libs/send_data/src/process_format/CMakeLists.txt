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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(PROCESS_FORMAT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(process_format STATIC
  ${PROCESS_FORMAT_ROOT_DIR}/process_format.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
  ${LIBS_DIR}/third_party/base64.c/base64.c
)

target_include_directories(process_format PUBLIC
  ${PROCESS_FORMAT_ROOT_DIR}
  ${ROOT_DIR}/include
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/third_party/base64.c
  ${LIBS_DIR}/depend/edge_app
)

target_link_libraries(process_format
  common log sm
)
