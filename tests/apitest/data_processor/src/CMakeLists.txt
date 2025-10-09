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

add_library(data_processor_api
  ${TESTS_DIR}/apitest/data_processor/src/apitest_data_processor.cpp
  ${TESTS_DIR}/apitest/data_processor/src/apitest_util.cpp
  ${TESTS_DIR}/apitest/data_processor/src/apitest_sensor.cpp
  ${TESTS_DIR}/apitest/data_processor/src/dcpu_param_parser.cpp
  ${SAMPLE_APP_DIR}/utils/src/sm_utils.cpp
  ${SAMPLE_APP_DIR}/utils/src/data_processor_utils.cpp
  ${LIBS_DIR}/third_party/parson/parson.c
  ${LIBS_DIR}/third_party/base64.c/base64.c
)

target_include_directories(data_processor_api PUBLIC
  ${SAMPLE_APP_DIR}/utils/include
  ${TESTS_DIR}/apitest/data_processor/src
  ${INCLUDE_DIR}
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/third_party/base64.c
  ${LIBS_DIR}/depend/edge_app
)

# for MOCK_APITEST
target_include_directories(data_processor_api PRIVATE
  ${LIBS_DIR}/tests/mocks
)

target_link_libraries(data_processor_api log)
