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
set(NN_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(NN_SRC_DIR ${NN_ROOT_DIR}/src)
set(NN_INC_DIR ${NN_ROOT_DIR}/include)

add_library(nn STATIC
  ${NN_SRC_DIR}/nn.cpp
  ${NN_SRC_DIR}/edgeapp_core.cpp
)

target_include_directories(nn PRIVATE
  ${NN_INC_DIR}
  ${ROOT_DIR}/include
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/depend/
  ${LIBS_DIR}/third_party/wasi_nn
)

target_link_libraries(nn draw)
