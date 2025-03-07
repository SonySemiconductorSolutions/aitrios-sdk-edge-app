/****************************************************************************
 * Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/


// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_SEMANTICSEGMENTATION_SMARTCAMERA_H_
#define FLATBUFFERS_GENERATED_SEMANTICSEGMENTATION_SMARTCAMERA_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
                  FLATBUFFERS_VERSION_MINOR == 1 &&
                  FLATBUFFERS_VERSION_REVISION == 21,
              "Non-compatible flatbuffers version included");

namespace SmartCamera {

struct SemanticSegmentationData;
struct SemanticSegmentationDataBuilder;

struct SemanticSegmentationTop;
struct SemanticSegmentationTopBuilder;

struct SemanticSegmentationData FLATBUFFERS_FINAL_CLASS
    : private ::flatbuffers::Table {
    typedef SemanticSegmentationDataBuilder Builder;
    enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
        VT_HEIGHT = 4,
        VT_WIDTH = 6,
        VT_CLASS_ID_MAP = 8,
        VT_NUM_CLASS_ID = 10,
        VT_SCORE_MAP = 12
    };
    uint16_t height() const { return GetField<uint16_t>(VT_HEIGHT, 0); }
    uint16_t width() const { return GetField<uint16_t>(VT_WIDTH, 0); }
    const ::flatbuffers::Vector<uint16_t> *class_id_map() const {
        return GetPointer<const ::flatbuffers::Vector<uint16_t> *>(
            VT_CLASS_ID_MAP);
    }
    uint16_t num_class_id() const {
        return GetField<uint16_t>(VT_NUM_CLASS_ID, 0);
    }
    const ::flatbuffers::Vector<float> *score_map() const {
        return GetPointer<const ::flatbuffers::Vector<float> *>(VT_SCORE_MAP);
    }
    bool Verify(::flatbuffers::Verifier &verifier) const {
        return VerifyTableStart(verifier) &&
               VerifyField<uint16_t>(verifier, VT_HEIGHT, 2) &&
               VerifyField<uint16_t>(verifier, VT_WIDTH, 2) &&
               VerifyOffset(verifier, VT_CLASS_ID_MAP) &&
               verifier.VerifyVector(class_id_map()) &&
               VerifyField<uint16_t>(verifier, VT_NUM_CLASS_ID, 2) &&
               VerifyOffset(verifier, VT_SCORE_MAP) &&
               verifier.VerifyVector(score_map()) && verifier.EndTable();
    }
};

struct SemanticSegmentationDataBuilder {
    typedef SemanticSegmentationData Table;
    ::flatbuffers::FlatBufferBuilder &fbb_;
    ::flatbuffers::uoffset_t start_;
    void add_height(uint16_t height) {
        fbb_.AddElement<uint16_t>(SemanticSegmentationData::VT_HEIGHT, height,
                                  0);
    }
    void add_width(uint16_t width) {
        fbb_.AddElement<uint16_t>(SemanticSegmentationData::VT_WIDTH, width, 0);
    }
    void add_class_id_map(
        ::flatbuffers::Offset<::flatbuffers::Vector<uint16_t>> class_id_map) {
        fbb_.AddOffset(SemanticSegmentationData::VT_CLASS_ID_MAP, class_id_map);
    }
    void add_num_class_id(uint16_t num_class_id) {
        fbb_.AddElement<uint16_t>(SemanticSegmentationData::VT_NUM_CLASS_ID,
                                  num_class_id, 0);
    }
    void add_score_map(
        ::flatbuffers::Offset<::flatbuffers::Vector<float>> score_map) {
        fbb_.AddOffset(SemanticSegmentationData::VT_SCORE_MAP, score_map);
    }
    explicit SemanticSegmentationDataBuilder(
        ::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
        start_ = fbb_.StartTable();
    }
    ::flatbuffers::Offset<SemanticSegmentationData> Finish() {
        const auto end = fbb_.EndTable(start_);
        auto o = ::flatbuffers::Offset<SemanticSegmentationData>(end);
        return o;
    }
};

inline ::flatbuffers::Offset<SemanticSegmentationData>
CreateSemanticSegmentationData(
    ::flatbuffers::FlatBufferBuilder &_fbb, uint16_t height = 0,
    uint16_t width = 0,
    ::flatbuffers::Offset<::flatbuffers::Vector<uint16_t>> class_id_map = 0,
    uint16_t num_class_id = 0,
    ::flatbuffers::Offset<::flatbuffers::Vector<float>> score_map = 0) {
    SemanticSegmentationDataBuilder builder_(_fbb);
    builder_.add_score_map(score_map);
    builder_.add_class_id_map(class_id_map);
    builder_.add_num_class_id(num_class_id);
    builder_.add_width(width);
    builder_.add_height(height);
    return builder_.Finish();
}

inline ::flatbuffers::Offset<SemanticSegmentationData>
CreateSemanticSegmentationDataDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb, uint16_t height = 0,
    uint16_t width = 0, const std::vector<uint16_t> *class_id_map = nullptr,
    uint16_t num_class_id = 0, const std::vector<float> *score_map = nullptr) {
    auto class_id_map__ =
        class_id_map ? _fbb.CreateVector<uint16_t>(*class_id_map) : 0;
    auto score_map__ = score_map ? _fbb.CreateVector<float>(*score_map) : 0;
    return SmartCamera::CreateSemanticSegmentationData(
        _fbb, height, width, class_id_map__, num_class_id, score_map__);
}

struct SemanticSegmentationTop FLATBUFFERS_FINAL_CLASS
    : private ::flatbuffers::Table {
    typedef SemanticSegmentationTopBuilder Builder;
    enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
        VT_PERCEPTION = 4
    };
    const SmartCamera::SemanticSegmentationData *perception() const {
        return GetPointer<const SmartCamera::SemanticSegmentationData *>(
            VT_PERCEPTION);
    }
    bool Verify(::flatbuffers::Verifier &verifier) const {
        return VerifyTableStart(verifier) &&
               VerifyOffset(verifier, VT_PERCEPTION) &&
               verifier.VerifyTable(perception()) && verifier.EndTable();
    }
};

struct SemanticSegmentationTopBuilder {
    typedef SemanticSegmentationTop Table;
    ::flatbuffers::FlatBufferBuilder &fbb_;
    ::flatbuffers::uoffset_t start_;
    void add_perception(
        ::flatbuffers::Offset<SmartCamera::SemanticSegmentationData>
            perception) {
        fbb_.AddOffset(SemanticSegmentationTop::VT_PERCEPTION, perception);
    }
    explicit SemanticSegmentationTopBuilder(
        ::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
        start_ = fbb_.StartTable();
    }
    ::flatbuffers::Offset<SemanticSegmentationTop> Finish() {
        const auto end = fbb_.EndTable(start_);
        auto o = ::flatbuffers::Offset<SemanticSegmentationTop>(end);
        return o;
    }
};

inline ::flatbuffers::Offset<SemanticSegmentationTop>
CreateSemanticSegmentationTop(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<SmartCamera::SemanticSegmentationData> perception =
        0) {
    SemanticSegmentationTopBuilder builder_(_fbb);
    builder_.add_perception(perception);
    return builder_.Finish();
}

inline const SmartCamera::SemanticSegmentationTop *GetSemanticSegmentationTop(
    const void *buf) {
    return ::flatbuffers::GetRoot<SmartCamera::SemanticSegmentationTop>(buf);
}

inline const SmartCamera::SemanticSegmentationTop *
GetSizePrefixedSemanticSegmentationTop(const void *buf) {
    return ::flatbuffers::GetSizePrefixedRoot<
        SmartCamera::SemanticSegmentationTop>(buf);
}

inline bool VerifySemanticSegmentationTopBuffer(
    ::flatbuffers::Verifier &verifier) {
    return verifier.VerifyBuffer<SmartCamera::SemanticSegmentationTop>(nullptr);
}

inline bool VerifySizePrefixedSemanticSegmentationTopBuffer(
    ::flatbuffers::Verifier &verifier) {
    return verifier
        .VerifySizePrefixedBuffer<SmartCamera::SemanticSegmentationTop>(
            nullptr);
}

inline void FinishSemanticSegmentationTopBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<SmartCamera::SemanticSegmentationTop> root) {
    fbb.Finish(root);
}

inline void FinishSizePrefixedSemanticSegmentationTopBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<SmartCamera::SemanticSegmentationTop> root) {
    fbb.FinishSizePrefixed(root);
}

}  // namespace SmartCamera

#endif  // FLATBUFFERS_GENERATED_SEMANTICSEGMENTATION_SMARTCAMERA_H_
