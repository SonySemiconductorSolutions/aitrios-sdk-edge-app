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


class ClassificationData:
    __slots__ = ["_tab"]

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = ClassificationData()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsClassificationData(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)

    # ClassificationData
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # ClassificationData
    def ClassificationList(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from SmartCamera.GeneralClassification import GeneralClassification

            obj = GeneralClassification()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # ClassificationData
    def ClassificationListLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # ClassificationData
    def ClassificationListIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        return o == 0


def ClassificationDataStart(builder):
    builder.StartObject(1)


def Start(builder):
    ClassificationDataStart(builder)


def ClassificationDataAddClassificationList(builder, classificationList):
    builder.PrependUOffsetTRelativeSlot(
        0, flatbuffers.number_types.UOffsetTFlags.py_type(classificationList), 0
    )


def AddClassificationList(builder, classificationList):
    ClassificationDataAddClassificationList(builder, classificationList)


def ClassificationDataStartClassificationListVector(builder, numElems):
    return builder.StartVector(4, numElems, 4)


def StartClassificationListVector(builder, numElems):
    return ClassificationDataStartClassificationListVector(builder, numElems)


def ClassificationDataEnd(builder):
    return builder.EndObject()


def End(builder):
    return ClassificationDataEnd(builder)
