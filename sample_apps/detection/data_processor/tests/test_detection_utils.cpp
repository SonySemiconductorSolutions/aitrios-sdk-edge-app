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

TEST_F(DetectionUtilsTest, ExtractBboxToFlatbufferTest) {
  flatbuffers::Offset<SmartCamera::BoundingBox2d> bbox_data;
  extern DataProcessorCustomParam ssd_param;

  for (int i = 0; i < detect_num; i++) {
    ExtractBboxToFlatbuffer(out_data, i, detect_num, &builder, &bbox_data,
                            ssd_param);
    auto *bbox = reinterpret_cast<const SmartCamera::BoundingBox2d *>(
        builder.GetCurrentBufferPointer() + builder.GetSize() - bbox_data.o);

    EXPECT_EQ(bbox->top(),
              (uint16_t)(round((out_data[i]) * (ssd_param.input_height - 1))));

    EXPECT_EQ(bbox->bottom(),
              (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                               (ssd_param.input_height - 1))));
    EXPECT_EQ(bbox->left(), (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                                             (ssd_param.input_width - 1))));
    ASSERT_EQ(bbox->right(), (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                                              (ssd_param.input_width - 1))));
  }
}

TEST_F(DetectionUtilsTest, ExtractDetectionsToFlatbufferObjectTest) {
  extern DataProcessorCustomParam ssd_param;
  ssd_param.max_detections = 1;
  std::vector<flatbuffers::Offset<SmartCamera::GeneralObject> > gdata_vector;
  ExtractDetectionsToFlatbufferObject(out_data, &gdata_vector, &builder,
                                      detect_num, ssd_param);
  for (int i = 0; i < gdata_vector.size(); i++) {
    auto *detection = reinterpret_cast<const SmartCamera::GeneralObject *>(
        builder.GetCurrentBufferPointer() + builder.GetSize() -
        gdata_vector[i].o);
    auto *bbox = detection->bounding_box_as_BoundingBox2d();

    EXPECT_EQ(bbox->top(),
              (uint16_t)(round((out_data[i]) * (ssd_param.input_height - 1))));
    EXPECT_EQ(bbox->bottom(),
              (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                               (ssd_param.input_height - 1))));
    EXPECT_EQ(bbox->left(), (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                                             (ssd_param.input_width - 1))));
    EXPECT_EQ(bbox->right(), (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                                              (ssd_param.input_width - 1))));

    EXPECT_EQ(detection->score(), out_data[(detect_num * 5) + i]);
    EXPECT_EQ(detection->class_id(), (uint8_t)out_data[(detect_num * 4) + i]);
  }
}

TEST_F(DetectionUtilsTest, CreateSSDOutputFlatbufferTest) {
  extern DataProcessorCustomParam ssd_param;

  ssd_param.max_detections = detect_num;

  int res = CreateSSDOutputFlatbuffer(out_data, detect_num * 6 + 1, &builder,
                                      ssd_param);
  uint8_t *buf_ptr = builder.GetBufferPointer();
  uint32_t buf_size = builder.GetSize();

  auto object_detection_top = SmartCamera::GetObjectDetectionTop(buf_ptr);

  EXPECT_EQ(108, buf_size);
  EXPECT_EQ(res, kDataProcessorOk);

  auto obj_detection_data =
      object_detection_top->perception()->object_detection_list();
  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);

    auto bbox = general_object->bounding_box_as_BoundingBox2d();

    EXPECT_EQ(general_object->class_id(),
              (uint8_t)out_data[(detect_num * 4) + i]);
    EXPECT_EQ(general_object->score(), out_data[(detect_num * 5) + i]);
    EXPECT_EQ(bbox->left(), (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                                             (ssd_param.input_width - 1))));
    EXPECT_EQ(bbox->top(),
              (uint16_t)(round((out_data[i]) * (ssd_param.input_height - 1))));
    EXPECT_EQ(bbox->right(), (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                                              (ssd_param.input_width - 1))));
    EXPECT_EQ(bbox->bottom(),
              (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                               (ssd_param.input_width - 1))));
  }
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

TEST_F(DetectionUtilsLargeNumberTest, CreateSSDOutputFlatbufferTest) {
  extern DataProcessorCustomParam ssd_param;

  ssd_param.max_detections = detect_num;
  // To detect all of detections, set lower than minimum score of detections.
  ssd_param.threshold = (1.0 / detect_num) * 0.5;

  int res = CreateSSDOutputFlatbuffer(out_data, detect_num * 6 + 1, &builder,
                                      ssd_param);
  uint8_t *buf_ptr = builder.GetBufferPointer();
  uint32_t buf_size = builder.GetSize();

  auto object_detection_top = SmartCamera::GetObjectDetectionTop(buf_ptr);

  EXPECT_EQ(11328, buf_size);
  EXPECT_EQ(res, kDataProcessorOk);

  auto obj_detection_data =
      object_detection_top->perception()->object_detection_list();

  EXPECT_EQ(detect_num, obj_detection_data->size());

  for (int i = 0; i < obj_detection_data->size(); ++i) {
    auto general_object = obj_detection_data->Get(i);

    auto bbox = general_object->bounding_box_as_BoundingBox2d();

    EXPECT_EQ(general_object->class_id(),
              (uint8_t)out_data[(detect_num * 4) + i]);
    EXPECT_EQ(general_object->score(), out_data[(detect_num * 5) + i]);
    EXPECT_EQ(bbox->left(), (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                                             (ssd_param.input_width - 1))));
    EXPECT_EQ(bbox->top(),
              (uint16_t)(round((out_data[i]) * (ssd_param.input_height - 1))));
    EXPECT_EQ(bbox->right(), (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                                              (ssd_param.input_width - 1))));
    EXPECT_EQ(bbox->bottom(),
              (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                               (ssd_param.input_width - 1))));
  }
}

TEST_F(DetectionUtilsLargeNumberTest, CreateSSDOutputJsonTest) {
  extern DataProcessorCustomParam ssd_param;

  ssd_param.max_detections = detect_num;
  ssd_param.threshold = 0.0;

  JSON_Value *tensor_output =
      CreateSSDOutputJson(out_data, detect_num * 6 + 1, ssd_param);

  EXPECT_TRUE(tensor_output);

  JSON_Array *obj_detection_data = json_value_get_array(tensor_output);

  size_t count = json_array_get_count(obj_detection_data);
  EXPECT_EQ(detect_num, count);

  for (int i = 0; i < count; ++i) {
    JSON_Object *object = json_array_get_object(obj_detection_data, i);
    int classid = json_object_get_number(object, "class_id");
    float score = json_object_get_number(object, "score");
    JSON_Object *bbox = json_object_get_object(object, "bbox");
    int left = json_object_get_number(bbox, "left");
    int top = json_object_get_number(bbox, "top");
    int right = json_object_get_number(bbox, "right");
    int bottom = json_object_get_number(bbox, "bottom");

    EXPECT_EQ(classid, (uint8_t)out_data[(detect_num * 4) + i]);
    EXPECT_EQ(score, out_data[(detect_num * 5) + i]);
    EXPECT_EQ(left, (uint16_t)(round((out_data[i + (1 * detect_num)]) *
                                     (ssd_param.input_width - 1))));
    EXPECT_EQ(top,
              (uint16_t)(round((out_data[i]) * (ssd_param.input_height - 1))));
    EXPECT_EQ(right, (uint16_t)(round((out_data[i + (3 * detect_num)]) *
                                      (ssd_param.input_width - 1))));
    EXPECT_EQ(bottom, (uint16_t)(round((out_data[i + (2 * detect_num)]) *
                                       (ssd_param.input_width - 1))));
  }

  json_value_free(tensor_output);
}
