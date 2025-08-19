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
req_id=$(uuidgen)

process_state=""
for arg in "$@"; do
    case $arg in
        --process_state=*)
            process_state="${arg#*=}"
            shift
            ;;
    esac
done

if [ "$1" = "d" ]; then
    configuration_json=""
else
    configuration_json=$(jq -n \
        --arg req_id "$req_id" \
        --argjson process_state "$process_state" \
    '{
        "req_info": {
            "req_id": ($req_id),
        },
        "common_settings": {
            "process_state": $process_state,
            "log_level": 3,
            "port_settings": {
                "metadata": {
                    "method": 0,
                    "storage_name": "",
                    "endpoint": "",
                    "path": "",
                    "enabled": true
                },
                "input_tensor": {
                    "method": 1,
                    "storage_name": "${project_id}",
                    "endpoint": "",
                    "path": "${device_id}/image/${current_timestamp}",
                    "enabled": true
                }
            }
        },
        "custom_settings": {
            "ai_models": {
                "detection": {
                    "ai_model_bundle_id": "${network_id_1}",
                    "parameters": {
                        "max_detections": 10,
                        "threshold": 0.3,
                        "input_width": 320,
                        "input_height": 320,
                        "bbox_order": "yxyx",
                        "bbox_normalization": true,
                        "class_score_order": "cls_score"
                    }
                }
            },
            "metadata_settings": {
                "format": 1
            }
        }
    }' | jq -c .)
fi

echo "Send Configuration:"
if [ -n "$configuration_json" ]; then
    echo "$configuration_json" | jq .
else
    echo "\"\""
fi

echo "$configuration_json" | nc localhost 8080
