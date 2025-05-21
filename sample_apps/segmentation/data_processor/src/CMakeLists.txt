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

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)
set(MOCKS_DIR ${LIBS_DIR}/tests/mocks)

add_library(data_processor_api
  ${SAMPLE_APP_DIR}/segmentation/data_processor/src/segmentation_data_processor.cpp
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
  ${SAMPLE_APP_DIR}/segmentation/data_processor/src/segmentation_utils.cpp
  ${SAMPLE_APP_DIR}/utils/src/data_processor_utils.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
)

target_include_directories(data_processor_api PUBLIC
  ${SAMPLE_APP_DIR}/utils/include
  ${SAMPLE_APP_DIR}/segmentation/include/schemas
  ${SAMPLE_APP_DIR}/segmentation/data_processor/src
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/third_party/flatbuffers/include
  ${MOCKS_DIR}/sensor
  ${MOCKS_DIR}/device
)

target_link_libraries(data_processor_api
  edge-app-lib-log)
