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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(AITRIOS_DATA_EXPORT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(data_export STATIC
  ${AITRIOS_DATA_EXPORT_ROOT_DIR}/src/data_export.cpp
)

target_include_directories(data_export PUBLIC
  ${AITRIOS_DATA_EXPORT_ROOT_DIR}/include
  ${AITRIOS_DATA_EXPORT_ROOT_DIR}/src
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/depend/edge_app/evp_c_sdk
  ${LIBS_DIR}/sm/src
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/send_data/src/process_format
)

target_link_libraries(data_export
  common log sm process_format
)
