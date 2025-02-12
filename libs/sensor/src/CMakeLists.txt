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
set(AITRIOS_SENSOR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(AITRIOS_SENSOR_SRC_DIR ${AITRIOS_SENSOR_ROOT_DIR}/src)

add_library(sensor STATIC
  ${AITRIOS_SENSOR_SRC_DIR}/sensor.cpp
  ${AITRIOS_SENSOR_SRC_DIR}/sensor_wrapper.cpp
  ${AITRIOS_SENSOR_SRC_DIR}/sensor_wrapper_error.cpp
)

target_include_directories(sensor PUBLIC
  ${ROOT_DIR}/include
  ${LIBS_DIR}/depend/
  ${LIBS_DIR}/sm/include
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/send_data/src/process_format
)
