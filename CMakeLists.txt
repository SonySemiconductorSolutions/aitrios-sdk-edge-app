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

cmake_minimum_required(VERSION 3.16)
project(edge_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Define a list of possible apps
set(APPS_LIST "passthrough" "classification" "detection" "posenet" "segmentation" "switch_dnn" "draw" "perfbench" "apitest")

if(NOT DEFINED APPS_SELECTION)
  set(APPS_SELECTION "passthrough")
endif()

if (APPS_SELECTION IN_LIST APPS_LIST)
  message(STATUS "App selected: ${APPS_SELECTION}")
else()
  message(FATAL_ERROR
    "Please if you are defining the `APPS_SELECTION` flag select one of the following: passthrough, classification, detection, posenet, segmentation, switch_dnn, draw, perfbench or apitest"
  )
endif()

set(app_dir ${CMAKE_CURRENT_LIST_DIR}/sample_apps/${APPS_SELECTION})
if (APPS_SELECTION STREQUAL apitest)
  set(app_dir ${CMAKE_CURRENT_LIST_DIR}/tests/${APPS_SELECTION})
endif()

set(tmp_install ${CMAKE_CURRENT_BINARY_DIR}/install)

# Note: edge-app-lib uses APPS_SELECTION only for
# non-wasm targets (libs/mock)
include(ExternalProject)
ExternalProject_Add(edge-app-lib
  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/libs"
  CMAKE_ARGS
    -DWASI_SDK_PREFIX=${WASI_SDK_PREFIX}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_INSTALL_PREFIX=${tmp_install}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DAPPS_SELECTION=${APPS_SELECTION}
  BUILD_COMMAND
    cmake --build .
  INSTALL_COMMAND
    cmake --build . --target install
)

include(ExternalProject)
ExternalProject_Add(edge-app
  SOURCE_DIR
    ${app_dir}
  CMAKE_ARGS
    -DWASI_SDK_PREFIX=${WASI_SDK_PREFIX}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_INSTALL_PREFIX=${tmp_install}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_PREFIX_PATH=${tmp_install}
  BUILD_COMMAND
    cmake --build .
  INSTALL_COMMAND
    cmake --build . --target install
)

add_dependencies(edge-app edge-app-lib)

set(edge_app ${CMAKE_CURRENT_BINARY_DIR}/edge_app)
add_custom_command(
  OUTPUT ${edge_app}
  COMMAND cmake -E copy ${tmp_install}/bin/${APPS_SELECTION} ${edge_app}
  DEPENDS edge-app)
add_custom_target(copy-edge-app ALL DEPENDS ${edge_app})
