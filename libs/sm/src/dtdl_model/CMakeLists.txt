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

set(DTDL_MODEL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBS_DIR ${ROOT_DIR}/libs)
set(SM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

set(PQ_SETTINGS_FILES
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/camera_image_size.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/camera_image_flip.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/frame_rate.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/auto_white_balance.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/manual_white_balance_preset.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/image_cropping.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/auto_exposure.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/auto_exposure_metering.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/manual_exposure.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/register_access.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/pq_settings/register_access_array.cpp
)

add_library(edge-app-lib-dtdl-model
  ${DTDL_MODEL_ROOT_DIR}/dtdl_model.cpp
  ${DTDL_MODEL_ROOT_DIR}/utils.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/custom_settings.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/req_info.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/res_info.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/json_object.cpp
  ${PQ_SETTINGS_FILES}
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/port_setting.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/port_settings.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/inference_settings.cpp
  ${DTDL_MODEL_ROOT_DIR}/objects/common_settings/codec_settings.cpp
  # TODO: properly link with parson
  ${LIBS_DIR}/third_party/parson/parson.c
  ${SM_SRC_DIR}/sm_context.cpp
  ${SM_SRC_DIR}/sm_configurator.cpp
  ${SM_SRC_DIR}/states/state_utils.cpp
)

target_include_directories(edge-app-lib-dtdl-model PRIVATE
  ${DTDL_MODEL_ROOT_DIR}
  ${DTDL_MODEL_ROOT_DIR}/..
  ${SM_SRC_DIR}/states
  # TODO: properly define include directories for logger and parson
  ${ROOT_DIR}/include/edgeapp
  ${LIBS_DIR}/common/include
  ${LIBS_DIR}/third_party/parson
  ${LIBS_DIR}/depend/edge_app
  ${LIBS_DIR}/log/include
)

target_link_libraries(edge-app-lib-dtdl-model
  edge-app-lib-common
  edge-app-lib-sensor
  edge-app-lib-log)
