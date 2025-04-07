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

project(edge_app_py)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)

set(EVP_MOCK 1)
if(NOT SENSCORD_INSTALL_PATH)
  set(MOCK 1)
else()
  set(MOCK 0)
endif()
set(ELF 1)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -pthread -fPIC")
set(SOURCE_FILES ${SOURCE_FILES}
  ${MOCKS_DIR}/evp/simple_evp.c
  ${LIBS_DIR}/third_party/parson/parson.c
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
)

add_compile_definitions(PYTHON_MODULE)

if(MOCK EQUAL 1)
  message(STATUS "Using mock")

  # For the data provided by mock sensor
  add_compile_definitions(MOCK_PASSTHROUGH)

  set(SOURCE_FILES ${SOURCE_FILES}
    ${MOCKS_DIR}/sensor/mock_sensor.cpp
    ${MOCKS_DIR}/sensor/testing_utils.cpp
    ${MOCKS_DIR}/device/mock_device.cpp
    ${MOCKS_DIR}/log/mock_log.cpp
  )
  include_directories(
    ${SAMPLE_APP_DIR}/include
    ${SAMPLE_APP_DIR}/include/schemas
    ${LIBS_DIR}/third_party/flatbuffers/include
    ${LIBS_DIR}/third_party/parson
    ${ROOT_DIR}/include
    ${MOCKS_DIR}
    ${MOCKS_DIR}/sensor
    ${MOCKS_DIR}/device
    ${LIBS_DIR}/sensor/src/
  )
else()
  # still need some mocks even when linking to real senscord
  set(SOURCE_FILES ${SOURCE_FILES}
    ${MOCKS_DIR}/device/mock_device.cpp
    src/py_mocks.cpp
  )
endif()

add_subdirectory(${LIBS_DIR} ${CMAKE_BINARY_DIR}/build)

# For now using the passthrough data_processor until a data_processor can be implemeneted in Python
add_subdirectory(${SAMPLE_APP_DIR}/passthrough/data_processor/src ${CMAKE_BINARY_DIR}/build/data_processor)

set(PYBIND11_FINDPYTHON ON)
add_subdirectory(${LIBS_DIR}/third_party/pybind11 ${CMAKE_BINARY_DIR}/build/pybind11)

set(SOURCE_FILES ${SOURCE_FILES}
  src/py_module.cpp
  src/py_sm.cpp
)

pybind11_add_module(_edge_app_sdk ${SOURCE_FILES})
target_link_libraries(_edge_app_sdk PRIVATE edge_app_lib data_processor_api)


if(MOCK EQUAL 0)
  message(STATUS "Linking senscord")

  if(NOT SENSCORD_INSTALL_PATH)
    message(FATAL_ERROR "SENSCORD_INSTALL_PATH variable is not defined.")
  endif()
  list(APPEND CMAKE_PREFIX_PATH ${SENSCORD_INSTALL_PATH})
  find_package(senscord REQUIRED)
  target_link_libraries(_edge_app_sdk PRIVATE senscord)
endif()


install(TARGETS _edge_app_sdk DESTINATION edge_app_sdk)

# Generate typing stubs for the python module
add_custom_command(
    TARGET _edge_app_sdk POST_BUILD
    # Create the py.typed marker file to indicate that the package has type annotations
    COMMAND
    "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/py.typed"
    # Execute pybind11-stubgen, with our python module
    COMMAND
    "${Python_EXECUTABLE}" -m pybind11_stubgen
    --exit-code
    -o "${CMAKE_CURRENT_BINARY_DIR}" _edge_app_sdk
    VERBATIM
)

# Include the generated stub files in the installation
install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/_edge_app_sdk.pyi"
    "${CMAKE_CURRENT_BINARY_DIR}/py.typed"
    DESTINATION edge_app_sdk
)
