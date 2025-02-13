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

#include <string>
#include <vector>

#include "flatbuffers/flatbuffers.h"
#include "segmentation_utils.hpp"
#include "semantic_segmentation_generated.h"
#include "sensor.h"
#include "testing_utils.hpp"

EdgeAppLibSensorStream s_stream = 0;

class SegmentationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    out_data = StringToFloatArray(data_body_str, &tensor_size);
  }

  void TearDown() override { free(out_data); }

  const char *data_body_str =
      "[1, 2, 1, 3, 2, 3, 1, 3, 2, 4, 1, 3, 2, 4, 4, 1]";
  uint32_t input_width = 4;
  uint32_t tensor_size = 0;
  float *out_data = nullptr;
  uint32_t out_size = 0;
};

TEST_F(SegmentationTest, CreateSegmentationFlatbufferTest) {
  // Assuming the previous two functions have been called first
  std::vector<uint16_t> expected_res = {1, 2, 2, 2, 2, 3, 4, 4,
                                        1, 1, 1, 4, 3, 3, 3, 1};
  extern DataProcessorCustomParam seg_param;

  seg_param.inputHeight = 4;
  seg_param.inputWidth = 4;
  flatbuffers::FlatBufferBuilder builder;
  int res =
      CreateSegmentationFlatbuffer(out_data, tensor_size, &builder, seg_param);

  EXPECT_EQ(res, 0);
  // Validate the FlatBuffer content
  auto flatBufferOut =
      flatbuffers::GetRoot<SmartCamera::SemanticSegmentationTop>(
          builder.GetBufferPointer());

  const auto *class_id_map = flatBufferOut->perception()->class_id_map();
  EXPECT_NE(class_id_map, nullptr);
  EXPECT_EQ(class_id_map->size(), expected_res.size());

  for (size_t i = 0; i < class_id_map->size(); ++i) {
    EXPECT_EQ(class_id_map->Get(i), expected_res[i]);
  }
}
