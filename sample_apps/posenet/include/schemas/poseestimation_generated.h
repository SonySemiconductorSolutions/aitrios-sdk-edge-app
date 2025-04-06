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


#ifndef FLATBUFFERS_GENERATED_POSEESTIMATION_SMARTCAMERA_H_
#define FLATBUFFERS_GENERATED_POSEESTIMATION_SMARTCAMERA_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
              FLATBUFFERS_VERSION_MINOR == 1 &&
              FLATBUFFERS_VERSION_REVISION == 21,
             "Non-compatible flatbuffers version included");

namespace SmartCamera {

struct Point2d;
struct Point2dBuilder;

struct KeyPoint;
struct KeyPointBuilder;

struct GeneralPose;
struct GeneralPoseBuilder;

struct PoseEstimationData;
struct PoseEstimationDataBuilder;

struct PoseEstimationTop;
struct PoseEstimationTopBuilder;

enum KeyPointName : int8_t {
  KeyPointName_Nose = 0,
  KeyPointName_LeftEye = 1,
  KeyPointName_RightEye = 2,
  KeyPointName_LeftEar = 3,
  KeyPointName_RightEar = 4,
  KeyPointName_LeftShoulder = 5,
  KeyPointName_RightShoulder = 6,
  KeyPointName_LeftElbow = 7,
  KeyPointName_RightElbow = 8,
  KeyPointName_LeftWrist = 9,
  KeyPointName_RightWrist = 10,
  KeyPointName_LeftHip = 11,
  KeyPointName_RightHip = 12,
  KeyPointName_LeftKnee = 13,
  KeyPointName_RightKnee = 14,
  KeyPointName_LeftAnkle = 15,
  KeyPointName_RightAnkle = 16,
  KeyPointName_MIN = KeyPointName_Nose,
  KeyPointName_MAX = KeyPointName_RightAnkle
};

inline const KeyPointName (&EnumValuesKeyPointName())[17] {
  static const KeyPointName values[] = {
    KeyPointName_Nose,
    KeyPointName_LeftEye,
    KeyPointName_RightEye,
    KeyPointName_LeftEar,
    KeyPointName_RightEar,
    KeyPointName_LeftShoulder,
    KeyPointName_RightShoulder,
    KeyPointName_LeftElbow,
    KeyPointName_RightElbow,
    KeyPointName_LeftWrist,
    KeyPointName_RightWrist,
    KeyPointName_LeftHip,
    KeyPointName_RightHip,
    KeyPointName_LeftKnee,
    KeyPointName_RightKnee,
    KeyPointName_LeftAnkle,
    KeyPointName_RightAnkle
  };
  return values;
}

inline const char * const *EnumNamesKeyPointName() {
  static const char * const names[18] = {
    "Nose",
    "LeftEye",
    "RightEye",
    "LeftEar",
    "RightEar",
    "LeftShoulder",
    "RightShoulder",
    "LeftElbow",
    "RightElbow",
    "LeftWrist",
    "RightWrist",
    "LeftHip",
    "RightHip",
    "LeftKnee",
    "RightKnee",
    "LeftAnkle",
    "RightAnkle",
    nullptr
  };
  return names;
}

inline const char *EnumNameKeyPointName(KeyPointName e) {
  if (::flatbuffers::IsOutRange(e, KeyPointName_Nose, KeyPointName_RightAnkle)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesKeyPointName()[index];
}

enum Point : uint8_t {
  Point_NONE = 0,
  Point_Point2d = 1,
  Point_MIN = Point_NONE,
  Point_MAX = Point_Point2d
};

inline const Point (&EnumValuesPoint())[2] {
  static const Point values[] = {
    Point_NONE,
    Point_Point2d
  };
  return values;
}

inline const char * const *EnumNamesPoint() {
  static const char * const names[3] = {
    "NONE",
    "Point2d",
    nullptr
  };
  return names;
}

inline const char *EnumNamePoint(Point e) {
  if (::flatbuffers::IsOutRange(e, Point_NONE, Point_Point2d)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesPoint()[index];
}

template<typename T> struct PointTraits {
  static const Point enum_value = Point_NONE;
};

template<> struct PointTraits<SmartCamera::Point2d> {
  static const Point enum_value = Point_Point2d;
};

bool VerifyPoint(::flatbuffers::Verifier &verifier, const void *obj, Point type);
bool VerifyPointVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<uint8_t> *types);

struct Point2d FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef Point2dBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_X = 4,
    VT_Y = 6
  };
  int32_t x() const {
    return GetField<int32_t>(VT_X, 0);
  }
  int32_t y() const {
    return GetField<int32_t>(VT_Y, 0);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_X, 4) &&
           VerifyField<int32_t>(verifier, VT_Y, 4) &&
           verifier.EndTable();
  }
};

struct Point2dBuilder {
  typedef Point2d Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_x(int32_t x) {
    fbb_.AddElement<int32_t>(Point2d::VT_X, x, 0);
  }
  void add_y(int32_t y) {
    fbb_.AddElement<int32_t>(Point2d::VT_Y, y, 0);
  }
  explicit Point2dBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<Point2d> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<Point2d>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<Point2d> CreatePoint2d(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int32_t x = 0,
    int32_t y = 0) {
  Point2dBuilder builder_(_fbb);
  builder_.add_y(y);
  builder_.add_x(x);
  return builder_.Finish();
}

struct KeyPoint FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef KeyPointBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SCORE = 4,
    VT_POINT_TYPE = 6,
    VT_POINT = 8,
    VT_NAME = 10
  };
  float score() const {
    return GetField<float>(VT_SCORE, 0.0f);
  }
  SmartCamera::Point point_type() const {
    return static_cast<SmartCamera::Point>(GetField<uint8_t>(VT_POINT_TYPE, 0));
  }
  const void *point() const {
    return GetPointer<const void *>(VT_POINT);
  }
  template<typename T> const T *point_as() const;
  const SmartCamera::Point2d *point_as_Point2d() const {
    return point_type() == SmartCamera::Point_Point2d ? static_cast<const SmartCamera::Point2d *>(point()) : nullptr;
  }
  SmartCamera::KeyPointName name() const {
    return static_cast<SmartCamera::KeyPointName>(GetField<int8_t>(VT_NAME, 0));
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<float>(verifier, VT_SCORE, 4) &&
           VerifyField<uint8_t>(verifier, VT_POINT_TYPE, 1) &&
           VerifyOffset(verifier, VT_POINT) &&
           VerifyPoint(verifier, point(), point_type()) &&
           VerifyField<int8_t>(verifier, VT_NAME, 1) &&
           verifier.EndTable();
  }
};

template<> inline const SmartCamera::Point2d *KeyPoint::point_as<SmartCamera::Point2d>() const {
  return point_as_Point2d();
}

struct KeyPointBuilder {
  typedef KeyPoint Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_score(float score) {
    fbb_.AddElement<float>(KeyPoint::VT_SCORE, score, 0.0f);
  }
  void add_point_type(SmartCamera::Point point_type) {
    fbb_.AddElement<uint8_t>(KeyPoint::VT_POINT_TYPE, static_cast<uint8_t>(point_type), 0);
  }
  void add_point(::flatbuffers::Offset<void> point) {
    fbb_.AddOffset(KeyPoint::VT_POINT, point);
  }
  void add_name(SmartCamera::KeyPointName name) {
    fbb_.AddElement<int8_t>(KeyPoint::VT_NAME, static_cast<int8_t>(name), 0);
  }
  explicit KeyPointBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<KeyPoint> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<KeyPoint>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<KeyPoint> CreateKeyPoint(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    float score = 0.0f,
    SmartCamera::Point point_type = SmartCamera::Point_NONE,
    ::flatbuffers::Offset<void> point = 0,
    SmartCamera::KeyPointName name = SmartCamera::KeyPointName_Nose) {
  KeyPointBuilder builder_(_fbb);
  builder_.add_point(point);
  builder_.add_score(score);
  builder_.add_name(name);
  builder_.add_point_type(point_type);
  return builder_.Finish();
}

struct GeneralPose FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef GeneralPoseBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SCORE = 4,
    VT_KEYPOINT_LIST = 6
  };
  float score() const {
    return GetField<float>(VT_SCORE, 0.0f);
  }
  const ::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::KeyPoint>> *keypoint_list() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::KeyPoint>> *>(VT_KEYPOINT_LIST);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<float>(verifier, VT_SCORE, 4) &&
           VerifyOffset(verifier, VT_KEYPOINT_LIST) &&
           verifier.VerifyVector(keypoint_list()) &&
           verifier.VerifyVectorOfTables(keypoint_list()) &&
           verifier.EndTable();
  }
};

struct GeneralPoseBuilder {
  typedef GeneralPose Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_score(float score) {
    fbb_.AddElement<float>(GeneralPose::VT_SCORE, score, 0.0f);
  }
  void add_keypoint_list(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::KeyPoint>>> keypoint_list) {
    fbb_.AddOffset(GeneralPose::VT_KEYPOINT_LIST, keypoint_list);
  }
  explicit GeneralPoseBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<GeneralPose> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<GeneralPose>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<GeneralPose> CreateGeneralPose(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    float score = 0.0f,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::KeyPoint>>> keypoint_list = 0) {
  GeneralPoseBuilder builder_(_fbb);
  builder_.add_keypoint_list(keypoint_list);
  builder_.add_score(score);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<GeneralPose> CreateGeneralPoseDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    float score = 0.0f,
    const std::vector<::flatbuffers::Offset<SmartCamera::KeyPoint>> *keypoint_list = nullptr) {
  auto keypoint_list__ = keypoint_list ? _fbb.CreateVector<::flatbuffers::Offset<SmartCamera::KeyPoint>>(*keypoint_list) : 0;
  return SmartCamera::CreateGeneralPose(
      _fbb,
      score,
      keypoint_list__);
}

struct PoseEstimationData FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef PoseEstimationDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_POSE_LIST = 4
  };
  const ::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::GeneralPose>> *pose_list() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::GeneralPose>> *>(VT_POSE_LIST);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_POSE_LIST) &&
           verifier.VerifyVector(pose_list()) &&
           verifier.VerifyVectorOfTables(pose_list()) &&
           verifier.EndTable();
  }
};

struct PoseEstimationDataBuilder {
  typedef PoseEstimationData Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_pose_list(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::GeneralPose>>> pose_list) {
    fbb_.AddOffset(PoseEstimationData::VT_POSE_LIST, pose_list);
  }
  explicit PoseEstimationDataBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<PoseEstimationData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<PoseEstimationData>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<PoseEstimationData> CreatePoseEstimationData(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<SmartCamera::GeneralPose>>> pose_list = 0) {
  PoseEstimationDataBuilder builder_(_fbb);
  builder_.add_pose_list(pose_list);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<PoseEstimationData> CreatePoseEstimationDataDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<::flatbuffers::Offset<SmartCamera::GeneralPose>> *pose_list = nullptr) {
  auto pose_list__ = pose_list ? _fbb.CreateVector<::flatbuffers::Offset<SmartCamera::GeneralPose>>(*pose_list) : 0;
  return SmartCamera::CreatePoseEstimationData(
      _fbb,
      pose_list__);
}

struct PoseEstimationTop FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef PoseEstimationTopBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PERCEPTION = 4
  };
  const SmartCamera::PoseEstimationData *perception() const {
    return GetPointer<const SmartCamera::PoseEstimationData *>(VT_PERCEPTION);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_PERCEPTION) &&
           verifier.VerifyTable(perception()) &&
           verifier.EndTable();
  }
};

struct PoseEstimationTopBuilder {
  typedef PoseEstimationTop Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_perception(::flatbuffers::Offset<SmartCamera::PoseEstimationData> perception) {
    fbb_.AddOffset(PoseEstimationTop::VT_PERCEPTION, perception);
  }
  explicit PoseEstimationTopBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<PoseEstimationTop> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<PoseEstimationTop>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<PoseEstimationTop> CreatePoseEstimationTop(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<SmartCamera::PoseEstimationData> perception = 0) {
  PoseEstimationTopBuilder builder_(_fbb);
  builder_.add_perception(perception);
  return builder_.Finish();
}

inline bool VerifyPoint(::flatbuffers::Verifier &verifier, const void *obj, Point type) {
  switch (type) {
    case Point_NONE: {
      return true;
    }
    case Point_Point2d: {
      auto ptr = reinterpret_cast<const SmartCamera::Point2d *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return true;
  }
}

inline bool VerifyPointVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<uint8_t> *types) {
  if (!values || !types) return !values && !types;
  if (values->size() != types->size()) return false;
  for (::flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifyPoint(
        verifier,  values->Get(i), types->GetEnum<Point>(i))) {
      return false;
    }
  }
  return true;
}

inline const SmartCamera::PoseEstimationTop *GetPoseEstimationTop(const void *buf) {
  return ::flatbuffers::GetRoot<SmartCamera::PoseEstimationTop>(buf);
}

inline const SmartCamera::PoseEstimationTop *GetSizePrefixedPoseEstimationTop(const void *buf) {
  return ::flatbuffers::GetSizePrefixedRoot<SmartCamera::PoseEstimationTop>(buf);
}

inline bool VerifyPoseEstimationTopBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<SmartCamera::PoseEstimationTop>(nullptr);
}

inline bool VerifySizePrefixedPoseEstimationTopBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<SmartCamera::PoseEstimationTop>(nullptr);
}

inline void FinishPoseEstimationTopBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<SmartCamera::PoseEstimationTop> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedPoseEstimationTopBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<SmartCamera::PoseEstimationTop> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace SmartCamera

#endif  // FLATBUFFERS_GENERATED_POSEESTIMATION_SMARTCAMERA_H_
