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
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

if(EVP_MOCK EQUAL 1)
  set(ELF 1)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -pthread")
  set(EXECUTABLE_FILES ${EXECUTABLE_FILES}
    ${MOCKS_DIR}/evp/simple_evp.c
  )
endif()

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
elseif(APPS_SELECTION STREQUAL "apitest")
  message(STATUS "apitest detected")
  add_compile_definitions(MOCK_APITEST)
endif()

if(MOCK EQUAL 1)
  message(STATUS "Using mock")
  set(EXECUTABLE_FILES ${EXECUTABLE_FILES}
    ${MOCKS_DIR}/sensor/mock_sensor.cpp
    ${MOCKS_DIR}/sensor/testing_utils.cpp
    ${MOCKS_DIR}/device/mock_device.cpp
    ${MOCKS_DIR}/log/mock_log.cpp
  )
  include_directories(
    ${SAMPLE_APP_DIR}/include
    ${SAMPLE_APP_DIR}/include/schemas
    ${LIBS_DIR}/third_party/flatbuffers/include
    ${ROOT_DIR}/include
    ${MOCKS_DIR}
    ${MOCKS_DIR}/sensor
    ${MOCKS_DIR}/device
    ${LIBS_DIR}/sensor/src/
  )
endif()

add_subdirectory(${ROOT_DIR} ${CMAKE_BINARY_DIR}/build)
