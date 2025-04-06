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

#include <string>
#include <vector>

#include "classification_generated.h"
#include "classification_utils.hpp"
#include "flatbuffers/flatbuffers.h"
#include "sensor.h"
#include "testing_utils.hpp"

EdgeAppLibSensorStream s_stream = 0;

class ClassificationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const char *data_body_str = "[0.10, 0.81, 0.32, 0.63, 0.54]";

    out_data = StringToFloatArray((char *)data_body_str, &num_array_elements);
  }

  void TearDown() override { free(out_data); }

  uint16_t data_size = 5;
  uint16_t output_size = 3;
  float *out_data = nullptr;
  uint32_t num_array_elements;
  uint8_t out_size = 5;
};

TEST_F(ClassificationTest, CreateClassificationFlatbufferTest) {
  std::vector<float> expected_scores = {0.81, 0.63, 0.54, 0.32, 0.10};
  std::vector<uint16_t> expected_indices = {1, 3, 4, 2, 0};
  flatbuffers::FlatBufferBuilder builder;

  extern DataProcessorCustomParam cls_param;

  cls_param.maxPredictions = 3;

  int res = CreateClassificationFlatbuffer(out_data, num_array_elements,
                                           &builder, cls_param);

  EXPECT_EQ(res, 0);
  // Validate the FlatBuffer content
  auto flatBufferOut = flatbuffers::GetRoot<SmartCamera::ClassificationTop>(
      builder.GetBufferPointer());
  const auto *classification_list =
      flatBufferOut->perception()->classification_list();
  EXPECT_NE(classification_list, nullptr);
  EXPECT_EQ(classification_list->size(), output_size);

  for (size_t i = 0; i < classification_list->size(); ++i) {
    EXPECT_EQ(classification_list->Get(i)->class_id(),
              expected_indices[i]);  //  class_id:uint;
    EXPECT_EQ(classification_list->Get(i)->score(),
              expected_scores[i]);  //  score:float;
  }
}

TEST_F(ClassificationTest, CreateClassificationFlatbufferTestNull) {
  std::vector<float> expected_scores = {0.81, 0.63, 0.54, 0.32, 0.10};
  std::vector<uint16_t> expected_indices = {1, 3, 4, 2, 0};
  flatbuffers::FlatBufferBuilder builder;

  extern DataProcessorCustomParam cls_param;
  float *out_data_null = NULL;
  int res = CreateClassificationFlatbuffer(out_data_null, num_array_elements,
                                           &builder, cls_param);

  EXPECT_EQ(res, -1);
}
