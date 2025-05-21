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

add_library(edge-app-lib-data-export STATIC
  ${AITRIOS_DATA_EXPORT_ROOT_DIR}/src/data_export.cpp
)

set(data_export_headers
  ${ROOT_DIR}/include/edgeapp/data_export.h
  ${ROOT_DIR}/include/edgeapp/data_export_types.h
)

set_property(TARGET edge-app-lib-data-export PROPERTY PUBLIC_HEADER ${data_export_headers})

target_include_directories(edge-app-lib-data-export INTERFACE
  $<BUILD_INTERFACE:${ROOT_DIR}/include>
  $<INSTALL_INTERFACE:include>
)


target_include_directories(edge-app-lib-data-export PRIVATE
  ${ROOT_DIR}/include/edgeapp
  ${AITRIOS_DATA_EXPORT_ROOT_DIR}/include
  ${AITRIOS_DATA_EXPORT_ROOT_DIR}/src
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/depend/edge_app/evp_c_sdk
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/sm/src
  ${LIBS_DIR}/third_party/parson
)

target_link_libraries(edge-app-lib-data-export
  edge-app-lib-common
  edge-app-lib-log
  edge-app-lib-sm
)
