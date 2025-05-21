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

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -pthread -fPIC")

# Root Directories
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)

# Base source files
file(GLOB BINDING_SOURCES "src/bindings/*.cpp")
set(SOURCE_FILES ${SOURCE_FILES}
  ${LIBS_DIR}/third_party/parson/parson.c
  src/py_data_export.cpp
  src/py_edge_app.cpp
  src/py_module.cpp
  src/py_sm.cpp
  ${BINDING_SOURCES}
)

include_directories(
  src/bindings
  src/includes
)

# Allow manually setting of EVP_MOCK (OFF by default)
option(EVP_MOCK "Force using mock EVP Agent implementation" OFF)

# Detect SensCord
if(NOT SENSCORD_INSTALL_PATH)
  message(WARNING "##### SENSCORD_INSTALL_PATH is not set. SensCord will be mocked.")
  set(SENSCORD_MOCK ON)
else()
  message(WARNING "##### SENSCORD_INSTALL_PATH is set. Using real SensCord.")
  set(SENSCORD_MOCK OFF)
endif()

# Build logic
if(SENSCORD_MOCK)
  message(WARNING "##### Fully mock build.")
  # Fully mock build
  list(APPEND SOURCE_FILES
    ${MOCKS_DIR}/evp/simple_evp.c
    ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
    ${MOCKS_DIR}/sensor/mock_sensor.cpp
    ${MOCKS_DIR}/sensor/testing_utils.cpp
    ${MOCKS_DIR}/device/mock_device.cpp
    ${MOCKS_DIR}/log/mock_log.cpp
  )
  add_compile_definitions(PYTHON_MODULE MOCK_PASSTHROUGH)

  include_directories(
    ${SAMPLE_APP_DIR}/include
    ${SAMPLE_APP_DIR}/include/schemas
    ${LIBS_DIR}/third_party/flatbuffers/include
    ${LIBS_DIR}/third_party/parson
    ${ROOT_DIR}/include
    ${ROOT_DIR}/include/edgeapp
    ${MOCKS_DIR}
    ${MOCKS_DIR}/sensor
    ${MOCKS_DIR}/device
    ${LIBS_DIR}/sensor/src/
    ${LIBS_DIR}/common/include
    ${LIBS_DIR}/log/include
    ${LIBS_DIR}/sm/include
    ${LIBS_DIR}/depend/edge_app
  )

elseif(EVP_MOCK)
  message(WARNING "##### SensCord real + EVP Agent mock.")
  # SensCord real + EVP Agent mock
  list(APPEND SOURCE_FILES
    ${MOCKS_DIR}/evp/simple_evp.c
    ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
    ${MOCKS_DIR}/device/mock_device.cpp
    src/py_mocks.cpp
  )
  add_compile_definitions(PYTHON_MODULE)

else()
  message(WARNING "##### Both SensCord and EVP Agent real.")
  # Both SensCord and EVP Agent real
  list(APPEND SOURCE_FILES
    ${MOCKS_DIR}/device/mock_device.cpp
    src/py_mocks.cpp
  )
  add_compile_definitions(PYTHON_MODULE)
endif()


# Dependencies
add_subdirectory(${LIBS_DIR} ${CMAKE_BINARY_DIR}/build)
# For now using the passthrough data_processor until a data_processor can be implemeneted in Python
add_subdirectory(${SAMPLE_APP_DIR}/passthrough/data_processor/src ${CMAKE_BINARY_DIR}/build/data_processor)
set(PYBIND11_FINDPYTHON ON)
add_subdirectory(${LIBS_DIR}/third_party/pybind11 ${CMAKE_BINARY_DIR}/build/pybind11)

# Build the pybind11 module
pybind11_add_module(_edge_app_sdk ${SOURCE_FILES})

# Linking
target_link_libraries(_edge_app_sdk
  PRIVATE
    edge-app-lib
    data_processor_api
)

if(NOT SENSCORD_MOCK)
  list(APPEND CMAKE_PREFIX_PATH ${SENSCORD_INSTALL_PATH})
  find_package(senscord REQUIRED)
  target_link_libraries(_edge_app_sdk PRIVATE senscord)

  if(NOT EVP_MOCK)
    set(EVP_LIB ${LIBS_DIR}/py/lib/libevp-app-sdk-bundle.a)
    target_link_libraries(_edge_app_sdk PRIVATE ${EVP_LIB})
  endif()
endif()

# Install Python module
install(TARGETS _edge_app_sdk DESTINATION edge_app_sdk)

# Generate typing stub files after build
add_custom_command(
  TARGET _edge_app_sdk POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/py.typed"
  COMMAND "${Python_EXECUTABLE}" -m pybind11_stubgen --exit-code -o "${CMAKE_CURRENT_BINARY_DIR}" _edge_app_sdk
  VERBATIM
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/_edge_app_sdk.pyi"
    "${CMAKE_CURRENT_BINARY_DIR}/py.typed"
  DESTINATION edge_app_sdk
)
