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
from dataclasses import dataclass


class ConvertibleMixIn:

    @classmethod
    def convert_all(cls, objs: list["Detection"]):
        return [cls.convert(r) for r in objs]


@dataclass
class Classification(ConvertibleMixIn):
    idx: int
    score: float

    @staticmethod
    def convert(obj: "Classification"):
        # Note:
        # Aitrios edge-app FBSEncoder expects a list of dictionary with the
        # following data.
        return dict(
            class_id=int(obj.idx),
            score=round(float(obj.score), 3),
        )


@dataclass
class Detection(ConvertibleMixIn):
    box: tuple
    category: int
    conf: float

    @staticmethod
    def convert(obj: "Detection"):
        # Note:
        # Aitrios edge-app FBSEncoder expects a list of dictionary with the
        # following data.
        x, y, w, h = obj.box
        return dict(
            object_id=int(obj.category),
            score=round(float(obj.conf), 3),
            bbox=dict(
                left=x,
                top=y,
                right=x + w,
                bottom=y + h,
            ),
        )
