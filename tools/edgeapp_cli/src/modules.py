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

"""Module definition for edge applications"""
from uuid import UUID
from dataclasses import dataclass
from typing import Optional


@dataclass
class Module:
    """Edge application module definition"""
    name: str
    hash: str
    host: str
    type: str = "wasm"
    entry_point: str = "main"
    id: Optional[UUID] = None
    instance: Optional[UUID] = None
