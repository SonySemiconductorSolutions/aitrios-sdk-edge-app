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
project(edge_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(TESTS_DIR ${ROOT_DIR}/tests)
set(LIBS_DIR ${ROOT_DIR}/libs)

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

if(NOT DEFINED ELF)
  include(${ROOT_DIR}/cmake/config_common.cmake)
endif()

add_subdirectory(${LIBS_DIR})

# Add the appropriate subdirectory based on the parameter
if(NOT APPS_SELECTION STREQUAL "apitest")
  set(EXECUTABLE_FILES ${EXECUTABLE_FILES}
    ${SAMPLE_APP_DIR}/${APPS_SELECTION}/src/sm.cpp
  )
else()
  set(EXECUTABLE_FILES ${EXECUTABLE_FILES}
    ${TESTS_DIR}/${APPS_SELECTION}/src/sm.cpp
  )
endif()


if(NOT APPS_SELECTION STREQUAL "apitest")
  add_subdirectory(${SAMPLE_APP_DIR}/${APPS_SELECTION}/data_processor/src)
else()
  add_subdirectory(${TESTS_DIR}/${APPS_SELECTION}/data_processor/src)
endif()
if(NOT APPS_SELECTION STREQUAL "switch_dnn")
set(EXECUTABLE_FILES ${EXECUTABLE_FILES}
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
)
endif()
set(TARGET_LIBS data_processor_api)

add_executable(edge_app ${EXECUTABLE_FILES})
target_link_libraries(edge_app ${TARGET_LIBS} edge_app_lib)
