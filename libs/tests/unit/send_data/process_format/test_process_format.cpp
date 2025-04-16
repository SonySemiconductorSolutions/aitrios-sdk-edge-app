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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_device.hpp"
#include "mock_sensor.hpp"
#include "mock_sm_api.hpp"
#include "process_format.hpp"
#include "send_data.h"
#include "sensor.h"
#include "sm.h"
#include "sm_api.hpp"

using namespace EdgeAppLib;
#define PREALLOCATED_BUFFER_SIZE 1024

class ProcessFormatTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {
    // clear mock memory.
    SensorCoreExit(0);
  }
};

void StreamSetPropertyVersionID(uint32_t category, const char *version_id,
                                const char *sensor_name) {
  EdgeAppLibSensorInfoStringProperty sensor_version_id = {};
  sensor_version_id.category = AITRIOS_SENSOR_INFO_STRING_SENSOR_NAME;
  strncpy(sensor_version_id.info, sensor_name,
          AITRIOS_SENSOR_INFO_STRING_LENGTH);
  SensorStreamSetProperty(GetSensorStream(),
                          AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY,
                          &sensor_version_id, sizeof(sensor_version_id));
  sensor_version_id.category = category;
  strncpy(sensor_version_id.info, version_id,
          AITRIOS_SENSOR_INFO_STRING_LENGTH);
  SensorStreamSetProperty(GetSensorStream(),
                          AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY,
                          &sensor_version_id, sizeof(sensor_version_id));
}
TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal) {
  uint8_t in_data[5] = {0x51, 0x53, 0x55, 0x57, 0x59};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  void *buffer = malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        (char *)buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  JSON_Value *output_tensor_value = json_parse_string((char *)buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  const char *versionId =
      json_object_get_string(output_tensor_object, "ModelID");
  ASSERT_STREQ(versionId, "11223344");

  JSON_Array *inferences =
      json_object_get_array(output_tensor_object, "Inferences");
  JSON_Object *inference = json_array_get_object(inferences, 0);
  const char *inference_o = json_object_get_string(inference, "O");
  ASSERT_STREQ(
      inference_o,
      "UVNVV1k=");  // base64 encoded from {0x51, 0x53, 0x55, 0x57, 0x59}
  ASSERT_EQ(json_object_get_number(inference, "F"), 0);
  const char *deviceId =
      json_object_get_string(output_tensor_object, "DeviceID");
  ASSERT_STREQ(deviceId, "test_id");

  bool image = json_object_get_boolean(output_tensor_object, "Image");
  ASSERT_EQ(image, true);

  free(buffer);
  json_value_free(output_tensor_value);
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_SizeZero) {
  uint8_t in_data[1] = {0};
  uint32_t in_size = 0;  // size is zero.
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  JSON_Value *output_tensor_value = json_parse_string((char *)buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  const char *versionId =
      json_object_get_string(output_tensor_object, "ModelID");
  ASSERT_STREQ(versionId, "11223344");

  JSON_Array *inferences =
      json_object_get_array(output_tensor_object, "Inferences");
  JSON_Object *inference = json_array_get_object(inferences, 0);
  const char *inference_o = json_object_get_string(inference, "O");
  ASSERT_STREQ(inference_o, "");
  ASSERT_EQ(json_object_get_number(inference, "F"), 0);

  const char *deviceId =
      json_object_get_string(output_tensor_object, "DeviceID");
  ASSERT_STREQ(deviceId, "test_id");

  bool image = json_object_get_boolean(output_tensor_object, "Image");
  ASSERT_EQ(image, true);

  free(buffer);
  json_value_free(output_tensor_value);
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_JsonString) {
  const char *in_data =
      "[{\"class_id\":81,\"score\":0.5625,\"bbox\":{\"left\":14,\"top\":"
      "229,\"right\":290,\"bottom\":321}}]";

  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  // data type is json string.
  ProcessFormatResult result =
      ProcessFormatMeta((void *)in_data, in_size, EdgeAppLibSendDataJson,
                        time_stamp, buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  JSON_Value *output_tensor_value = json_parse_string((char *)buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  const char *versionId =
      json_object_get_string(output_tensor_object, "ModelID");
  ASSERT_STREQ(versionId, "11223344");

  JSON_Array *inferences =
      json_object_get_array(output_tensor_object, "Inferences");
  JSON_Object *inference = json_array_get_object(inferences, 0);
  JSON_Value *inference_o = json_object_get_value(inference, "O");
  char *o_str = json_serialize_to_string(inference_o);
  ASSERT_STREQ(o_str, in_data);  // the same as in_data
  json_free_serialized_string(o_str);
  ASSERT_EQ(json_object_get_number(inference, "F"), 1);

  const char *deviceId =
      json_object_get_string(output_tensor_object, "DeviceID");
  ASSERT_STREQ(deviceId, "test_id");

  bool image = json_object_get_boolean(output_tensor_object, "Image");
  ASSERT_EQ(image, true);

  json_value_free(output_tensor_value);
  free(buffer);
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_GetDeviceID_Error) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  // get device id will be failed.
  setEsfSystemGetDeviceIDFail();

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  JSON_Value *output_tensor_value = json_parse_string((char *)buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  const char *deviceId =
      json_object_get_string(output_tensor_object, "DeviceID");
  // "000000000000000" when get device id is failed.
  ASSERT_STREQ(deviceId, "000000000000000");

  json_value_free(output_tensor_value);
  free(buffer);
  resetEsfSystemGetDeviceIDSuccess();
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_InputTensorDisabled) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  // set input tensor disabled.
  setPortSettingsInputTensorDisabled();

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  JSON_Value *output_tensor_value = json_parse_string((char *)buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  bool image = json_object_get_boolean(output_tensor_object, "Image");
  // image is false.
  ASSERT_EQ(image, false);

  json_value_free(output_tensor_value);
  free(buffer);
  resetPortSettings();
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_NoInputTensor) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  // input tensor will not be exist.
  setPortSettingsNoInputTensor(2);

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  JSON_Value *output_tensor_value = json_parse_string((char *)buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  bool image = json_object_get_boolean(output_tensor_object, "Image");
  // image is false.
  ASSERT_EQ(image, false);

  json_value_free(output_tensor_value);
  free(buffer);
  resetPortSettings();
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_NoInputTensorEnabled) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  // input tensor's enabled will not exist.
  setPortSettingsNoInputTensorEnabled();

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  JSON_Value *output_tensor_value = json_parse_string((char *)buffer);
  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);
  bool image = json_object_get_boolean(output_tensor_object, "Image");
  // image is false.
  ASSERT_EQ(image, false);

  json_value_free(output_tensor_value);
  free(buffer);
  resetPortSettings();
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_InDataNull) {
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  // in_data is NULL.
  ProcessFormatResult result = ProcessFormatMeta(
      NULL, 0, EdgeAppLibSendDataBase64, time_stamp, buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  free(buffer);
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Normal_JsonInDataNull) {
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  // in_data is NULL.
  ProcessFormatResult result = ProcessFormatMeta(
      NULL, 0, EdgeAppLibSendDataJson, time_stamp, buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultOk);

  free(buffer);
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Error_StreamGetProperty) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  // stream get property will be failed.
  setEdgeAppLibSensorStreamGetPropertyFail();

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        buffer, buffer_size);
  free(buffer);
  ASSERT_EQ(result, kProcessFormatResultFailure);

  resetEdgeAppLibSensorStreamGetPropertySuccess();
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Error_DataType) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;

  // data type is invalid.
  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataType(2), time_stamp,
                        buffer, buffer_size);
  ASSERT_EQ(result, kProcessFormatResultInvalidParam);

  free(buffer);
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Error_verisonId) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  char *buffer = (char *)malloc(PREALLOCATED_BUFFER_SIZE);
  size_t buffer_size = PREALLOCATED_BUFFER_SIZE;
  ProcessFormatResult result =
      ProcessFormatMeta(in_data, in_size, EdgeAppLibSendDataBase64, time_stamp,
                        buffer, buffer_size);

  ASSERT_EQ(result, kProcessFormatResultFailure);

  free(buffer);
}

TEST_F(ProcessFormatTest, ProcessFormatMeta_Error_OutNull) {
  uint8_t in_data[5] = {0xa1, 0xa3, 0xa5, 0xa7, 0xa9};
  uint32_t in_size = sizeof(in_data);
  uint64_t time_stamp = 10000;
  int timeout_ms = 10000;

  StreamSetPropertyVersionID(AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION,
                             "11223344", "IMX500");

  // output_tensor_value is NULL.
  ProcessFormatResult result = ProcessFormatMeta(
      in_data, in_size, EdgeAppLibSendDataJson, time_stamp, NULL, 0);
  ASSERT_EQ(result, kProcessFormatResultInvalidParam);
}

TEST_F(ProcessFormatTest, ProcessFormatInputRawMap) {
  void *in_data = malloc(1024);
  MemoryRef data = {MEMORY_MANAGER_MAP_TYPE, in_data};
  int32_t image_size = 0;
  void *image = nullptr;

  auto result = ProcessFormatInput(data, 1024, kProcessFormatImageTypeRaw, 0,
                                   &image, &image_size);
  EXPECT_EQ(result, kProcessFormatResultOk);

  free(in_data);
  free(image);
}

TEST_F(ProcessFormatTest, ProcessFormatInputRawFileIO) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  int32_t image_size = 0;
  void *image = nullptr;

  auto result = ProcessFormatInput(data, 1024, kProcessFormatImageTypeRaw, 0,
                                   &image, &image_size);
  EXPECT_EQ(result, kProcessFormatResultOk);
  EXPECT_NE(image, nullptr);
  free(image);
}

TEST_F(ProcessFormatTest, ProcessFormatInputRawFileIO_PreadFail) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  int32_t image_size = 0;
  void *image = nullptr;
  setEsfMemoryManagerPreadFail();
  auto result = ProcessFormatInput(data, 1024, kProcessFormatImageTypeRaw, 0,
                                   &image, &image_size);
  EXPECT_EQ(result, kProcessFormatResultOther);
  resetEsfMemoryManagerPreadSuccess();
}

// Test for Invalid Input Parameters
TEST_F(ProcessFormatTest, ProcessFormatInputInvalidParams) {
  MemoryRef in_data = {0, nullptr};
  int32_t image_size = 0;
  void *image = nullptr;

  auto result = ProcessFormatInput(in_data, 1024, kProcessFormatImageTypeRaw, 0,
                                   &image, &image_size);
  EXPECT_EQ(result, kProcessFormatResultInvalidParam);
}

// Test for get property failure
TEST_F(ProcessFormatTest, ProcessFormatInputGetPropertyFail) {
  MemoryRef in_data = {0, nullptr};
  int32_t image_size = 0;
  void *image = nullptr;
  setEdgeAppLibSensorStreamGetPropertyFail();
  auto result = ProcessFormatInput(in_data, 1024, kProcessFormatImageTypeRaw, 0,
                                   &image, &image_size);
  EXPECT_EQ(result, kProcessFormatResultInvalidParam);
  resetEdgeAppLibSensorStreamGetPropertySuccess();
}

// Test for Unsupported Data Type
TEST_F(ProcessFormatTest, ProcessFormatInputInvalidType) {
  void *in_data = malloc(1024);
  MemoryRef data = {MEMORY_MANAGER_MAP_TYPE, in_data};
  void *image = nullptr;
  int32_t image_size = 0;

  auto result =
      ProcessFormatInput(data, 1024, static_cast<ProcessFormatImageType>(999),
                         0, &image, &image_size);
  EXPECT_EQ(result, kProcessFormatResultInvalidParam);

  free(in_data);
}

// Test for End-to-End Success Case
TEST_F(ProcessFormatTest, ProcessFormatInputWithMapped_RGB8_Planer) {
  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;

  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  void *in_data = malloc(property.height * property.stride_bytes * 3);
  void *image = nullptr;
  int32_t image_size = 0;
  MemoryRef data = {MEMORY_MANAGER_MAP_TYPE, in_data};

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));

  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 3,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOk);
  EXPECT_NE(image, nullptr);
  EXPECT_GT(image_size, 0);

  free(image);
  free(in_data);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithMapped_RGB24) {
  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300 * 3;

  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB24,
          sizeof(property.pixel_format));

  void *in_data = malloc(property.height * property.stride_bytes);
  void *image = nullptr;
  int32_t image_size = 0;
  MemoryRef data = {MEMORY_MANAGER_MAP_TYPE, in_data};

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));

  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOk);
  EXPECT_NE(image, nullptr);
  EXPECT_GT(image_size, 0);

  free(image);
  free(in_data);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithHandle_RGB24) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300 * 3;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB24,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOk);
  EXPECT_NE(image, nullptr);
  EXPECT_GT(image_size, 0);

  free(image);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithHandle_RGB_PLANER) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 3,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOk);
  EXPECT_NE(image, nullptr);
  EXPECT_GT(image_size, 0);

  free(image);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithMapped_UnsupportedFormat) {
  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300 * 3;

  strncpy(property.pixel_format, "test", sizeof(property.pixel_format));

  void *in_data = malloc(property.height * property.stride_bytes);
  void *image = nullptr;
  int32_t image_size = 0;
  MemoryRef data = {MEMORY_MANAGER_MAP_TYPE, in_data};

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));

  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultInvalidParam);
  EXPECT_EQ(image, nullptr);
  EXPECT_EQ(image_size, 0);

  if (image) free(image);
  free(in_data);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithHandle_invalideSize) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 1,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultMemoryError);
  EXPECT_EQ(image, nullptr);
  EXPECT_EQ(image_size, 0);

  free(image);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithHandle_JpegEncodefail) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  setEsfCodecJpegEncodeFail();
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 3,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOther);
  EXPECT_EQ(image, nullptr);
  resetEsfCodecJpegEncodeSuccess();
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithHandle_NullImage) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  auto result = ProcessFormatInput(data, 0, kProcessFormatImageTypeJpeg, 0,
                                   &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultMemoryError);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithHandle_JpegEncodeReleasefail) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  setEsfCodecJpegEncodeReleaseFail();
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 3,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOther);
  resetEsfCodecJpegEncodeReleaseSuccess();
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithMap_NullImage) {
  void *in_data = nullptr;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_MAP_TYPE;
  data.u.p = in_data;
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 3,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultInvalidParam);
  EXPECT_EQ(image, nullptr);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithMap_JpegEncodefail) {
  void *in_data = malloc(1024);
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_MAP_TYPE;
  data.u.p = in_data;
  setEsfCodecJpegEncodeFail();
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 3,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOther);
  EXPECT_EQ(image, nullptr);
  resetEsfCodecJpegEncodeSuccess();
  free(in_data);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithMap_WrongSize) {
  void *in_data = malloc(1024);
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_MAP_TYPE;
  data.u.p = in_data;
  auto result =
      ProcessFormatInput(data, property.height, kProcessFormatImageTypeJpeg, 0,
                         &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultMemoryError);
  EXPECT_EQ(image, nullptr);
  resetEsfCodecJpegEncodeSuccess();
  free(in_data);
}

TEST_F(ProcessFormatTest, ProcessFormatInputWithFileIO_PreadFail) {
  EsfMemoryManagerHandle in_data = (EsfMemoryManagerHandle)0x20000000;
  void *image = nullptr;
  int32_t image_size = 0;

  EdgeAppLibSensorImageProperty property = {};
  property.height = 300;
  property.width = 300;
  property.stride_bytes = 300;
  strncpy(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
          sizeof(property.pixel_format));

  SensorStreamSetProperty(GetSensorStream(), AITRIOS_SENSOR_IMAGE_PROPERTY_KEY,
                          &property, sizeof(property));
  MemoryRef data;
  data.type = MEMORY_MANAGER_FILE_TYPE;
  data.u.esf_handle = in_data;
  setEsfMemoryManagerPreadFail();
  auto result =
      ProcessFormatInput(data, property.height * property.stride_bytes * 3,
                         kProcessFormatImageTypeJpeg, 0, &image, &image_size);

  EXPECT_EQ(result, kProcessFormatResultOther);
  resetEsfMemoryManagerPreadSuccess();
}
