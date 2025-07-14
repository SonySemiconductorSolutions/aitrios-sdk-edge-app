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

import os
from enum import IntEnum
from typing import Optional, Dict, Any

import cv2
import json
import numpy as np
import base64

from edge_app_sdk import get_port_settings
from edge_app_sdk import get_workspace_directory
from edge_app_sdk import format_timestamp
from edge_app_sdk import get_file_suffix
from edge_app_sdk import DataExportDataType
from edge_app_sdk import DataExportResult
from edge_app_sdk import SendDataType
from edge_app_sdk import send_file
from edge_app_sdk import send_telemetry
from edge_app_sdk.utils import log


class METHOD(IntEnum):
    MQTT = 0
    BLOB_STORAGE = 1
    HTTP_STORAGE = 2


class DataExporter:
    def __init__(self):
        self.device_id = "Aid-XXXXXXXX-0000-2000-9002-000000000000"
        self.model_id = "0311030000000100"

    def _get_port_setting(self, key: str) -> Optional[Dict[str, Any]]:
        try:
            data = json.loads(get_port_settings())
        except json.JSONDecodeError as e:
            log(f"[Python] Failed to parse port settings JSON: {e}")
            return None

        port_setting = data.get(key)
        if not port_setting:
            log(f"[Python] No '{key}' found in port_settings")
        return port_setting

    def _validate_image_input(self, image: np.ndarray, timestamp: int) -> bool:
        if not isinstance(image, np.ndarray):
            log("[Python] Invalid image input: must be a numpy array")
            return False

        if not isinstance(timestamp, int) or timestamp < 0:
            log("[Python] Invalid timestamp: must be a non-negative integer")
            return False

        return True

    def _validate_inference_input(self, inferences: list, timestamp: int) -> bool:
        if not isinstance(inferences, list):
            log("[Python] Invalid inferences: must be a list")
            return False

        if not isinstance(timestamp, int) or timestamp < 0:
            log("[Python] Invalid timestamp: must be a non-negative integer")
            return False

        return True

    def _validate_method(self, method: int, allowed_methods: tuple) -> bool:
        if not method or method not in allowed_methods:
            log(f"[Python] METHOD {method} not implemented")
            return False
        return True

    def _encode_image(
        self, image: np.ndarray, filename_extension: str
    ) -> Optional[bytes]:
        try:
            if filename_extension == ".jpg":
                image_bgr = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
                success, encoded_image = cv2.imencode('.jpg', image_bgr)
                if not success:
                    log("[Python] Failed to encode image to JPG")
                    return None
                return encoded_image.tobytes()
            elif filename_extension == ".bin":
                return image.tobytes()
            else:
                log(f"[Python] Unsupported file extension: {filename_extension}")
                return None
        except Exception as e:
            log(f"[Python] Failed to encode image: {e}")
            return None

    def _prepare_file_path(
        self, timestamp: int, data_type: DataExportDataType
    ) -> tuple[Optional[str], Optional[str], Optional[str]]:
        workspace = get_workspace_directory()
        if not workspace:
            log("[Python] Failed to get workspace directory")
            return None, None, None

        filename = format_timestamp(timestamp)
        filename_extension = get_file_suffix(data_type)
        local_file_path = f"{workspace}/{filename}{filename_extension}"

        return filename, filename_extension, local_file_path

    def _write_and_send_file(
        self,
        data: Any,
        data_type: DataExportDataType,
        local_file_path: str,
        url: str,
        timeout_ms: int,
        is_binary: bool = False
    ) -> bool:
        try:
            mode = "wb" if is_binary else "w"
            with open(local_file_path, mode) as f:
                if is_binary:
                    f.write(data)
                else:
                    json.dump(data, f)

            result = send_file(data_type, local_file_path, url, timeout_ms)
            if result != DataExportResult.SUCCESS:
                log(f"[Python] Failed to send file to {url}")
                return False

            log(f"[Python] Successfully sent data to {url}")
            return True
        except Exception as e:
            log(f"[Python] Failed to upload data: {e}")
            return False

        # Note: The file is deleted in send_file() by a background thread.
        # Should not delete the file here.

    def _prepare_url(self, endpoint: str, path: str, filename: str, extension: str) -> str:
        endpoint = endpoint.rstrip("/")
        path = path.lstrip("/")
        return f"{endpoint}/{path}/{filename}{extension}"

    def _create_inference_payload(
        self,
        inferences: list,
        metadata_format: SendDataType,
        timestamp: int,
        image_enabled: bool
    ) -> dict:
        if metadata_format == SendDataType.Base64:
            o_value = base64.b64encode(
                json.dumps(inferences).encode('utf-8')).decode('utf-8')
            f_value = 0
        else:
            o_value = inferences
            f_value = 1

        return {
            "DeviceID": self.device_id,
            "ModelID": self.model_id,
            "Image": image_enabled,
            "Inferences": [
                {
                    "T": format_timestamp(timestamp),
                    "O": o_value,
                    "F": f_value
                }
            ],
        }

    def send_image(self, image: np.ndarray, timestamp: int, timeout_ms: int) -> bool:
        port_setting = self._get_port_setting("input_tensor")
        if not port_setting or not port_setting.get("enabled"):
            log("[Python] Input tensor is not enabled")
            return False

        if not self._validate_image_input(image, timestamp):
            return False

        if not self._validate_method(
            port_setting.get("method"),
            (METHOD.HTTP_STORAGE, METHOD.BLOB_STORAGE)
        ):
            return False

        filename, filename_extension, local_file_path = self._prepare_file_path(
            timestamp, DataExportDataType.Raw
        )
        if not all([filename, filename_extension, local_file_path]):
            return False

        url = self._prepare_url(
            port_setting.get("endpoint", ""),
            port_setting.get("path", ""),
            filename,
            filename_extension
        )

        encoded_image = self._encode_image(image, filename_extension)
        if encoded_image is None:
            return False

        return self._write_and_send_file(
            encoded_image,
            DataExportDataType.Raw,
            local_file_path,
            url,
            timeout_ms,
            is_binary=True
        )

    def send_inference(
        self,
        inferences: list,
        metadata_format: SendDataType,
        timestamp: int,
        timeout_ms: int
    ) -> bool:
        port_setting = self._get_port_setting("metadata")
        if not port_setting or not port_setting.get("enabled"):
            log("[Python] Metadata is not enabled")
            return False

        if not self._validate_inference_input(inferences, timestamp):
            return False

        send_method = port_setting.get("method")
        image_enabled = False
        input_tensor_setting = self._get_port_setting("input_tensor")
        if input_tensor_setting and input_tensor_setting.get("enabled"):
            image_enabled = True

        payload = self._create_inference_payload(
            inferences,
            metadata_format,
            timestamp,
            image_enabled
        )

        if send_method in (METHOD.HTTP_STORAGE, METHOD.BLOB_STORAGE):
            filename, filename_extension, local_file_path = self._prepare_file_path(
                timestamp, DataExportDataType.Metadata
            )
            if not all([filename, filename_extension, local_file_path]):
                return False

            url = self._prepare_url(
                port_setting.get("endpoint", ""),
                port_setting.get("path", ""),
                filename,
                filename_extension
            )

            return self._write_and_send_file(
                payload,
                DataExportDataType.Metadata,
                local_file_path,
                url,
                timeout_ms
            )

        elif send_method == METHOD.MQTT:
            json_str = json.dumps(payload)
            json_bytes = json_str.encode('utf-8')
            result = send_telemetry(json_bytes, timeout_ms)
            if result != DataExportResult.SUCCESS:
                log("[Python] Failed to send telemetry")
                return False

            log("[Python] Successfully sent telemetry")
            return True
        else:
            log(f"[Python] Unsupported send method: {send_method}")
            return False


# Create a singleton instance
_data_exporter = DataExporter()

# Export the functions that use the singleton instance
def send_image(image: np.ndarray, timestamp: int, timeout_ms: int = 10000) -> bool:
    return _data_exporter.send_image(image, timestamp, timeout_ms)

def send_inference(
    inferences: list,
    metadata_format: SendDataType,
    timestamp: int,
    timeout_ms: int = 10000
) -> bool:
    return _data_exporter.send_inference(
        inferences, metadata_format, timestamp, timeout_ms)
