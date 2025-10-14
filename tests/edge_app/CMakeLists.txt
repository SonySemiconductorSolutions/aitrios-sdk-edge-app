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

project(edge_app_mock)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

if(EVP_MOCK EQUAL 1)
  set(ELF 1)
endif()

add_compile_definitions(MOCK_INTEGRATION_TEST)

#  TODO: split in per application part
if(APPS_SELECTION STREQUAL "classification")
  message(STATUS "classification detected")
  add_compile_definitions(MOCK_CLASSIFICATION)
elseif(APPS_SELECTION STREQUAL "detection")
message(STATUS "detection detected")
  add_compile_definitions(MOCK_DETECTION)
elseif(APPS_SELECTION STREQUAL "segmentation")
  message(STATUS "segmentation detected")
  add_compile_definitions(MOCK_SEGMENTATION)
elseif(APPS_SELECTION STREQUAL "passthrough")
  message(STATUS "passthrough detected")
  add_compile_definitions(MOCK_PASSTHROUGH)
elseif(APPS_SELECTION STREQUAL "barcode")
  message(STATUS "barcode detected")
  add_compile_definitions(MOCK_BARCODE)
elseif(APPS_SELECTION STREQUAL "lp_recog")
  message(STATUS "lp_recog detected")
  add_compile_definitions(MOCK_LP_RECOG)
elseif(APPS_SELECTION STREQUAL "apitest")
  message(STATUS "apitest detected")
  add_compile_definitions(MOCK_APITEST)
elseif(APPS_SELECTION STREQUAL "gaze")
  message(STATUS "gaze detected")
  add_compile_definitions(MOCK_GAZE)
elseif(APPS_SELECTION STREQUAL "posenet")
  message(STATUS "posenet detected")
  add_compile_definitions(MOCK_POSENET)
elseif(APPS_SELECTION STREQUAL "switch_dnn")
  message(STATUS "switch_dnn detected")
  add_compile_definitions(MOCK_SWITCH)
endif()

if(MOCK EQUAL 1)
  message(STATUS "Using mock")
endif()

add_subdirectory(${ROOT_DIR} ${CMAKE_BINARY_DIR}/build)
