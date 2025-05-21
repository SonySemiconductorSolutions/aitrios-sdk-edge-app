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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

add_library(edge-app-lib-sensor-mock STATIC
  ${MOCKS_DIR}/sensor/mock_sensor.cpp
  ${MOCKS_DIR}/sensor/testing_utils.cpp
  ${MOCKS_DIR}/device/mock_device.cpp
)

#  TODO: split in per application part
if(APPS_SELECTION STREQUAL "classification")
  message(STATUS "classification detected")
  target_compile_definitions(edge-app-lib-sensor-mock PRIVATE MOCK_CLASSIFICATION)
elseif(APPS_SELECTION STREQUAL "detection")
  message(STATUS "detection detected")
  target_compile_definitions(edge-app-lib-sensor-mock PRIVATE MOCK_DETECTION)
elseif(APPS_SELECTION STREQUAL "segmentation")
  message(STATUS "segmentation detected")
  target_compile_definitions(edge-app-lib-sensor-mock PRIVATE MOCK_SEGMENTATION)
elseif(APPS_SELECTION STREQUAL "passthrough")
  message(STATUS "passthrough detected")
  target_compile_definitions(edge-app-lib-sensor-mock PRIVATE MOCK_PASSTHROUGH)
elseif(APPS_SELECTION STREQUAL "apitest")
  message(STATUS "apitest detected")
  target_compile_definitions(edge-app-lib-sensor-mock PRIVATE MOCK_APITEST)
endif()

target_include_directories(edge-app-lib-sensor-mock PRIVATE
  ${LIBS_DIR}/third_party/parson
  ${ROOT_DIR}/include
  ${ROOT_DIR}/include/edgeapp
  ${MOCKS_DIR}
  ${MOCKS_DIR}/sensor
  ${MOCKS_DIR}/device
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/sensor/src/
  ${LIBS_DIR}/depend/edge_app
)

add_library(edge-app-lib-log-mock STATIC
  ${LIBS_DIR}/log/src/log_common.cpp
  ${MOCKS_DIR}/log/mock_log.cpp
)

target_compile_definitions(edge-app-lib-log-mock PRIVATE MOCK_INTEGRATION_TEST)

target_include_directories(edge-app-lib-log-mock PRIVATE
  ${ROOT_DIR}/include/edgeapp
  ${LIBS_DIR}/log/include
)

add_library(edge-app-lib-evp-mock STATIC
  ${MOCKS_DIR}/evp/simple_evp.c
  ${MOCKS_DIR}/log/mock_log.cpp
)

target_include_directories(edge-app-lib-evp-mock PRIVATE
  ${ROOT_DIR}/include
  ${ROOT_DIR}/include/edgeapp
  ${MOCKS_DIR}
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/sensor/src/
  ${LIBS_DIR}/depend/edge_app
)
