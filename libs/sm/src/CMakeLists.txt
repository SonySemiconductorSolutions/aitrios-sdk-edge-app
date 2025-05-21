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
set(SM_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SM_SRC_DIR ${SM_ROOT_DIR}/src)
set(SM_INC_DIR ${SM_ROOT_DIR}/include)

add_library(edge-app-lib-sm STATIC
  ${SM_SRC_DIR}/main.cpp
  ${SM_SRC_DIR}/sm_configurator.cpp
  ${SM_SRC_DIR}/sm_context.cpp
  ${SM_SRC_DIR}/sm_api.cpp
  ${SM_SRC_DIR}/sm_core.cpp
  ${SM_SRC_DIR}/callbacks/configuration.cpp
  ${SM_SRC_DIR}/states/applying.cpp
  ${SM_SRC_DIR}/states/creating.cpp
  ${SM_SRC_DIR}/states/destroying.cpp
  ${SM_SRC_DIR}/states/coolingdown.cpp
  ${SM_SRC_DIR}/states/idle.cpp
  ${SM_SRC_DIR}/states/running.cpp
  ${SM_SRC_DIR}/states/running_thread.cpp
  ${SM_SRC_DIR}/states/state_factory.cpp
  ${SM_SRC_DIR}/states/state_utils.cpp
  ${SM_SRC_DIR}/states/state.cpp
)

set(sm_headers
  ${ROOT_DIR}/include/edgeapp/sm.h
  ${ROOT_DIR}/include/edgeapp/sm_types.h
)

set_property(TARGET edge-app-lib-sm PROPERTY PUBLIC_HEADER ${sm_headers})

target_include_directories(edge-app-lib-sm INTERFACE
  $<BUILD_INTERFACE:${ROOT_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(edge-app-lib-sm PRIVATE
  ${ROOT_DIR}/include/edgeapp
  ${SM_SRC_DIR}
  ${SM_INC_DIR}
  ${SM_SRC_DIR}/dtdl_model
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/send_data/src/data_export/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/depend/
  ${LIBS_DIR}/third_party/parson
)

add_subdirectory(dtdl_model)

target_link_libraries(edge-app-lib-sm
  edge-app-lib-common
  edge-app-lib-dtdl-model
)
