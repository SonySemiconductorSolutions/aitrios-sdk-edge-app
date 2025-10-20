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

cmake_minimum_required(VERSION 3.18)
project(edge_app_rootdir LANGUAGES)

# Define a list of possible apps
set(APPS_LIST
  "passthrough"
  "classification"
  "detection"
  "posenet"
  "gaze"
  "segmentation"
  "switch_dnn"
  "draw"
  "perfbench"
  "apitest"
  "lp_recog"
  "ssl"
)

if(NOT DEFINED APPS_SELECTION)
  set(APPS_SELECTION "passthrough")
endif()

if(APPS_SELECTION IN_LIST APPS_LIST)
  message(STATUS "App selected: ${APPS_SELECTION}")
else()
  message(FATAL_ERROR
    "Invalid APPS_SELECTION: '${APPS_SELECTION}'. "
    "Please select one of the following supported apps: ${APPS_LIST}"
  )
endif()

if(${APPS_SELECTION} STREQUAL apitest)
  set(SAMPLE_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
else()
  set(SAMPLE_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sample_apps)
endif()

# copy a few directories ("include" and "libs") to application.
# note: typical sample applications have their own include directories
# for schema headers. this effectively merges the top-level include
# directory into them.
include(FetchContent)
FetchContent_Declare(edgeapplib-include
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/include ${SAMPLE_APP_DIR}/${APPS_SELECTION}/include
  SOURCE_DIR ${SAMPLE_APP_DIR}/${APPS_SELECTION}/include
)
FetchContent_Declare(edgeapplib-libs
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/libs ${SAMPLE_APP_DIR}/${APPS_SELECTION}/libs
  SOURCE_DIR ${SAMPLE_APP_DIR}/${APPS_SELECTION}/libs
  # arbitrary SOURCE_SUBDIR which doesn't contain CMakeLists.txt.
  # we don't want to make FetchContent_MakeAvailable call add_subdirectory.
  SOURCE_SUBDIR dummy
)
FetchContent_MakeAvailable(edgeapplib-include edgeapplib-libs)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_subdirectory(${SAMPLE_APP_DIR}/${APPS_SELECTION})

add_custom_target(copy-edge_app
  ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy $<TARGET_FILE:edge_app> ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(copy-edge_app edge_app)
