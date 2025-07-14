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

from abc import ABC, abstractmethod

from ._edge_app_sdk import (
    EdgeAppError,
    SensorStream,
    SensorFrame,
    SensorChannel,
    SensorAiModelBundleId,
    SensorImageCrop,
    stream,
    run_sm,
    send_metadata,
    send_input_tensor,
    send_state,
    get_port_settings,
    DataExportResult,
    ResponseCode,
    get_workspace_directory,
    format_timestamp,
    DataExportDataType,
    get_file_suffix,
    send_file,
    send_telemetry,
    SendDataType,
)

from .data_export import send_image
from .data_export import send_inference
from .utils import log
from .utils import get_configure_error_json
from .utils import set_edge_app_lib_network
from .enums import DataProcessorResultCode


class EdgeApp(ABC):
    def on_create(self) -> int:
        log("[Python] on_create")
        return 0

    def on_configure(self, topic: str, config_str: str) -> int:
        log("[Python] on_configure")
        return 0

    @abstractmethod
    def on_iterate(self) -> int:
        ...

    def on_stop(self) -> int:
        log("[Python] on_stop")
        return 0

    def on_start(self) -> int:
        log("[Python] on_start")
        return 0

    def on_destroy(self) -> int:
        log("[Python] on_destroy")
        return 0


__all__ = [
    "EdgeApp",
    "EdgeAppError",
    "SensorStream",
    "SensorFrame",
    "SensorChannel",
    "SensorAiModelBundleId",
    "SensorImageCrop",
    "stream",
    "run_sm",
    "send_metadata",
    "send_input_tensor",
    "get_port_settings",
    "send_image",
    "send_inference",
    "send_state",
    "log",
    "DataExportResult",
    "ResponseCode",
    "get_configure_error_json",
    "set_edge_app_lib_network",
    "DataProcessorResultCode",
    "SendDataType",
]
