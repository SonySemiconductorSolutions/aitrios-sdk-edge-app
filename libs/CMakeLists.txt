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

include(config_common)

add_subdirectory(common/src)
add_subdirectory(log/src)
add_subdirectory(draw/src)
add_subdirectory(sensor/src)
add_subdirectory(send_data/src/data_export/src)
add_subdirectory(send_data/src/)
add_subdirectory(receive_data/src/)
add_subdirectory(sm/src)
add_subdirectory(nn/src)

# circular dependency sensor <-> sm
target_link_libraries(sensor sm)

if(MOCK EQUAL 1)
  add_library(mock1 STATIC
    ${MOCKS_DIR}/sensor/mock_sensor.cpp
  )
  # this is PUBLIC only because it's moved from test/edge_app.
  # i haven't checked if it's really nececssary or not.
  target_include_directories(mock1 PUBLIC
    ${SAMPLE_APP_DIR}/utils/include
    ${LIBS_DIR}/third_party/flatbuffers/include
    ${LIBS_DIR}/third_party/wasi_nn
    ${ROOT_DIR}/include
    ${MOCKS_DIR}
    ${MOCKS_DIR}/sensor
    ${MOCKS_DIR}/device
    ${LIBS_DIR}/sensor/src/
    ${LIBS_DIR}/send_data/src/data_export/include
  )
  target_include_directories(mock1 PRIVATE
    ${LIBS_DIR}/sm/include
    ${LIBS_DIR}/depend/edge_app
    ${LIBS_DIR}/third_party/parson
  )
  target_link_libraries(mock1 PRIVATE common)
endif()

if(MOCK EQUAL 1)
  add_library(mock2 STATIC
    ${MOCKS_DIR}/sensor/testing_utils.cpp
    ${MOCKS_DIR}/device/mock_device.cpp
    ${LIBS_DIR}/log/src/log_common.cpp
    ${MOCKS_DIR}/log/mock_log.cpp
    ${MOCKS_DIR}/nn/mock_wasi_nn.c
  )
  # this is PUBLIC only because it's moved from test/edge_app.
  # i haven't checked if it's really nececssary or not.
  target_include_directories(mock2 PUBLIC
    ${SAMPLE_APP_DIR}/utils/include
    ${LIBS_DIR}/third_party/flatbuffers/include
    ${LIBS_DIR}/third_party/wasi_nn
    ${ROOT_DIR}/include
    ${MOCKS_DIR}
    ${MOCKS_DIR}/sensor
    ${MOCKS_DIR}/device
    ${LIBS_DIR}/sensor/src/
    ${LIBS_DIR}/send_data/src/data_export/include
  )
  target_include_directories(mock2 PRIVATE
    ${LIBS_DIR}/sm/include
    ${LIBS_DIR}/depend/edge_app
    ${LIBS_DIR}/third_party/parson
    ${LIBS_DIR}/log/include
  )
  target_link_libraries(mock2 PRIVATE common)
endif()

if(EVP_MOCK EQUAL 1)
  add_library(evp_mock STATIC
    ${MOCKS_DIR}/evp/simple_evp.c
    ${MOCKS_DIR}/evp/pytest_support.c
  )
  target_include_directories(evp_mock PRIVATE
    ${ROOT_DIR}/include
    ${LIBS_DIR}/depend/edge_app
  )
  target_compile_options(evp_mock PUBLIC -pthread)
  target_compile_definitions(evp_mock PRIVATE HAVE_CHECK_ACK_FILE)
  target_link_libraries(evp_mock PRIVATE common)
endif()

add_library(edge_app_lib INTERFACE)

target_link_libraries(edge_app_lib INTERFACE
  $<$<BOOL:${MOCK}>:mock1>
  common
  sm
  data_export
  send_data
  nn
  $<$<BOOL:${MOCK}>:mock2>
  log
  draw
  receive_data
  $<$<BOOL:${EVP_MOCK}>:evp_mock>
)
