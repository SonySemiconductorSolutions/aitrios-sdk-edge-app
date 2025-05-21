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

#include <cmath>
#include <list>
#include <string>

#include "detection_utils.hpp"
#include "flatbuffers/flatbuffers.h"
#include "objectdetection_generated.h"
#include "parson.h"
#include "sensor.h"
#include "testing_utils.hpp"

EdgeAppLibSensorStream s_stream = 0;

class DetectionUtilsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    out_data = StringToFloatArray((char *)data_body_str, &tensor_size);
  }

  void TearDown() override { free(out_data); }

  const char *data_body_str =
      "[0.1, 0.2, 0.15, 0.25, 0.5, 0.6, 0.55, 0.65, 235, 132, 0.8, 0.2, "
      "2]";
  const uint16_t detect_num = 2;
  float data_array[13] = {0.1,  0.2, 0.15, 0.25, 0.5, 0.6, 0.55,
                          0.65, 235, 132,  0.8,  0.2, 2};
  float data_array_int_coords[7] = {48, 32, 175, 160, 235, 0.8, 1};
  flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
  uint32_t tensor_size = 0;
  uint32_t out_size = 0;
  float *out_data;
};

TEST_F(DetectionUtilsTest, ExtractNumberOfDetectionsTest) {
  uint16_t number_of_detections = ExtractNumberOfDetections(tensor_size);
  EXPECT_EQ(number_of_detections, detect_num);

  // minimum tensor size
  number_of_detections = ExtractNumberOfDetections(7);
  EXPECT_EQ(number_of_detections, 1);

  // less than minimum tensor size
  number_of_detections = ExtractNumberOfDetections(6);
  EXPECT_EQ(number_of_detections, 0);
}

TEST_F(DetectionUtilsTest, CreateDetectionsTest) {
  extern DataProcessorCustomParam detection_param;

  Detections *detections =
      CreateDetections(out_data, tensor_size * sizeof(float), detection_param);
  EXPECT_EQ(detections->num_detections, detect_num);

  for (int i = 0; i < detections->num_detections; ++i) {
    EXPECT_EQ(detections->detection_data[i].class_id,
              (uint8_t)out_data[(detect_num * 4) + i]);
    EXPECT_FLOAT_EQ(detections->detection_data[i].score,
                    out_data[(detect_num * 5) + i]);
    EXPECT_EQ(detections->detection_data[i].bbox.left,
              (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                               (detection_param.input_width - 1))));
    EXPECT_EQ(
        detections->detection_data[i].bbox.top,
        (uint16_t)(round((out_data[i]) * (detection_param.input_height - 1))));
    EXPECT_EQ(detections->detection_data[i].bbox.right,
              (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                               (detection_param.input_width - 1))));
    EXPECT_EQ(detections->detection_data[i].bbox.bottom,
              (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                               (detection_param.input_width - 1))));
  }

  free(detections->detection_data);
  free(detections);
}

TEST_F(DetectionUtilsTest, MakeDetectionFlatbufferTest) {
  extern DataProcessorCustomParam detection_param;

  detection_param.max_detections = detect_num;
  Detections *detections =
      CreateDetections(out_data, tensor_size * sizeof(float), detection_param);
  int res = MakeDetectionFlatbuffer(detections, &builder);
  uint8_t *buf_ptr = builder.GetBufferPointer();
  uint32_t buf_size = builder.GetSize();

  auto object_detection_root = SmartCamera::GetObjectDetectionTop(buf_ptr);

  EXPECT_EQ(152, buf_size);
  EXPECT_EQ(res, kDataProcessorOk);

  auto obj_detection_data =
      object_detection_root->perception()->object_detection_list();
  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);

    auto bbox = general_object->bounding_box_as_BoundingBox2d();

    EXPECT_EQ(general_object->class_id(),
              (uint8_t)out_data[(detect_num * 4) + i]);
    EXPECT_EQ(general_object->score(), out_data[(detect_num * 5) + i]);
    EXPECT_EQ(bbox->left(),
              (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                               (detection_param.input_width - 1))));
    EXPECT_EQ(
        bbox->top(),
        (uint16_t)(round((out_data[i]) * (detection_param.input_height - 1))));
    EXPECT_EQ(bbox->right(),
              (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                               (detection_param.input_width - 1))));
    EXPECT_EQ(bbox->bottom(),
              (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                               (detection_param.input_width - 1))));
  }
  free(detections->detection_data);
  free(detections);
}

class DetectionUtilsLargeNumberTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tensor_size = detect_num * (4 + 1 + 1) + 1;
    out_data = (float *)malloc(tensor_size * sizeof(float));
    for (int i = 0; i < detect_num; i++) {
      *(out_data + 4 * i + 0) = 0.1;
      *(out_data + 4 * i + 1) = 0.2;
      *(out_data + 4 * i + 2) = 0.15;
      *(out_data + 4 * i + 3) = 0.25;
      *(out_data + 4 * detect_num + i) = 235;
      *(out_data + 5 * detect_num + i) = 0.8 * (detect_num - i) / detect_num;
    }
    *(out_data + 6 * detect_num) = detect_num;
  }

  void TearDown() override { free(out_data); }

  // Test that number of detections is greater than 255 (= over max uint8_t).
  const uint16_t detect_num = 256;
  flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
  uint32_t tensor_size = 0;
  uint32_t out_size = 0;
  float *out_data;
};

TEST_F(DetectionUtilsLargeNumberTest, ExtractNumberOfDetectionsTest) {
  uint16_t number_of_detections = ExtractNumberOfDetections(tensor_size);
  EXPECT_EQ(number_of_detections, detect_num);
}

TEST_F(DetectionUtilsLargeNumberTest, MakeDetectionJsonTest) {
  extern DataProcessorCustomParam detection_param;

  Detections *detections =
      CreateDetections(out_data, tensor_size * sizeof(float), detection_param);
  JSON_Value *tensor_output = MakeDetectionJson(detections);

  EXPECT_TRUE(tensor_output);

  JSON_Array *obj_detection_data = json_value_get_array(tensor_output);

  size_t count = json_array_get_count(obj_detection_data);
  EXPECT_EQ(detect_num, count);

  for (int i = 0; i < count; ++i) {
    JSON_Object *object = json_array_get_object(obj_detection_data, i);
    int classid = json_object_get_number(object, "class_id");
    float score = json_object_get_number(object, "score");
    JSON_Object *bbox = json_object_get_object(object, "bounding_box");
    int left = json_object_get_number(bbox, "left");
    int top = json_object_get_number(bbox, "top");
    int right = json_object_get_number(bbox, "right");
    int bottom = json_object_get_number(bbox, "bottom");

    EXPECT_EQ(classid, (uint8_t)out_data[(detect_num * 4) + i]);
    EXPECT_EQ(score, out_data[(detect_num * 5) + i]);
    EXPECT_EQ(left, (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                                     (detection_param.input_width - 1))));
    EXPECT_EQ(top, (uint16_t)(round((out_data[i]) *
                                    (detection_param.input_height - 1))));
    EXPECT_EQ(right, (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                                      (detection_param.input_width - 1))));
    EXPECT_EQ(bottom, (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                                       (detection_param.input_width - 1))));
  }

  json_value_free(tensor_output);
  free(detections->detection_data);
  free(detections);
}

class DetectionUtilsFilterDetectionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    detection_data =
        (DetectionData *)malloc(num_of_obj * sizeof(DetectionData));
    detection_data[0] = {4, 0.61, {60, 60, 70, 75}};    // Overlap = 1.0
    detection_data[1] = {2, 0.32, {50, 50, 60, 60}};    // Overlap = 1.0
    detection_data[2] = {1, 0.87, {75, 75, 125, 125}};  // Overlap=0.25
    detection_data[3] = {3, 0.56, {99, 99, 120, 121}};  // 0<Overlap<0.1
    detection_data[4] = {1, 0.59, {55, 56, 65, 64}};    // Overlap = 1.0
    detection_data[5] = {1, 0.89, {5, 6, 7, 8}};        // Overlap = 0.0
    detection_data[6] = {4, 0.51, {51, 49, 99, 101}};   // Overlap =~ 1.0
    detection_data[7] = {0, 0.50, {49, 51, 100, 99}};   // Overlap =~ 1.0

    detections = (Detections *)malloc(sizeof(Detections));
    detections->num_detections = num_of_obj;
    detections->detection_data = detection_data;

    area = {.coordinates = {50, 50, 100, 100},
            .overlap = 0.5,
            .class_ids = {1, 2, 0, 8},
            .num_of_class = 4};
  }

  void TearDown() override {
    free(detections->detection_data);
    free(detections);
    free(area_count);
  }
  uint8_t num_of_obj = 8;
  DetectionData *detection_data = NULL;
  Detections *detections = NULL;
  Area area = {};
  AreaCount *area_count = NULL;
  flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
};

TEST_F(DetectionUtilsFilterDetectionsTest, CreateAreaCountNormalTest) {
  area_count = CreateAreaCount(&detections, area);
  ASSERT_NE(area_count, nullptr);
  EXPECT_EQ(area_count[0].class_id, 2);
  EXPECT_EQ(area_count[0].count, 1);
  EXPECT_EQ(area_count[1].class_id, 1);
  EXPECT_EQ(area_count[1].count, 1);
  EXPECT_EQ(area_count[2].class_id, 0);
  EXPECT_EQ(area_count[2].count, 1);
  for (int i = 3; i < CLASS_IDS_SIZE; i++) {
    EXPECT_EQ(area_count[i].class_id, UINT16_MAX);
    EXPECT_EQ(area_count[i].count, UINT16_MAX);
  }
  EXPECT_EQ(detections->num_detections, 3);

  EXPECT_EQ(detections->detection_data[0].class_id, 2);
  EXPECT_EQ(detections->detection_data[0].bbox.left, 50);
  EXPECT_EQ(detections->detection_data[0].bbox.top, 50);
  EXPECT_EQ(detections->detection_data[0].bbox.right, 60);
  EXPECT_EQ(detections->detection_data[0].bbox.bottom, 60);
  EXPECT_FLOAT_EQ(detections->detection_data[0].score, 0.32);
  EXPECT_EQ(detections->detection_data[1].class_id, 1);
  EXPECT_EQ(detections->detection_data[1].bbox.left, 55);
  EXPECT_EQ(detections->detection_data[1].bbox.top, 56);
  EXPECT_EQ(detections->detection_data[1].bbox.right, 65);
  EXPECT_EQ(detections->detection_data[1].bbox.bottom, 64);
  EXPECT_FLOAT_EQ(detections->detection_data[1].score, 0.59);
  EXPECT_EQ(detections->detection_data[2].class_id, 0);
  EXPECT_EQ(detections->detection_data[2].bbox.left, 49);
  EXPECT_EQ(detections->detection_data[2].bbox.top, 51);
  EXPECT_EQ(detections->detection_data[2].bbox.right, 100);
  EXPECT_EQ(detections->detection_data[2].bbox.bottom, 99);
  EXPECT_FLOAT_EQ(detections->detection_data[2].score, 0.50);
}

TEST_F(DetectionUtilsFilterDetectionsTest, CreateAreaCountEmptyClassIdTest) {
  area = {.coordinates = {50, 50, 100, 100},
          .overlap = 0.5,
          .class_ids = {},
          .num_of_class = 0};
  area_count = CreateAreaCount(&detections, area);

  DetectionData *expected_detection_data =
      (DetectionData *)malloc(5 * sizeof(DetectionData));
  expected_detection_data[0] = {4, 0.61, {60, 60, 70, 75}};   // Overlap = 1.0
  expected_detection_data[1] = {2, 0.32, {50, 50, 60, 60}};   // Overlap = 1.0
  expected_detection_data[2] = {1, 0.59, {55, 56, 65, 64}};   // Overlap = 1.0
  expected_detection_data[3] = {4, 0.51, {51, 49, 99, 101}};  // Overlap =~ 1.0
  expected_detection_data[4] = {0, 0.50, {49, 51, 100, 99}};  // Overlap =~ 1.0
  Detections *expected_detections = (Detections *)malloc(sizeof(Detections));
  expected_detections->num_detections = 5;
  expected_detections->detection_data = expected_detection_data;

  ASSERT_NE(area_count, nullptr);
  EXPECT_EQ(area_count[0].class_id, 4);
  EXPECT_EQ(area_count[0].count, 2);
  EXPECT_EQ(area_count[1].class_id, 2);
  EXPECT_EQ(area_count[1].count, 1);
  EXPECT_EQ(area_count[2].class_id, 1);
  EXPECT_EQ(area_count[2].count, 1);
  EXPECT_EQ(area_count[3].class_id, 0);
  EXPECT_EQ(area_count[3].count, 1);
  for (int i = 4; i < CLASS_IDS_SIZE; i++) {
    EXPECT_EQ(area_count[i].class_id, UINT16_MAX);
    EXPECT_EQ(area_count[i].count, UINT16_MAX);
  }
  EXPECT_EQ(detections->num_detections, expected_detections->num_detections);
  for (int i = 0; i < detections->num_detections; i++) {
    EXPECT_EQ(detections->detection_data[i].class_id,
              expected_detection_data[i].class_id);
    EXPECT_EQ(detections->detection_data[i].bbox.left,
              expected_detection_data[i].bbox.left);
    EXPECT_EQ(detections->detection_data[i].bbox.top,
              expected_detection_data[i].bbox.top);
    EXPECT_EQ(detections->detection_data[i].bbox.right,
              expected_detection_data[i].bbox.right);
    EXPECT_EQ(detections->detection_data[i].bbox.bottom,
              expected_detection_data[i].bbox.bottom);
    EXPECT_FLOAT_EQ(detections->detection_data[i].score,
                    expected_detection_data[i].score);
  }

  free(expected_detection_data);
  free(expected_detections);
}

TEST_F(DetectionUtilsFilterDetectionsTest, FilterByParamsTest) {
  extern DataProcessorCustomParam detection_param;
  detection_param.threshold = 0.6;
  detection_param.max_detections = 5;

  FilterByParams(&detections, detection_param);
  EXPECT_EQ(detections->num_detections, 3);
  EXPECT_EQ(detections->detection_data[0].class_id, 4);
  EXPECT_EQ(detections->detection_data[0].bbox.left, 60);
  EXPECT_EQ(detections->detection_data[0].bbox.top, 60);
  EXPECT_EQ(detections->detection_data[0].bbox.right, 70);
  EXPECT_EQ(detections->detection_data[0].bbox.bottom, 75);
  EXPECT_FLOAT_EQ(detections->detection_data[0].score, 0.61);
  EXPECT_EQ(detections->detection_data[1].class_id, 1);
  EXPECT_EQ(detections->detection_data[1].bbox.left, 75);
  EXPECT_EQ(detections->detection_data[1].bbox.top, 75);
  EXPECT_EQ(detections->detection_data[1].bbox.right, 125);
  EXPECT_EQ(detections->detection_data[1].bbox.bottom, 125);
  EXPECT_FLOAT_EQ(detections->detection_data[1].score, 0.87);
  EXPECT_EQ(detections->detection_data[2].class_id, 1);
  EXPECT_EQ(detections->detection_data[2].bbox.left, 5);
  EXPECT_EQ(detections->detection_data[2].bbox.top, 6);
  EXPECT_EQ(detections->detection_data[2].bbox.right, 7);
  EXPECT_EQ(detections->detection_data[2].bbox.bottom, 8);
  EXPECT_FLOAT_EQ(detections->detection_data[2].score, 0.89);
}

TEST_F(DetectionUtilsFilterDetectionsTest, MakeAreaFlatbufferTest) {
  extern DataProcessorCustomParam detection_param;
  area_count = CreateAreaCount(&detections, area);
  DetectionData *expected_detection_data =
      (DetectionData *)malloc(3 * sizeof(DetectionData));
  expected_detection_data[0] = {2, 0.32, {50, 50, 60, 60}};   // Overlap = 1.0
  expected_detection_data[1] = {1, 0.59, {55, 56, 65, 64}};   // Overlap = 1.0
  expected_detection_data[2] = {0, 0.50, {49, 51, 100, 99}};  // Overlap =~ 1.0

  Detections *expected_detections = (Detections *)malloc(sizeof(Detections));
  expected_detections->num_detections = 3;
  expected_detections->detection_data = expected_detection_data;

  AreaCount expected_area_count[3] = {{.class_id = 2, .count = 1},
                                      {.class_id = 1, .count = 1},
                                      {.class_id = 0, .count = 1}};

  int res =
      MakeAreaFlatbuffer(detections, area_count, &builder, area.num_of_class);
  uint8_t *buf_ptr = builder.GetBufferPointer();
  uint32_t buf_size = builder.GetSize();

  auto object_detection_root = SmartCamera::GetObjectDetectionTop(buf_ptr);

  EXPECT_EQ(264, buf_size);
  EXPECT_EQ(res, kDataProcessorOk);

  auto obj_detection_data = object_detection_root->area_count();

  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);

    EXPECT_EQ(general_object->class_id(),
              detections->detection_data[i].class_id);
  }

  auto area_count_data = object_detection_root->area_count();
  for (int i = 0; i < 3; i++) {
    auto count_data = area_count_data->Get(i);
    EXPECT_EQ(count_data->class_id(), expected_area_count[i].class_id);
    EXPECT_EQ(count_data->count(), expected_area_count[i].count);
  }
  free(expected_detection_data);
  free(expected_detections);
}

TEST_F(DetectionUtilsFilterDetectionsTest, MakeAreaJsonTest) {
  extern DataProcessorCustomParam detection_param;
  area_count = CreateAreaCount(&detections, area);

  JSON_Value *out_json =
      MakeAreaJson(detections, area_count, area.num_of_class);

  EXPECT_TRUE(out_json);

  char expected_json_str[1024];
  snprintf(expected_json_str, sizeof(expected_json_str), R"(
  {
    "area_count":{
        "1": 1,
        "2": 1,
        "0": 1
      },
  "detections":[
        {
            "class_id": 2,
            "score": 0.32,
            "bounding_box": {
            "left": 50,
            "top": 50,
            "right": 60,
            "bottom": 60
            }
        },
        {
            "class_id": 1,
            "score": 0.59,
            "bounding_box": {
            "left": 55,
            "top": 56,
            "right": 65,
            "bottom": 64
            }
        },
        {
            "class_id": 0,
            "score": 0.5,
            "bounding_box": {
                "left": 49,
                "top": 51,
                "right": 100,
                "bottom": 99
            }
        }
       ]
  }
    )");
  JSON_Value *expected_json = json_parse_string(expected_json_str);
  char *actual_json_str = json_serialize_to_string_pretty(out_json);

  EXPECT_TRUE(json_value_equals(out_json, expected_json))
      << "  Actual JSON: " << actual_json_str << '\n'
      << "Expected JSON: " << expected_json_str;

  json_free_serialized_string(actual_json_str);
  json_value_free(expected_json);
  json_value_free(out_json);
}
