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

add_subdirectory(${SM_SRC_DIR}/dtdl_model ${CMAKE_BINARY_DIR}/src/dtdl_model)

set(COMMON_SRC
  ${SM_SRC_DIR}/sm_context.cpp
  ${SM_SRC_DIR}/sm_configurator.cpp
  ${MOCKS_DIR}/data_export/mock_data_export.cpp
  ${MOCKS_DIR}/event_functions/mock_sm.cpp
  ${LIBS_DIR}/third_party/base64.c/base64.c
)

macro(add_test_executable TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp ${COMMON_SRC} ${ARGN})
  target_link_libraries(${TEST_NAME} fixtures dtdl_model sm log GTest::gtest_main GTest::gmock_main)
  gtest_discover_tests(${TEST_NAME})
endmacro()

add_test_executable(test_dtdl_model)
add_test_executable(test_utils)
add_test_executable(test_res_info)
add_test_executable(test_req_info)
add_test_executable(test_common_settings)
add_test_executable(test_port_setting)
add_test_executable(test_port_settings)
add_test_executable(test_camera_image)
add_test_executable(test_auto_exposure)
add_test_executable(test_auto_exposure_metering)
add_test_executable(test_manual_exposure)
add_test_executable(test_frame_rate)
add_test_executable(test_auto_white_balance)
add_test_executable(test_manual_white_balance_preset)
add_test_executable(test_image_cropping)
add_test_executable(test_pq_settings)
add_test_executable(test_inference_settings)
add_test_executable(test_custom_settings)
add_test_executable(test_codec_settings)
add_test_executable(test_register_access)
add_test_executable(test_register_access_array)

target_compile_definitions(test_dtdl_model PUBLIC
  DTDL_OBJ_PATH="${CMAKE_CURRENT_SOURCE_DIR}/sample_implemented.json"
)
