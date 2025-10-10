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

#include "lp_recog_utils.hpp"
#include "testing_utils.hpp"

class LPRecogUtilsTestFixture : public ::testing::Test {
 protected:
  DataProcessorCustomParam_LPD param;
  void SetUp() override {
    param.input_width = 300;
    param.input_height = 300;
    param.bbox_normalized = false;
  }
};

TEST(LPRecogUtilsTest, SimplePlate) {
  // Upper row: Nagoya 589, Lower row: ka 45-67
  // Category IDs depend on the order of CATEGORIES
  // KANJI: "Nagoya" → SPECIAL(2)+NUMBER(10)+KANJI(3)=15
  // Numbers: "5","8","9" → SPECIAL(2)+NUMBER(5,8,9)=7,10,11
  // Hiragana: "ka" → SPECIAL(2)+NUMBER(10)+KANJI(116)+HIRAGANA(36)=164
  // Lower row numbers: "4","5","6","7" → SPECIAL(2)+NUMBER(4,5,6,7)=6,7,8,9

  std::vector<Prediction> preds = {
      // Upper row
      {0.2125, 0.24, 0.5875, 0.4, 0.99, 15},     // Nagoya
      {0.6, 0.25667, 0.67, 0.4, 0.98, 7},        // 5
      {0.67, 0.25667, 0.74, 0.4, 0.97, 10},      // 8
      {0.7425, 0.25667, 0.8125, 0.4, 0.96, 11},  // 9
      // Lower row
      {0.1, 0.5, 0.225, 0.78667, 0.95, 164},    // ka (hiragana)
      {0.2375, 0.48667, 0.3925, 0.8, 0.94, 6},  // 4
      {0.3925, 0.48667, 0.5475, 0.8, 0.93, 7},  // 5
      {0.43, 0.48667, 0.52, 0.8, 0.93, 0},      // -
      {0.5325, 0.48667, 0.63, 0.8, 0.92, 8},    // 6
      {0.6125, 0.48333, 0.7675, 0.8, 0.91, 9}   // 7
  };

  std::string result = interpret_predictions(preds);
  printf("Result: %s\n", result.c_str());
  std::string expected_str = "Nagoya 589, ka 45-67";
  EXPECT_EQ(result, expected_str)
      << "Actual result: " << result << "\nExpected result: " << expected_str;
}

TEST_F(LPRecogUtilsTestFixture, NoKanjiPlate) {
  // Upper row: 123, Lower row: sa 12-34
  // No Kanji in upper row
  std::vector<Prediction> preds = {
      // Upper row (only numbers)
      {0.21, 0.24, 0.58, 0.4, 0.99, 2},  // 0 (Kanji was not detected)
      {0.25, 0.24, 0.32, 0.4, 0.99, 3},  // 1
      {0.33, 0.24, 0.40, 0.4, 0.98, 4},  // 2
      {0.41, 0.24, 0.48, 0.4, 0.97, 5},  // 3
      // Lower row
      {0.1, 0.5, 0.225, 0.78667, 0.95, 128},    // sa (hiragana)
      {0.2375, 0.48667, 0.3925, 0.8, 0.94, 3},  // 1
      {0.3925, 0.48667, 0.5475, 0.8, 0.93, 4},  // 2
      {0.43, 0.48667, 0.52, 0.8, 0.93, 0},      // -
      {0.5325, 0.48667, 0.63, 0.8, 0.92, 5},    // 3
      {0.6125, 0.48333, 0.7675, 0.8, 0.91, 6}   // 4
  };

  std::string result = interpret_predictions(preds);
  printf("Result (No Kanji): %s\n", result.c_str());
  std::string expected_str = "? 012, sa 12-34";
  EXPECT_EQ(result, expected_str)
      << "Actual result: " << result << "\nExpected result: " << expected_str;
}

TEST_F(LPRecogUtilsTestFixture, DotPlate) {
  // Upper row: Shonan 300, Lower row: a .. .9
  // No Kanji in upper row
  std::vector<Prediction> preds = {
      // Upper row (only numbers)
      {0.21, 0.24, 0.58, 0.4, 0.99, 67},  //  Shonan
      {0.25, 0.24, 0.32, 0.4, 0.99, 5},   // 1
      {0.33, 0.24, 0.40, 0.4, 0.98, 2},   // 2
      {0.41, 0.24, 0.48, 0.4, 0.97, 2},   // 3
      // Lower row
      {0.1, 0.5, 0.225, 0.78667, 0.95, 160},  // a (hiragana)
      {0.23, 0.48667, 0.32, 0.8, 0.94, 1},    // .
      {0.33, 0.48667, 0.42, 0.8, 0.93, 1},    // .
      {0.53, 0.48667, 0.62, 0.8, 0.92, 1},    // .
      {0.63, 0.48333, 0.77, 0.8, 0.91, 11}    // 9
  };

  std::string result = interpret_predictions(preds);
  printf("Result: %s\n", result.c_str());
  std::string expected_str = "Shonan 300, a .. .9";
  EXPECT_EQ(result, expected_str)
      << "Actual result: " << result << "\nExpected result: " << expected_str;
}

TEST_F(LPRecogUtilsTestFixture, CreateLPDetections_Basic) {
  // Prepare dummy input data for 2 detections
  // Each detection: [score, ymin, xmin, ymax, xmax, class_id]
  // Layout of in_data: [all scores][all ymin][all xmin][all ymax][all
  // xmax][num_detections][all class_id]
  float in_data[2 * 6 + 1] = {// score
                              0.95f, 0.85f,
                              // ymin
                              0.1f, 0.2f,
                              // xmin
                              0.15f, 0.25f,
                              // ymax
                              0.3f, 0.4f,
                              // xmax
                              0.35f, 0.45f,
                              // valid num_detections
                              2.0f,
                              // class_id
                              5.0f, 10.0f};

  param.bbox_normalized = true;

  EdgeAppCore::Tensor tensor;
  tensor.size = sizeof(float) * (2 * 6 + 1);  // 2 detections, 6 values each

  Detections *dets =
      CreateLPDetections(in_data, sizeof(in_data), param, &tensor);
  ASSERT_NE(dets, nullptr);
  EXPECT_EQ(dets->num_detections, 2);

  ASSERT_NE(dets->detection_data, nullptr);

  // Check first detection
  EXPECT_EQ(dets->detection_data[0].class_id, 5);
  EXPECT_FLOAT_EQ(dets->detection_data[0].score, 0.95f);
  EXPECT_EQ(dets->detection_data[0].bbox.left,
            static_cast<uint16_t>(round(0.15f * (param.input_width - 1))));
  EXPECT_EQ(dets->detection_data[0].bbox.top,
            static_cast<uint16_t>(round(0.1f * (param.input_height - 1))));
  EXPECT_EQ(dets->detection_data[0].bbox.right,
            static_cast<uint16_t>(round(0.35f * (param.input_width - 1))));
  EXPECT_EQ(dets->detection_data[0].bbox.bottom,
            static_cast<uint16_t>(round(0.3f * (param.input_height - 1))));

  // Check second detection
  EXPECT_EQ(dets->detection_data[1].class_id, 10);
  EXPECT_FLOAT_EQ(dets->detection_data[1].score, 0.85f);
  EXPECT_EQ(dets->detection_data[1].bbox.left,
            static_cast<uint16_t>(round(0.25f * (param.input_width - 1))));
  EXPECT_EQ(dets->detection_data[1].bbox.top,
            static_cast<uint16_t>(round(0.2f * (param.input_height - 1))));
  EXPECT_EQ(dets->detection_data[1].bbox.right,
            static_cast<uint16_t>(round(0.45f * (param.input_width - 1))));
  EXPECT_EQ(dets->detection_data[1].bbox.bottom,
            static_cast<uint16_t>(round(0.4f * (param.input_height - 1))));

  free(dets->detection_data);
  free(dets);
}

TEST_F(LPRecogUtilsTestFixture, CreateLPDetections_ZeroDetections) {
  float in_data[6 + 2] = {// score
                          0,
                          // ymin
                          0,
                          // xmin
                          0,
                          // ymax
                          0,
                          // xmax
                          0,
                          // valid num_detections
                          0.0f,
                          // class_id
                          0};

  param.bbox_normalized = true;

  EdgeAppCore::Tensor tensor;
  tensor.size = sizeof(float) * 6;

  Detections *dets =
      CreateLPDetections(in_data, sizeof(in_data), param, &tensor);
  ASSERT_NE(dets, nullptr);
  EXPECT_EQ(dets->num_detections, 0);
  EXPECT_NE(dets->detection_data, nullptr);

  free(dets->detection_data);
  free(dets);
}

TEST_F(LPRecogUtilsTestFixture, CreateLPDetections_ExceedMaxDetectionDataSize) {
  size_t over_detections = UINT16_MAX + 1;

  // Each detection: [score, ymin, xmin, ymax, xmax, class_id]
  // Layout: [all scores][all ymin][all xmin][all ymax][all
  // xmax][num_detections][all class_id]
  size_t num_values = over_detections * 6 + 1 + over_detections;
  std::vector<float> in_data(num_values, 0.5f);

  // Set num_detections to over_detections
  in_data[over_detections * 5] = static_cast<float>(over_detections);

  param.bbox_normalized = true;

  EdgeAppCore::Tensor tensor;
  tensor.size = sizeof(float) * (over_detections * 6 + 1);

  Detections *dets = CreateLPDetections(
      in_data.data(), in_data.size() * sizeof(float), param, &tensor);
  // Should return NULL due to exceeding MAX_DETECTION_DATA_SIZE
  EXPECT_EQ(dets, nullptr);
}

TEST_F(LPRecogUtilsTestFixture, CreateLPDetections_Unnormalized) {
  // Prepare dummy input data for 2 detections (unnormalized coordinates)
  // Each detection: [score, ymin, xmin, ymax, xmax, class_id]
  // Layout of in_data: [all scores][all ymin][all xmin][all ymax][all
  // xmax][num_detections][all class_id]
  float in_data[2 * 6 + 1] = {// score
                              0.90f, 0.80f,
                              // ymin
                              10.0f, 20.0f,
                              // xmin
                              15.0f, 25.0f,
                              // ymax
                              30.0f, 40.0f,
                              // xmax
                              35.0f, 45.0f,
                              // valid num_detections
                              2.0f,
                              // class_id
                              3.0f, 7.0f};

  param.bbox_normalized = false;

  EdgeAppCore::Tensor tensor;
  tensor.size = sizeof(float) * (2 * 6 + 1);

  Detections *dets =
      CreateLPDetections(in_data, sizeof(in_data), param, &tensor);
  ASSERT_NE(dets, nullptr);
  EXPECT_EQ(dets->num_detections, 2);
  ASSERT_NE(dets->detection_data, nullptr);

  // Check first detection
  EXPECT_EQ(dets->detection_data[0].class_id, 3);
  EXPECT_FLOAT_EQ(dets->detection_data[0].score, 0.90f);
  EXPECT_EQ(dets->detection_data[0].bbox.left,
            static_cast<uint16_t>(round(15.0f)));
  EXPECT_EQ(dets->detection_data[0].bbox.top,
            static_cast<uint16_t>(round(10.0f)));
  EXPECT_EQ(dets->detection_data[0].bbox.right,
            static_cast<uint16_t>(round(35.0f)));
  EXPECT_EQ(dets->detection_data[0].bbox.bottom,
            static_cast<uint16_t>(round(30.0f)));

  // Check second detection
  EXPECT_EQ(dets->detection_data[1].class_id, 7);
  EXPECT_FLOAT_EQ(dets->detection_data[1].score, 0.80f);
  EXPECT_EQ(dets->detection_data[1].bbox.left,
            static_cast<uint16_t>(round(25.0f)));
  EXPECT_EQ(dets->detection_data[1].bbox.top,
            static_cast<uint16_t>(round(20.0f)));
  EXPECT_EQ(dets->detection_data[1].bbox.right,
            static_cast<uint16_t>(round(45.0f)));
  EXPECT_EQ(dets->detection_data[1].bbox.bottom,
            static_cast<uint16_t>(round(40.0f)));

  free(dets->detection_data);
  free(dets);
}
TEST(LPRecogUtilsTest, IsValidJapaneseNumberPlate_ValidPlates) {
  // Valid plates with dash in the correct position (3rd from end)
  EXPECT_TRUE(is_valid_japanese_number_plate("Nagoya 589, ka 45-67"));
  EXPECT_TRUE(is_valid_japanese_number_plate("Tokyo 123, su 12-34"));
  EXPECT_TRUE(is_valid_japanese_number_plate("Test AB-CD"));
  EXPECT_TRUE(is_valid_japanese_number_plate("12-34"));

  // Valid plates with dot
  EXPECT_TRUE(is_valid_japanese_number_plate("Shonan 300, a .. .9"));
}

TEST(LPRecogUtilsTest, IsValidJapaneseNumberPlate_InvalidPlates) {
  // Plates with question marks
  EXPECT_FALSE(is_valid_japanese_number_plate("Test?123"));
  EXPECT_FALSE(is_valid_japanese_number_plate("? 012, sa 12-34"));
  EXPECT_FALSE(is_valid_japanese_number_plate("ABC?DEF"));

  // Plates with consecutive dashes
  EXPECT_FALSE(is_valid_japanese_number_plate("Test--123"));
  EXPECT_FALSE(is_valid_japanese_number_plate("AB--CD"));
  EXPECT_FALSE(is_valid_japanese_number_plate("--test"));

  // Plates with neither dot nor dash
  EXPECT_FALSE(is_valid_japanese_number_plate("Test123"));
  EXPECT_FALSE(is_valid_japanese_number_plate("ABCDEF"));
  EXPECT_FALSE(is_valid_japanese_number_plate("123456"));

  // Plates with dash in wrong position
  EXPECT_FALSE(
      is_valid_japanese_number_plate("Test-123"));  // dash not 3rd from end
  EXPECT_FALSE(
      is_valid_japanese_number_plate("-test123"));  // dash at beginning
  EXPECT_FALSE(is_valid_japanese_number_plate(
      "te-st123"));  // dash in middle (not 3rd from end)
  EXPECT_FALSE(
      is_valid_japanese_number_plate("test12-3"));  // dash 2nd from end

  EXPECT_FALSE(is_valid_japanese_number_plate("test123-"));  // dash at end

  // Null pointer
  EXPECT_FALSE(is_valid_japanese_number_plate(nullptr));
}

TEST(LPRecogUtilsTest, IsValidJapaneseNumberPlate_EdgeCases) {
  // Empty string
  EXPECT_FALSE(is_valid_japanese_number_plate(""));

  // Very short strings
  EXPECT_FALSE(is_valid_japanese_number_plate("A-B"));
  EXPECT_FALSE(is_valid_japanese_number_plate("AB-C"));
  EXPECT_FALSE(is_valid_japanese_number_plate("A-BC"));

  // Single dot or dash
  EXPECT_FALSE(is_valid_japanese_number_plate("."));
  EXPECT_FALSE(is_valid_japanese_number_plate("-"));

  // Multiple dots
  EXPECT_TRUE(is_valid_japanese_number_plate(
      "A.B.C"));  // This case is actually invalid but rare. So we keep the
                  // implementation of data_processor simple.
  EXPECT_FALSE(is_valid_japanese_number_plate("..."));
}
