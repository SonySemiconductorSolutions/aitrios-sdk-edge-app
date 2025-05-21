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

from ._edge_app_sdk import (
    AI_MODEL_BUNDLE_ID_SIZE,
    EdgeAppLibSensorStatusParam,
    get_last_error_string,
    ResponseCode,
    SensorStream,
    SensorAiModelBundleId,
)
import json

def log(msg: str):
    print(msg, flush=True)

def get_configure_error_json(code: ResponseCode, msg: str, res_id: str) -> str:
    return json.dumps({
        "code": code,
        "msg": msg,
        "res_id": res_id
    })

def set_edge_app_lib_network(stream: SensorStream, config: dict) -> int:
    ai_model_bundle_id = config.get("ai_model_bundle_id")
    if not ai_model_bundle_id:
        log("AI model bundle ID is not available")
        return -1

    if len(ai_model_bundle_id) >= AI_MODEL_BUNDLE_ID_SIZE:
        log("AI model bundle ID is too long")
        return -1

    try:
        bundle_id = SensorAiModelBundleId()
        bundle_id.id = ai_model_bundle_id

        stream.ai_model_bundle_id = bundle_id
        log(f"Successfully set ai bundle id {ai_model_bundle_id}")
        return 0
    except Exception as e:
        log(f"Error setting AI model bundle ID: {e}")
        print_sensor_error()
        return -1

def print_sensor_error() -> None:
    try:
        error_msg = get_last_error_string(
            EdgeAppLibSensorStatusParam.AITRIOS_SENSOR_STATUS_PARAM_MESSAGE
        )
        log(f"Sensor error: {error_msg}")
    except Exception as e:
        log(f"Failed to get sensor error message: {e}")
