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

import pytest
from edge_app_sdk import (
    SensorImageCrop,
    SensorAiModelBundleId,
)


def test_image_crop_type():
    c = SensorImageCrop()

    assert c.left == 0
    assert c.top == 0
    assert c.width == 0
    assert c.height == 0

    c.left = 10
    c.top = 20
    c.width = 30
    c.height = 40

    assert c.left == 10
    assert c.top == 20
    assert c.width == 30
    assert c.height == 40


def test_ai_model_bundle_id_type():
    b = SensorAiModelBundleId()
    assert b.id == ""

    b.id = "test"
    assert b.id == "test"

    b.id = "a" * 127
    assert b.id == ("a" * 127)

    with pytest.raises(ValueError):  # id string too long
        b.id = "a" * 128

    assert b.id == ("a" * 127)
