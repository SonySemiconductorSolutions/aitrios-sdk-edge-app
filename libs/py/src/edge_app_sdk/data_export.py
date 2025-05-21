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

import cv2
import json
import numpy as np
import requests
from enum import IntEnum

from edge_app_sdk import get_port_settings
from edge_app_sdk.utils import log


class METHOD(IntEnum):
    MQTT = 0
    BLOB_STORAGE = 1
    HTTP_STORAGE = 2


def _get_port_setting(key: str):
    try:
        data = json.loads(get_port_settings())
    except json.JSONDecodeError as e:
        log(f"[Python] Failed to parse port settings JSON: {e}")
        return None

    port_setting = data.get(key)
    if not port_setting:
        log(f"[Python] No '{key}' found in port_settings")
    return port_setting


def send_image(image: np.ndarray, timestamp: str, filename: str) -> None:
    port_setting = _get_port_setting("input_tensor")
    if not port_setting:
        return

    send_method = port_setting["method"]
    if send_method != METHOD.HTTP_STORAGE:
        log(f"[Python] METHOD {send_method} not implemented for send_image")
        return

    endpoint = port_setting.get("endpoint", "").rstrip("/")
    path = port_setting.get("path", "").lstrip("/")
    filename_extension = ".jpg"
    url = f"{endpoint}/{path}/{filename}{filename_extension}"

    image_bgr = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
    success, encoded_image = cv2.imencode('.jpg', image_bgr)
    if not success:
        log("[Python] Failed to encode image to JPG")
        return

    try:
        response = requests.post(
            url,
            data=encoded_image.tobytes(),
            headers={"Content-Type": "image/jpeg"},
            timeout=5
        )
        if response.status_code == 200:
            log(f"[Python] Uploaded {timestamp}.jpg successfully")
        else:
            log(f"[Python] Upload failed: {response.status_code}")
    except Exception as e:
        log(f"[Python] Failed to upload image: {e}")


def send_inference(inferences: list, timestamp: str, filename: str) -> None:
    port_setting = _get_port_setting("metadata")
    if not port_setting:
        return

    send_method = port_setting["method"]

    if send_method != METHOD.HTTP_STORAGE:
        log(f"[Python] METHOD {send_method} not implemented")
        return

    endpoint = port_setting.get("endpoint", "").rstrip("/")
    path = port_setting.get("path", "").lstrip("/")
    filename_extension = ".txt"
    url = f"{endpoint}/{path}/{filename}{filename_extension}"

    image_enabled = False
    port_setting = _get_port_setting("input_tensor")
    if port_setting and port_setting.get("enabled"):
        image_enabled = True

    payload = {
        "DeviceID":"Aid-XXXXXXXX-0000-2000-9002-000000000000",
        "ModelID":"0311030000000100",
        "Image": image_enabled,
        "Inferences": [{"T": timestamp, "O": inferences, "F": 1}],
    }

    try:
        response = requests.post(url, json=payload, timeout=5)
        if response.status_code == 200:
            log("[Python] Metadata uploaded successfully")
        else:
            log(f"[Python] Metadata upload failed: {response.status_code}")
    except Exception as e:
        log(f"[Python] Error uploading metadata: {e}")
