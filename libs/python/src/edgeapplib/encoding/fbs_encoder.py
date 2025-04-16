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
from typing import Any

import flatbuffers
from edgeapplib.flatbuffers.classification.SmartCamera import ClassificationData
from edgeapplib.flatbuffers.classification.SmartCamera import ClassificationTop
from edgeapplib.flatbuffers.classification.SmartCamera import GeneralClassification
from edgeapplib.flatbuffers.detection.SmartCamera import BoundingBox2d
from edgeapplib.flatbuffers.detection.SmartCamera import GeneralObject
from edgeapplib.flatbuffers.detection.SmartCamera import ObjectDetectionData
from edgeapplib.flatbuffers.detection.SmartCamera import ObjectDetectionTop


class FBSEncoder:
    def __init__(self, encode_type: str) -> None:
        self.builder = flatbuffers.Builder(1024)
        self.encode_type = encode_type.lower()

    def encode(self, data_list: list) -> bytearray:
        if self.encode_type == "classification":
            return self._encode_classification(data_list)
        elif self.encode_type == "detection":
            return self._encode_detection(data_list)
        else:
            raise ValueError(
                "Unsupported encode type. Please use 'classification' or 'detection'."
            )

    def _encode_classification(self, classification_list: list) -> bytearray:
        self.builder = flatbuffers.Builder(1024)
        classification_list_offsets = []
        for cls in reversed(classification_list):
            GeneralClassification.Start(self.builder)
            GeneralClassification.AddClassId(self.builder, cls["class_id"])
            GeneralClassification.AddScore(self.builder, cls["score"])
            classification_offset = GeneralClassification.End(self.builder)
            classification_list_offsets.append(classification_offset)

        ClassificationData.StartClassificationListVector(
            self.builder, len(classification_list_offsets)
        )
        for offset in classification_list_offsets:
            self.builder.PrependUOffsetTRelative(offset)
        classification_list_vector = self.builder.EndVector()

        ClassificationData.Start(self.builder)
        ClassificationData.AddClassificationList(
            self.builder, classification_list_vector
        )
        perception_offset = ClassificationData.End(self.builder)

        ClassificationTop.Start(self.builder)
        ClassificationTop.AddPerception(self.builder, perception_offset)
        root_offset = ClassificationTop.End(self.builder)

        return self._finish(root_offset)

    def _encode_detection(self, detection_list: list) -> bytearray:
        self.builder = flatbuffers.Builder(1024)
        object_list_offsets = []
        for det in reversed(detection_list):
            BoundingBox2d.Start(self.builder)
            BoundingBox2d.AddTop(self.builder, det["bounding_box"]["top"])
            BoundingBox2d.AddLeft(self.builder, det["bounding_box"]["left"])
            BoundingBox2d.AddRight(self.builder, det["bounding_box"]["right"])
            BoundingBox2d.AddBottom(self.builder, det["bounding_box"]["bottom"])
            bbox2d_offset = BoundingBox2d.End(self.builder)

            GeneralObject.Start(self.builder)
            GeneralObject.AddClassId(self.builder, det["object_id"])
            GeneralObject.AddScore(self.builder, det["score"])

            GeneralObject.AddBoundingBoxType(self.builder, 1)
            GeneralObject.AddBoundingBox(self.builder, bbox2d_offset)

            object_offset = GeneralObject.End(self.builder)
            object_list_offsets.append(object_offset)

        ObjectDetectionData.StartObjectDetectionListVector(
            self.builder, len(object_list_offsets)
        )
        for offset in reversed(object_list_offsets):
            self.builder.PrependUOffsetTRelative(offset)
        object_list_vector = self.builder.EndVector()

        ObjectDetectionData.Start(self.builder)
        ObjectDetectionData.AddObjectDetectionList(self.builder, object_list_vector)
        detection_data_offset = ObjectDetectionData.End(self.builder)

        ObjectDetectionTop.Start(self.builder)
        ObjectDetectionTop.AddPerception(self.builder, detection_data_offset)
        root_offset = ObjectDetectionTop.End(self.builder)

        return self._finish(root_offset)

    def _finish(self, root_offset: Any) -> bytearray:
        self.builder.Finish(root_offset)
        return self.builder.Output()
