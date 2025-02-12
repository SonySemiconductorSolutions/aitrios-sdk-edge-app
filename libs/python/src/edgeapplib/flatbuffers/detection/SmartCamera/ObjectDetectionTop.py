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
# automatically generated by the FlatBuffers compiler, do not modify
# namespace: SmartCamera
import flatbuffers
from flatbuffers.compat import import_numpy

np = import_numpy()


class ObjectDetectionTop:
    __slots__ = ["_tab"]

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = ObjectDetectionTop()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsObjectDetectionTop(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)

    # ObjectDetectionTop
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # ObjectDetectionTop
    def Perception(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            x = self._tab.Indirect(o + self._tab.Pos)
            from SmartCamera.ObjectDetectionData import ObjectDetectionData

            obj = ObjectDetectionData()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None


def ObjectDetectionTopStart(builder):
    builder.StartObject(1)


def Start(builder):
    ObjectDetectionTopStart(builder)


def ObjectDetectionTopAddPerception(builder, perception):
    builder.PrependUOffsetTRelativeSlot(
        0, flatbuffers.number_types.UOffsetTFlags.py_type(perception), 0
    )


def AddPerception(builder, perception):
    ObjectDetectionTopAddPerception(builder, perception)


def ObjectDetectionTopEnd(builder):
    return builder.EndObject()


def End(builder):
    return ObjectDetectionTopEnd(builder)
