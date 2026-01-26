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

set(OPENCV_VERSION 4.6.0)
set(OPENCV_INSTALL_DIR /opt/opencv-${OPENCV_VERSION}-install/)
if(MOCK_INTEGRATION_TEST)
  set(OPENCV_INSTALL_CMAKE_DIR ${OPENCV_INSTALL_DIR}/install/lib/cmake/opencv4/)
else()
  set(OPENCV_INSTALL_CMAKE_DIR ${OPENCV_INSTALL_DIR}/install-wasi/lib/cmake/opencv4/)
endif()
set(OPENCV_INCLUDE_DIR ${OPENCV_INSTALL_CMAKE_DIR}/../../../include/opencv4)
set(OPENCV_LIB_DIR ${OPENCV_INSTALL_CMAKE_DIR}/../../../lib)
set(OPENCV_LIB_STATIC
  "${OPENCV_LIB_DIR}/libopencv_imgproc.a"
  "${OPENCV_LIB_DIR}/libopencv_core.a"
)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OPENCV_INCLUDE_DIR})

set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
link_directories(${OPENCV_LIB_DIR})

add_subdirectory(libs ${CMAKE_BINARY_DIR}/edgeapplib)

set(MAX_MEMORY 209715200)
set(INITIAL_MEMORY 209715200)
include(edge_app)

set(SAMPLE_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_executable(edge_app
  src/sm.cpp
  src/preprocess_lib.cpp
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
)
target_compile_options(edge_app PRIVATE -fexceptions)
target_compile_options(edge_app PRIVATE -ffunction-sections -fdata-sections)
target_link_options(edge_app PRIVATE "-Wl,--gc-sections")
add_subdirectory(data_processor/src)
target_link_libraries(edge_app ${OPENCV_LIB_STATIC} data_processor_api edge_app_lib)
