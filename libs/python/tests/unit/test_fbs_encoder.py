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
from edgeapplib.encoding.fbs_encoder import FBSEncoder


def test_encode_classification():
    task = "classification"
    fbs_encoder = FBSEncoder(task)
    inference_results = [{"class_id": 1, "score": 0.5}, {"class_id": 2, "score": 0.8}]
    flatbuffers: bytearray = fbs_encoder.encode(inference_results)
    assert flatbuffers


def test_encode_detection():
    task = "detection"
    fbs_encoder = FBSEncoder(task)
    inference_results = [
        {
            "object_id": 0,
            "score": 0.68,
            "bbox": {"left": 177, "top": 76, "right": 581, "bottom": 479},
        }
    ]
    flatbuffers: bytearray = fbs_encoder.encode(inference_results)
    assert flatbuffers


def test_encode_unsupported():
    with pytest.raises(
        ValueError,
        match="Unsupported encode type. Please use 'classification' or 'detection'.",
    ):
        task = "other"
        fbs_encoder = FBSEncoder(task)
        inference_results = [
            {"class_id": 1, "score": 0.5},
            {"class_id": 2, "score": 0.8},
        ]
        fbs_encoder.encode(inference_results)
