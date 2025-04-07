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
)

from abc import ABC, abstractmethod


class EdgeApp(ABC):
    @abstractmethod
    def on_create(self) -> int:
        ...

    @abstractmethod
    def on_configure(self) -> int:
        ...

    @abstractmethod
    def on_iterate(self) -> int:
        ...

    @abstractmethod
    def on_stop(self) -> int:
        ...

    @abstractmethod
    def on_start(self) -> int:
        ...

    @abstractmethod
    def on_destroy(self) -> int:
        ...


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
]
