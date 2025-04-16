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
#include <gtest/gtest.h>
#include <inttypes.h>
#include <math.h>

#include <fstream>
#include <list>
#include <string>
#include <vector>

#include "flatbuffers/flatbuffers.h"
#include "poseestimation_generated.h"
#include "posenet_utils.hpp"
#include "sensor.h"
#include "testing_utils.hpp"
#define EPSILON 1e-5
class PoseNetTest : public ::testing::Test {
 protected:
  void SetUp() override {
    init_pose_result();
    uint32_t num_array_elements = 0;
    out_data = load_output_tensor(
        "../../../test_data/westworld_out_w481_h353.bin", &num_array_elements);
    out_size = num_array_elements;
  }
  void TearDown() override {
    pose_result.clear();
    pose_result.shrink_to_fit();
    delete[] out_data;
  }
  pose_t pose1 = {0.836640f,  // pose_score
                  {{175.0f, 67.0f, 0.998968f},
                   {180.0f, 59.0f, 0.993307f},
                   {170.0f, 59.0f, 0.957912f},
                   {192.0f, 62.0f, 0.952574f},
                   {165.0f, 61.0f, 0.106691f},
                   {213.0f, 102.0f, 0.992423f},
                   {147.0f, 100.0f, 0.997199f},
                   {225.0f, 157.0f, 0.939913f},
                   {142.0f, 153.0f, 0.991423f},
                   {212.0f, 210.0f, 0.893309f},
                   {138.0f, 206.0f, 0.904651f},
                   {191.0f, 200.0f, 0.946597f},
                   {162.0f, 198.0f, 0.970688f},
                   {188.0f, 272.0f, 0.777300f},
                   {157.0f, 276.0f, 0.893309f},
                   {188.0f, 342.0f, 0.468791f},
                   {161.0f, 357.0f, 0.437824f}}};
  pose_t pose2 = {0.829633f,  // pose_score
                  {{429.0f, 86.0f, 0.998299f},
                   {434.0f, 81.0f, 0.985936f},
                   {425.0f, 82.0f, 0.904651f},
                   {444.0f, 84.0f, 0.939913f},
                   {421.0f, 86.0f, 0.119203f},
                   {458.0f, 121.0f, 0.996406f},
                   {416.0f, 115.0f, 0.970688f},
                   {463.0f, 162.0f, 0.957912f},
                   {413.0f, 156.0f, 0.798187f},
                   {454.0f, 222.0f, 0.437824f},
                   {409.0f, 216.0f, 0.592667f},
                   {445.0f, 215.0f, 0.984094f},
                   {417.0f, 214.0f, 0.962673f},
                   {440.0f, 280.0f, 0.974043f},
                   {419.0f, 279.0f, 0.957912f},
                   {434.0f, 343.0f, 0.817575f},
                   {420.0f, 342.0f, 0.705785f}}};
  pose_t pose3 = {0.762777f,  // pose_score
                  {{320.0f, 64.0f, 0.999620f},
                   {326.0f, 59.0f, 0.996406f},
                   {315.0f, 59.0f, 0.993307f},
                   {333.0f, 62.0f, 0.777300f},
                   {305.0f, 63.0f, 0.924142f},
                   {343.0f, 102.0f, 0.995930f},
                   {297.0f, 103.0f, 0.998299f},
                   {345.0f, 157.0f, 0.991423f},
                   {291.0f, 153.0f, 0.974043f},
                   {352.0f, 212.0f, 0.946597f},
                   {284.0f, 205.0f, 0.932453f},
                   {333.0f, 196.0f, 0.991423f},
                   {307.0f, 193.0f, 0.993307f},
                   {332.0f, 285.0f, 0.201813f},
                   {314.0f, 280.0f, 0.245085f},
                   {343.0f, 368.0f, 0.003594f},
                   {325.0f, 368.0f, 0.002473f}}};
  void init_pose_result() {
    pose_result.push_back(pose1);
    pose_result.push_back(pose2);
    pose_result.push_back(pose3);
  }
  float *load_output_tensor(const std::string &in_bin_file,
                            uint32_t *out_size) {
    float *data = nullptr;
    std::ifstream ifs(in_bin_file, std::ios::in | std::ios::binary);
    if (!ifs) {
      return nullptr;
    }
    ifs.seekg(0, ifs.end);
    uint32_t file_size = ifs.tellg();
    ifs.seekg(0, ifs.beg);
    uint32_t num_floats = file_size / sizeof(float);
    data = new float[num_floats];
    if (!data) {
      ifs.close();
      return nullptr;
    }
    ifs.read(reinterpret_cast<char *>(data), file_size);
    ifs.close();
    *out_size = file_size;
    return data;
  }
  std::vector<pose_t> pose_result;
  uint16_t output_size = 3;
  uint32_t num_array_elements = 0;
  uint32_t out_size = 0;
  float *out_data = nullptr;
};

TEST_F(PoseNetTest, CreatePoseNetDataTest) {
  extern DataProcessorCustomParam g_posenet_param;

  std::vector<pose_t> test_pose_result;
  int ret =
      createPoseNetData(out_data, out_size, g_posenet_param, test_pose_result);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(test_pose_result.size(), pose_result.size());
  for (size_t i = 0; i < test_pose_result.size(); ++i) {
    EXPECT_NEAR(test_pose_result[i].pose_score, pose_result[i].pose_score,
                EPSILON);
    for (int8_t j = 0; j <= SmartCamera::KeyPointName_MAX; j++) {
      EXPECT_NEAR((test_pose_result[i].keypoint[j].score),
                  pose_result[i].keypoint[j].score, EPSILON);
      EXPECT_EQ((int)(test_pose_result[i].keypoint[j].x *
                      g_posenet_param.input_width),
                pose_result[i].keypoint[j].x);
      EXPECT_EQ((int)(test_pose_result[i].keypoint[j].y *
                      g_posenet_param.input_height),
                pose_result[i].keypoint[j].y);
    }
  }
}

TEST_F(PoseNetTest, CreatePoseNetFlatbufferTest) {
  extern DataProcessorCustomParam g_posenet_param;
  g_posenet_param.input_height = 481;
  g_posenet_param.input_width = 353;
  flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
  createPoseEstimationOutputFlatbuffer(&builder, g_posenet_param, pose_result);
  auto flatBufferOut = flatbuffers::GetRoot<SmartCamera::PoseEstimationTop>(
      builder.GetBufferPointer());
  const auto *pose_list = flatBufferOut->perception()->pose_list();
  EXPECT_NE(pose_list, nullptr);
  EXPECT_EQ(pose_list->size(), output_size);
  for (size_t i = 0; i < pose_list->size(); ++i) {
    EXPECT_NEAR(pose_list->Get(i)->score(), pose_result[i].pose_score, EPSILON);
    for (int8_t j = 0; j <= SmartCamera::KeyPointName_MAX; j++) {
      EXPECT_EQ(pose_list->Get(i)->keypoint_list()->Get(j)->score(),
                pose_result[i].keypoint[j].score);
      EXPECT_EQ(
          pose_list->Get(i)->keypoint_list()->Get(j)->point_as_Point2d()->x(),
          pose_result[i].keypoint[j].x * (g_posenet_param.input_width - 1));
      EXPECT_EQ(
          pose_list->Get(i)->keypoint_list()->Get(j)->point_as_Point2d()->y(),
          pose_result[i].keypoint[j].y * (g_posenet_param.input_height - 1));
    }
  }
}
