#!/bin/bash
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

if [ -d "./metadata" ]; then
  rm -rf "./metadata"
fi
if [ -d "./images" ]; then
  rm -rf "./images"
fi

/home/pi/senscord_libcamera_package/share/senscord/setup_env.sh \
python ../example/privacy_mask_edge_app.py
