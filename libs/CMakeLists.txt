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
project(edge-app-lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(CMAKE_C_COMPILER_TARGET MATCHES "wasm")
  include(../cmake/config_common.cmake)
  set(LINK_MOCK_LIBS OFF)
else()
  add_subdirectory(mock)
  set(LINK_MOCK_LIBS ON)
endif()

add_subdirectory(common/src)
add_subdirectory(log/src)
add_subdirectory(draw/src)
add_subdirectory(sensor/src)
add_subdirectory(send_data/src/data_export/src)
add_subdirectory(send_data/src/)
add_subdirectory(sm/src)
add_subdirectory(compat_headers)

add_library(edge-app-lib INTERFACE)

# Note: edge-app-lib-compat-headers is intentionally excluded here.
# An application which needs the compat is supposed to add it to
# its own target_link_libraries by itself.
target_link_libraries(edge-app-lib INTERFACE
  $<$<BOOL:${LINK_MOCK_LIBS}>:edge-app-lib-sensor-mock>
  edge-app-lib-common
  edge-app-lib-sm
  edge-app-lib-data-export
  edge-app-lib-send-data
  $<$<BOOL:${LINK_MOCK_LIBS}>:edge-app-lib-log-mock>
  edge-app-lib-log
  edge-app-lib-draw
  $<$<BOOL:${LINK_MOCK_LIBS}>:edge-app-lib-evp-mock>
  Threads::Threads
)

install(TARGETS
    edge-app-lib
    edge-app-lib-common
    edge-app-lib-sm
    edge-app-lib-data-export
    edge-app-lib-dtdl-model
    edge-app-lib-process-format
    edge-app-lib-send-data
    edge-app-lib-sensor
    edge-app-lib-log
    edge-app-lib-draw
  EXPORT edge-app-lib-config
  PUBLIC_HEADER DESTINATION include/edgeapp
)

install(TARGETS edge-app-lib-compat-headers
  EXPORT edge-app-lib-config
  INCLUDES DESTINATION include/edgeapp
)

if(LINK_MOCK_LIBS)
install(TARGETS
    edge-app-lib-sensor-mock
    edge-app-lib-log-mock
    edge-app-lib-evp-mock
  EXPORT edge-app-lib-config
)
endif()

install(EXPORT edge-app-lib-config
  DESTINATION lib/cmake/edge-app-lib
)

# install a few cmake utility modules for the convenience of user applications
install(FILES
  ../cmake/edge_app.cmake
  ../cmake/config_common.cmake
  DESTINATION lib/cmake/edge-app
)
