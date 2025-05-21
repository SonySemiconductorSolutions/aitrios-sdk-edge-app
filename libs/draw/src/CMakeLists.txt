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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(AITRIOS_DRAW_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(AITRIOS_DRAW_SRC_DIR ${AITRIOS_DRAW_ROOT_DIR}/src)

add_library(edge-app-lib-draw STATIC
  ${AITRIOS_DRAW_SRC_DIR}/draw.cpp
)

set(draw_headers
  ${ROOT_DIR}/include/edgeapp/draw.h
)

set_property(TARGET edge-app-lib-draw PROPERTY PUBLIC_HEADER ${draw_headers})

target_include_directories(edge-app-lib-draw INTERFACE
  $<BUILD_INTERFACE:${ROOT_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(edge-app-lib-draw PRIVATE
  ${ROOT_DIR}/include/edgeapp
)

target_link_libraries(edge-app-lib-draw
  edge-app-lib-log
)
