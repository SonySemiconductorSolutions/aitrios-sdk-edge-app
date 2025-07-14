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

add_library(edge_app_lib INTERFACE)

target_link_libraries(edge_app_lib INTERFACE common sm data_export send_data log draw nn receive_data)
