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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "data_export.h"
#include "send_data.h"
#include "sensor.h"
#include "sm.h"
#include "switch_dnn_analyzer.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

static const float kOutputTensorOd[] = {
    // y_min
    0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // x_min
    0.15, 0.25, 0.35, 0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // y_max
    0.5, 0.6, 0.7, 0.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // x_max
    0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // class
    15, 132, 15, 15, 0, 0, 0, 0, 0, 0,
    // score
    0.8, 0.2, 0.8, 0.3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // num of detection
    10};

static const float kOutputTensorIc[] = {
    // score
    0.10, 0.81, 0.32, 0.63, 0.54};

static bool is_od = true;
static bool is_od_analyze_ok = true;
static bool is_ic_analyze_ok = true;
static bool is_od_serialize_ok = true;
static bool is_od_analyze_normal = true;
static bool is_od_analyze_detected = true;

class SwitchDnnTestMock {
 public:
  /* sensor.h */
  MOCK_METHOD(int32_t, SensorCoreInit, (EdgeAppLibSensorCore * core));
  MOCK_METHOD(int32_t, SensorCoreExit, (EdgeAppLibSensorCore core));
  MOCK_METHOD(int32_t, SensorCoreOpenStream,
              (EdgeAppLibSensorCore core, const char *stream_key,
               EdgeAppLibSensorStream *stream));
  MOCK_METHOD(int32_t, SensorCoreCloseStream,
              (EdgeAppLibSensorCore core, EdgeAppLibSensorStream stream));
  MOCK_METHOD(int32_t, SensorStart, (EdgeAppLibSensorStream stream));
  MOCK_METHOD(int32_t, SensorStop, (EdgeAppLibSensorStream stream));
  MOCK_METHOD(int32_t, SensorGetFrame,
              (EdgeAppLibSensorStream stream, EdgeAppLibSensorFrame *frame,
               int32_t timeout_msec));
  MOCK_METHOD(int32_t, SensorReleaseFrame,
              (EdgeAppLibSensorStream stream, EdgeAppLibSensorFrame frame));
  MOCK_METHOD(int32_t, SensorFrameGetChannelFromChannelId,
              (EdgeAppLibSensorFrame frame, uint32_t channel_id,
               EdgeAppLibSensorChannel *channel));
  MOCK_METHOD(int32_t, SensorChannelGetRawData,
              (EdgeAppLibSensorChannel channel,
               struct EdgeAppLibSensorRawData *raw_data));
  MOCK_METHOD(int32_t, SensorStreamSetProperty,
              (EdgeAppLibSensorStream stream, const char *property_key,
               const void *value, size_t value_size));
  MOCK_METHOD(int32_t, SensorStreamGetProperty,
              (EdgeAppLibSensorStream stream, const char *property_key,
               void *value, size_t value_size));
  MOCK_METHOD(int32_t, SensorChannelGetProperty,
              (EdgeAppLibSensorChannel channel, const char *property_key,
               void *value, size_t value_size));
  MOCK_METHOD(enum EdgeAppLibSensorErrorCause, SensorGetLastErrorCause, ());
  MOCK_METHOD(int32_t, SensorGetLastErrorString,
              (enum EdgeAppLibSensorStatusParam param, char *buffer,
               uint32_t *length));
  MOCK_METHOD(enum EdgeAppLibSensorErrorLevel, SensorGetLastErrorLevel, ());

  /* data_export.h */
  MOCK_METHOD(EdgeAppLibDataExportFuture *, DataExportSendData,
              (char *portname, EdgeAppLibDataExportDataType datatype,
               void *data, int datalen, uint64_t timestamp));
  MOCK_METHOD(EdgeAppLibDataExportResult, DataExportAwait,
              (EdgeAppLibDataExportFuture * future, int timeout_ms));
  MOCK_METHOD(EdgeAppLibDataExportResult, DataExportCleanup,
              (EdgeAppLibDataExportFuture * future));
  MOCK_METHOD(EdgeAppLibDataExportResult, DataExportSendState,
              (const char *topic, void *state, int statelen));

  /* send_data.h */
  MOCK_METHOD(EdgeAppLibSendDataResult, SendDataSyncMeta,
              (void *data, int datalen, EdgeAppLibSendDataType datatype,
               uint64_t timestamp, int timeout_ms));
};

class AnalyzerOdMock : public AnalyzerOd {
 public:
  MOCK_METHOD(AnalyzerBase::ResultCode, GetAnalyzedData,
              (DetectionData & data));
  MOCK_METHOD(AnalyzerBase::ResultCode, GetInputTensorSize,
              (uint16_t & width, uint16_t &height));
  MOCK_METHOD(AnalyzerBase::ResultCode, ValidateParam, (const void *param));
  MOCK_METHOD(AnalyzerBase::ResultCode, SetValidatedParam, (const void *param));
  MOCK_METHOD(AnalyzerBase::ResultCode, ClearValidatingParam, ());
  MOCK_METHOD(AnalyzerBase::ResultCode, GetParam, (PPLParam & param));
};

class AnalyzerIcMock : public AnalyzerIc {
 public:
  MOCK_METHOD(AnalyzerBase::ResultCode, GetAnalyzedData,
              (AnalyzerIc::ClassificationData & data));
  MOCK_METHOD(AnalyzerBase::ResultCode, GetInputTensorSize,
              (uint16_t & width, uint16_t &height));
  MOCK_METHOD(AnalyzerBase::ResultCode, ValidateParam, (const void *param));
  MOCK_METHOD(AnalyzerBase::ResultCode, SetValidatedParam, (const void *param));
  MOCK_METHOD(AnalyzerBase::ResultCode, ClearValidatingParam, ());
  MOCK_METHOD(AnalyzerBase::ResultCode, GetParam, (PPLParam & param));
};

class SwitchDnnTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_ = new ::testing::NiceMock<SwitchDnnTestMock>();
    mock_od_ = new ::testing::NiceMock<AnalyzerOdMock>();
    mock_ic_ = new ::testing::NiceMock<AnalyzerIcMock>();
  }
  void TearDown() override {
    delete mock_;
    delete mock_od_;
    delete mock_ic_;
    is_od = true;
    is_od_analyze_ok = true;
    is_ic_analyze_ok = true;
    is_od_serialize_ok = true;
    is_od_analyze_normal = true;
    is_od_analyze_detected = true;
  }

 public:
  static int32_t StubOdSensorStreamGetProperty(EdgeAppLibSensorStream stream,
                                               const char *property_key,
                                               void *value, size_t value_size);
  static int32_t StubIcSensorStreamGetProperty(EdgeAppLibSensorStream stream,
                                               const char *property_key,
                                               void *value, size_t value_size);
  static int32_t StubOdSensorChannelGetProperty(EdgeAppLibSensorChannel channel,
                                                const char *property_key,
                                                void *value, size_t value_size);
  static int32_t StubOdSensorChannelGetPropertyCrop1(
      EdgeAppLibSensorChannel channel, const char *property_key, void *value,
      size_t value_size);
  static int32_t StubOdSensorChannelGetPropertyCrop2(
      EdgeAppLibSensorChannel channel, const char *property_key, void *value,
      size_t value_size);
  static int32_t StubOdSensorChannelGetPropertyCrop3(
      EdgeAppLibSensorChannel channel, const char *property_key, void *value,
      size_t value_size);
  static int32_t StubOdSensorChannelGetPropertyCrop4(
      EdgeAppLibSensorChannel channel, const char *property_key, void *value,
      size_t value_size);
  static int32_t StubIcSensorChannelGetProperty(EdgeAppLibSensorChannel channel,
                                                const char *property_key,
                                                void *value, size_t value_size);
  static int32_t StubOdSensorChannelGetRawData(
      EdgeAppLibSensorChannel channel,
      struct EdgeAppLibSensorRawData *raw_data);
  static int32_t StubIcSensorChannelGetRawData(
      EdgeAppLibSensorChannel channel,
      struct EdgeAppLibSensorRawData *raw_data);
  static EdgeAppLibDataExportResult StubDataExportSendStateOK(const char *topic,
                                                              void *state,
                                                              int statelen);
  static EdgeAppLibDataExportResult StubDataExportSendStateNG(const char *topic,
                                                              void *state,
                                                              int statelen);
  void ExpectCallForOnIterate();
  static SwitchDnnTestMock *mock_;
  static AnalyzerOdMock *mock_od_;
  static AnalyzerIcMock *mock_ic_;
};
SwitchDnnTestMock *SwitchDnnTest::mock_ = nullptr;
AnalyzerOdMock *SwitchDnnTest::mock_od_ = nullptr;
AnalyzerIcMock *SwitchDnnTest::mock_ic_ = nullptr;

int32_t SwitchDnnTest::StubOdSensorStreamGetProperty(
    EdgeAppLibSensorStream stream, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    EdgeAppLibSensorImageCropProperty *crop =
        reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
    crop->left = 0;
    crop->top = 0;
    crop->width = 4056;
    crop->height = 3040;
  } else if (0 == strcmp(property_key,
                         AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY)) {
    EdgeAppLibSensorAiModelBundleIdProperty *id =
        reinterpret_cast<EdgeAppLibSensorAiModelBundleIdProperty *>(value);
    memset(id->ai_model_bundle_id, 0, AI_MODEL_BUNDLE_ID_SIZE);
    sprintf(id->ai_model_bundle_id, "000001");
  } else if (0 == strcmp(property_key,
                         AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraImageSizeProperty *size =
        reinterpret_cast<EdgeAppLibSensorCameraImageSizeProperty *>(value);
    size->width = 4056;
    size->height = 3040;
  }
  return 0;
}
int32_t SwitchDnnTest::StubIcSensorStreamGetProperty(
    EdgeAppLibSensorStream stream, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    EdgeAppLibSensorImageCropProperty *crop =
        reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
    crop->left = 572;
    crop->top = 290;
    crop->width = 1560;
    crop->height = 1200;
  } else if (0 == strcmp(property_key, "ai_model_bundle_id_property")) {
    EdgeAppLibSensorAiModelBundleIdProperty *id =
        reinterpret_cast<EdgeAppLibSensorAiModelBundleIdProperty *>(value);
    memset(id->ai_model_bundle_id, 0, AI_MODEL_BUNDLE_ID_SIZE);
    sprintf(id->ai_model_bundle_id, "000002");
    printf("StubIcSensorStreamGetProperty %s\n", id->ai_model_bundle_id);
  }
  return 0;
}
int32_t SwitchDnnTest::StubOdSensorChannelGetProperty(
    EdgeAppLibSensorChannel channel, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    crop->left = 0;
    crop->top = 0;
    crop->width = 4056;
    crop->height = 3040;
  } else if (0 == strcmp(property_key, "ai_model_bundle_id_property")) {
    EdgeAppLibSensorAiModelBundleIdProperty *id =
        reinterpret_cast<EdgeAppLibSensorAiModelBundleIdProperty *>(value);
    sprintf(id->ai_model_bundle_id, "000001");
  }
  return 0;
}
int32_t SwitchDnnTest::StubOdSensorChannelGetPropertyCrop1(
    EdgeAppLibSensorChannel channel, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    crop->left = 0 + 1; /* invalid */
    crop->top = 0;
    crop->width = 4056;
    crop->height = 3040;
  }
  return 0;
}
int32_t SwitchDnnTest::StubOdSensorChannelGetPropertyCrop2(
    EdgeAppLibSensorChannel channel, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    crop->left = 0;
    crop->top = 0 + 1; /* invalid */
    crop->width = 4056;
    crop->height = 3040;
  }
  return 0;
}
int32_t SwitchDnnTest::StubOdSensorChannelGetPropertyCrop3(
    EdgeAppLibSensorChannel channel, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    crop->left = 0;
    crop->top = 0;
    crop->width = 4056 + 1; /* invalid */
    crop->height = 3040;
  }
  return 0;
}
int32_t SwitchDnnTest::StubOdSensorChannelGetPropertyCrop4(
    EdgeAppLibSensorChannel channel, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    crop->left = 0;
    crop->top = 0;
    crop->width = 4056;
    crop->height = 3040 + 1; /* invalid */
  }
  return 0;
}
int32_t SwitchDnnTest::StubIcSensorChannelGetProperty(
    EdgeAppLibSensorChannel channel, const char *property_key, void *value,
    size_t value_size) {
  EdgeAppLibSensorImageCropProperty *crop =
      reinterpret_cast<EdgeAppLibSensorImageCropProperty *>(value);
  if (0 == strcmp(property_key, "image_crop_property")) {
    crop->left = 572;
    crop->top = 290;
    crop->width = 1560;
    crop->height = 1200;
  } else if (0 == strcmp(property_key, "ai_model_bundle_id_property")) {
    EdgeAppLibSensorAiModelBundleIdProperty *id =
        reinterpret_cast<EdgeAppLibSensorAiModelBundleIdProperty *>(value);
    sprintf(id->ai_model_bundle_id, "000002");
  }
  return 0;
}
int32_t SwitchDnnTest::StubOdSensorChannelGetRawData(
    EdgeAppLibSensorChannel channel, struct EdgeAppLibSensorRawData *raw_data) {
  raw_data->address = (void *)&kOutputTensorOd;
  raw_data->size = sizeof(kOutputTensorOd);
  raw_data->type = (char *)"";
  raw_data->timestamp = 0;
  return 0;
}
int32_t SwitchDnnTest::StubIcSensorChannelGetRawData(
    EdgeAppLibSensorChannel channel, struct EdgeAppLibSensorRawData *raw_data) {
  raw_data->address = (void *)&kOutputTensorIc;
  raw_data->size = sizeof(kOutputTensorIc);
  raw_data->type = (char *)"";
  raw_data->timestamp = 0;
  return 0;
}
EdgeAppLibDataExportResult SwitchDnnTest::StubDataExportSendStateOK(
    const char *topic, void *state, int statelen) {
  free(state);
  return EdgeAppLibDataExportResultSuccess;
}
EdgeAppLibDataExportResult SwitchDnnTest::StubDataExportSendStateNG(
    const char *topic, void *state, int statelen) {
  return EdgeAppLibDataExportResultFailure;
}
void SwitchDnnTest::ExpectCallForOnIterate() {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0))   // image
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0));  // image
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))  // meta
      .WillOnce(Return(0))                              // image
      .WillOnce(Invoke(StubIcSensorChannelGetRawData))  // meta
      .WillOnce(Return(0));                             // image
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))   // stream camera size
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))   // stream crop => (OD)
      .WillOnce(Invoke(StubIcSensorStreamGetProperty));  // stream crop => (IC)
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(
          Invoke(StubOdSensorChannelGetProperty))  // channel network_id => 1
      .WillOnce(
          Invoke(StubIcSensorChannelGetProperty));  // channel network_id => 2
}

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif

/* Mock Function:SensorCoreInit */
int32_t SensorCoreInit(EdgeAppLibSensorCore *core) {
  return SwitchDnnTest::mock_->SensorCoreInit(core);
}
/* Mock Function:SensorCoreExit */
int32_t SensorCoreExit(EdgeAppLibSensorCore core) {
  return SwitchDnnTest::mock_->SensorCoreExit(core);
}
/* Mock Function:SensorCoreOpenStream */
int32_t SensorCoreOpenStream(EdgeAppLibSensorCore core, const char *stream_key,
                             EdgeAppLibSensorStream *stream) {
  return SwitchDnnTest::mock_->SensorCoreOpenStream(core, stream_key, stream);
}
/* Mock Function:SensorCoreCloseStream */
int32_t SensorCoreCloseStream(EdgeAppLibSensorCore core,
                              EdgeAppLibSensorStream stream) {
  return SwitchDnnTest::mock_->SensorCoreCloseStream(core, stream);
}
/* Mock Function:SensorStart */
int32_t SensorStart(EdgeAppLibSensorStream stream) {
  return SwitchDnnTest::mock_->SensorStart(stream);
}
/* Mock Function:SensorStop */
int32_t SensorStop(EdgeAppLibSensorStream stream) {
  return SwitchDnnTest::mock_->SensorStop(stream);
}
/* Mock Function:SensorGetFrame */
int32_t SensorGetFrame(EdgeAppLibSensorStream stream,
                       EdgeAppLibSensorFrame *frame, int32_t timeout_msec) {
  return SwitchDnnTest::mock_->SensorGetFrame(stream, frame, timeout_msec);
}
/* Mock Function:SensorReleaseFrame */
int32_t SensorReleaseFrame(EdgeAppLibSensorStream stream,
                           EdgeAppLibSensorFrame frame) {
  return SwitchDnnTest::mock_->SensorReleaseFrame(stream, frame);
}
/* Mock Function:SensorFrameGetChannelFromChannelId */
int32_t SensorFrameGetChannelFromChannelId(EdgeAppLibSensorFrame frame,
                                           uint32_t channel_id,
                                           EdgeAppLibSensorChannel *channel) {
  return SwitchDnnTest::mock_->SensorFrameGetChannelFromChannelId(
      frame, channel_id, channel);
}
/* Mock Function:SensorChannelGetRawData */
int32_t SensorChannelGetRawData(EdgeAppLibSensorChannel channel,
                                struct EdgeAppLibSensorRawData *raw_data) {
  return SwitchDnnTest::mock_->SensorChannelGetRawData(channel, raw_data);
}
/* Mock Function:SensorStreamSetProperty */
int32_t SensorStreamSetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, const void *value,
                                size_t value_size) {
  return SwitchDnnTest::mock_->SensorStreamSetProperty(stream, property_key,
                                                       value, value_size);
}
/* Mock Function:SensorStreamGetProperty */
int32_t SensorStreamGetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, void *value,
                                size_t value_size) {
  return SwitchDnnTest::mock_->SensorStreamGetProperty(stream, property_key,
                                                       value, value_size);
}
/* Mock Function:SensorChannelGetProperty */
int32_t SensorChannelGetProperty(EdgeAppLibSensorChannel channel,
                                 const char *property_key, void *value,
                                 size_t value_size) {
  return SwitchDnnTest::mock_->SensorChannelGetProperty(channel, property_key,
                                                        value, value_size);
}
/* Mock Function:SensorGetLastErrorCause */
enum EdgeAppLibSensorErrorCause SensorGetLastErrorCause(void) {
  return SwitchDnnTest::mock_->SensorGetLastErrorCause();
}
/* Mock Function:SensorGetLastErrorString */
int32_t SensorGetLastErrorString(enum EdgeAppLibSensorStatusParam param,
                                 char *buffer, uint32_t *length) {
  return SwitchDnnTest::mock_->SensorGetLastErrorString(param, buffer, length);
}
/* Mock Function:SensorGetLastErrorLevel */
enum EdgeAppLibSensorErrorLevel SensorGetLastErrorLevel() {
  return SwitchDnnTest::mock_->SensorGetLastErrorLevel();
}
/* Mock Function:DataExportSendData */
EdgeAppLibDataExportFuture *DataExportSendData(
    char *portname, EdgeAppLibDataExportDataType datatype, void *data,
    int datalen, uint64_t timestamp, uint32_t current, uint32_t division) {
  static uint8_t dummy;
  EdgeAppLibDataExportFuture *future =
      reinterpret_cast<EdgeAppLibDataExportFuture *>(&dummy);
  return future;
}
/* Mock Function:DataExportAwait */
EdgeAppLibDataExportResult DataExportAwait(EdgeAppLibDataExportFuture *future,
                                           int timeout_ms) {
  return SwitchDnnTest::mock_->DataExportAwait(future, timeout_ms);
}
/* Mock Function:DataExportCleanup */
EdgeAppLibDataExportResult DataExportCleanup(
    EdgeAppLibDataExportFuture *future) {
  return SwitchDnnTest::mock_->DataExportCleanup(future);
}
/* Mock Function:DataExportSendState */
EdgeAppLibDataExportResult DataExportSendState(const char *topic, void *state,
                                               int statelen) {
  return SwitchDnnTest::mock_->DataExportSendState(topic, state, statelen);
}
/* Mock Function:SendDataSyncMeta */
EdgeAppLibSendDataResult SendDataSyncMeta(void *data, int datalen,
                                          EdgeAppLibSendDataType datatype,
                                          uint64_t timestamp, int timeout_ms) {
  return SwitchDnnTest::mock_->SendDataSyncMeta(data, datalen, datatype,
                                                timestamp, timeout_ms);
}

#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib

/**
 * Mock Class:AnalyzerBase
 */
AnalyzerBase::AnalyzerBase() {}
AnalyzerBase::~AnalyzerBase(){};

/**
 * Mock Class:AnalyzerOd
 */
AnalyzerBase::ResultCode AnalyzerOd::ValidateParam(const void *param) {
  char network_id[AI_MODEL_BUNDLE_ID_SIZE];
  sprintf(network_id, "000001");
  SetNetworkId(network_id);
  return SwitchDnnTest::mock_od_->ValidateParam(param);
}
AnalyzerBase::ResultCode AnalyzerOd::SetValidatedParam(const void *param) {
  return SwitchDnnTest::mock_od_->SetValidatedParam(param);
}
AnalyzerBase::ResultCode AnalyzerOd::ClearValidatingParam() {
  DoClearValidatingParam();
  return SwitchDnnTest::mock_od_->ClearValidatingParam();
}
void AnalyzerOd::DoClearValidatingParam() {}
AnalyzerBase::ResultCode AnalyzerOd::GetParam(PPLParam &param) const {
  return SwitchDnnTest::mock_od_->GetParam(param);
}
AnalyzerBase::ResultCode AnalyzerOd::Analyze(const float *p_data, uint32_t size,
                                             uint64_t trace_id) {
  if (p_data == nullptr || is_od_analyze_ok == false) {
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  const uint8_t kNumOfDetection = 10;
  data_.num_of_detections_ = kNumOfDetection;
  data_.v_bbox_.assign(kNumOfDetection, {0, 0, 0, 0});
  data_.v_classes_.assign(kNumOfDetection, 0);
  data_.v_scores_.assign(kNumOfDetection, 0);
  data_.v_is_used_for_cropping_.assign(kNumOfDetection, false);
  int index = 0;
  for (uint8_t i = 0; i < kNumOfDetection; ++i) {
    data_.v_bbox_[i].m_ymin_ = p_data[index++] * 299;
  }
  for (uint8_t i = 0; i < kNumOfDetection; ++i) {
    data_.v_bbox_[i].m_xmin_ = p_data[index++] * 299;
  }
  for (uint8_t i = 0; i < kNumOfDetection; ++i) {
    data_.v_bbox_[i].m_ymax_ = p_data[index++] * 299;
  }
  for (uint8_t i = 0; i < kNumOfDetection; ++i) {
    data_.v_bbox_[i].m_xmax_ = p_data[index++] * 299;
  }
  for (uint8_t i = 0; i < kNumOfDetection; ++i) {
    data_.v_classes_[i] = p_data[index++];
  }
  for (uint8_t i = 0; i < kNumOfDetection; ++i) {
    data_.v_scores_[i] = p_data[index++];
  }
  if (is_od_analyze_normal) {
    data_.v_is_used_for_cropping_[0] = true;
  }
  if (!is_od_analyze_detected) {
    data_.num_of_detections_ = 0;
  }
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerOd::Serialize(void **out_buf, uint32_t *size,
                                               const Allocator *allocator) {
  if (is_od_serialize_ok == false) {
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  /* allocator test */
  uint8_t *tmp = (uint8_t *)allocator->Malloc(1);
  if (tmp) {
    allocator->Free(tmp);
  }
  DetectionData detection_data;
  AnalyzerOd::GetAnalyzedData(detection_data);
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerOd::GetNetworkId(
    char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const {
  sprintf(network_id, "000001");
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerOd::GetAnalyzedData(
    DetectionData &data) const {
  data = data_;
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerOd::GetInputTensorSize(
    uint16_t &width, uint16_t &height) const {
  width = 300;
  height = 300;
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerOd::SetNetworkId(const char *network_id) {
  strncpy(network_id_, network_id, strlen(network_id));
  return AnalyzerBase::ResultCode::kOk;
}
/**
 * Mock Class:AnalyzerIc
 */
AnalyzerBase::ResultCode AnalyzerIc::ValidateParam(const void *param) {
  char network_id[AI_MODEL_BUNDLE_ID_SIZE];
  sprintf(network_id, "000002");
  SetNetworkId(network_id);
  return SwitchDnnTest::mock_ic_->ValidateParam(param);
}
AnalyzerBase::ResultCode AnalyzerIc::SetValidatedParam(const void *param) {
  return SwitchDnnTest::mock_ic_->SetValidatedParam(param);
}
AnalyzerBase::ResultCode AnalyzerIc::ClearValidatingParam() {
  DoClearValidatingParam();
  return SwitchDnnTest::mock_ic_->ClearValidatingParam();
}
void AnalyzerIc::DoClearValidatingParam() {}
AnalyzerBase::ResultCode AnalyzerIc::GetParam(PPLParam &param) const {
  return SwitchDnnTest::mock_ic_->GetParam(param);
}
AnalyzerBase::ResultCode AnalyzerIc::Analyze(const float *p_data, uint32_t size,
                                             uint64_t trace_id) {
  if (is_ic_analyze_ok == false) {
    return AnalyzerBase::ResultCode::kInvalidParam;
  }
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerIc::Serialize(void **out_buf, uint32_t *size,
                                               const Allocator *allocator) {
  ClassificationData classification_data;
  AnalyzerIc::GetAnalyzedData(classification_data);
  return AnalyzerBase::ResultCode::kOk;
}

AnalyzerBase::ResultCode AnalyzerIc::GetNetworkId(
    char (&network_id)[AI_MODEL_BUNDLE_ID_SIZE]) const {
  sprintf(network_id, "000002");
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerIc::GetAnalyzedData(
    AnalyzerIc::ClassificationData &data) const {
  return AnalyzerBase::ResultCode::kOk;
}
AnalyzerBase::ResultCode AnalyzerIc::SetNetworkId(const char *network_id) {
  strncpy(network_id_, network_id, strlen(network_id));
  return AnalyzerBase::ResultCode::kOk;
}

TEST_F(SwitchDnnTest, OnCreateSuccess) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorCoreInit(_)).WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorCoreOpenStream(_, _, _)).WillOnce(Return(0));
  int result = onCreate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, OnCreateFailureOnInit) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorCoreInit(_)).WillOnce(Return(-1));
  int result = onCreate();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, OnCreateFailureOnOpen) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorCoreInit(_)).WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorCoreOpenStream(_, _, _)).WillOnce(Return(-1));
  int result = onCreate();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureSuccess) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onConfigureNullTopic) {
  char *config = strdup("");
  int result = onConfigure(nullptr, config, 0);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureNullConfig) {
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  int result = onConfigure((char *)"", nullptr, 0);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureEmptyArgument) {
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup("");
  int result = onConfigure((char *)"", config, 0);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureErrorWithResId) {
  std::string text = R"({
    "res_info" : {
      "code" : 0,
      "detail_msg" : "",
      "res_id" : "id"
    }
  })";
  EXPECT_CALL(*mock_od_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kInvalidParam));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureValidateErrorOnOd) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  // Call Scenarios
  EXPECT_CALL(*mock_od_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kInvalidParam));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureSetParamErrorOnOd) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  // Call Scenarios
  EXPECT_CALL(*mock_od_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_od_, SetValidatedParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kInvalidParam));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureValidateErrorOnIc) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  // Call Scenarios
  EXPECT_CALL(*mock_od_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_od_, SetValidatedParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_ic_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kInvalidParam));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureForceSwitch) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300,
          "force_switch": 1
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  // Call Scenarios
  EXPECT_CALL(*mock_od_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_od_, SetValidatedParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_ic_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kInvalidParam));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureSetParamErrorOnIc) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  // Call Scenarios
  EXPECT_CALL(*mock_od_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_od_, SetValidatedParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_ic_, ValidateParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kOk));
  EXPECT_CALL(*mock_ic_, SetValidatedParam(_))
      .WillOnce(Return(AnalyzerBase::ResultCode::kInvalidParam));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureSendErrorStateError) {
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateNG));
  char *config = strdup("");
  int length = 0;
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onConfigureSendStateError) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateNG));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onConfigureSetNetworkError) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  EXPECT_CALL(*mock_, SensorStreamSetProperty(_, _, _, _)).WillOnce(Return(-1));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  char *config = strdup(text.c_str());
  int length = text.size();
  int result = onConfigure((char *)"", config, length);
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onStopSuccess) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorStop(_)).WillOnce(Return(0));
  int result = onStop();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onStopError) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorStop(_)).WillOnce(Return(-1));
  int result = onStop();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onStartSuccess) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorStart(_)).WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _)).WillOnce(Return(0));
  int result = onStart();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onStartError) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorStart(_)).WillOnce(Return(-1));
  int result = onStart();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onStartCropError) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorStart(_)).WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _)).WillOnce(Return(-1));
  int result = onStart();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onDestroySuccess) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorCoreCloseStream(_, _)).WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorCoreExit(_)).WillOnce(Return(0));
  int result = onDestroy();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onDestroyCloseError) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorCoreCloseStream(_, _)).WillOnce(Return(-1));
  int result = onDestroy();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onDestroyErrorExit) {
  // Call Scenarios
  EXPECT_CALL(*mock_, SensorCoreCloseStream(_, _)).WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorCoreExit(_)).WillOnce(Return(-1));
  int result = onDestroy();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onIterateSuccess) {
  ExpectCallForOnIterate();
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateReleseError) {
  ExpectCallForOnIterate();
  EXPECT_CALL(*mock_, SensorReleaseFrame(_, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateDnnChannelCheckError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Return(-1));  // get network id
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateDnnChannelCheckDifferentID) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));  // stream camera size
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubIcSensorChannelGetProperty));  // channel
                                                          // network_id => 2
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateCropStreamCheckError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))  // get camera size
      .WillOnce(Return(-1));                            // get crop
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));  // get network id
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateCropStreamGetError) {
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Return(-1));  // get camera size
  int result = onIterate();
  EXPECT_EQ(-1, result);
}

TEST_F(SwitchDnnTest, onIterateCropStreamCheckDifferentValue) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))   // get camera size
      .WillOnce(Invoke(StubIcSensorStreamGetProperty));  // get crop (IC:NG)
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));  // get network id
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateSendImageChannelError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))    // meta
      .WillOnce(Return(-1));  // image
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _)).WillOnce(Return(0));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateSendImageRawDataError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0));  // image
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _)).WillOnce(Return(-1));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _)).WillOnce(Return(0));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateWaitImageError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0));  // image
  EXPECT_CALL(*mock_, DataExportAwait(_, _))
      .WillOnce(Return(EdgeAppLibDataExportResultFailure));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _)).WillOnce(Return(0));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateSerializeError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0));  // image
  EXPECT_CALL(*mock_, DataExportAwait(_, _))
      .WillOnce(Return(EdgeAppLibDataExportResultFailure));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData));
  is_od_serialize_ok = false;
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateSendDataAwaitError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0));  // image
  EXPECT_CALL(*mock_, DataExportAwait(_, _))
      .WillOnce(Return(EdgeAppLibDataExportResultFailure));
  EXPECT_CALL(*mock_, SendDataSyncMeta(_, _, _, _, _))
      .WillOnce(Return(EdgeAppLibSendDataResultFailure));
  EXPECT_CALL(*mock_, DataExportSendState(_, _, _))
      .WillOnce(Invoke(StubDataExportSendStateOK));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateGetFrameTimedOut) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(-1))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorGetLastErrorCause())
      .WillOnce(Return(AITRIOS_SENSOR_ERROR_TIMEOUT))
      .WillOnce(Return(AITRIOS_SENSOR_ERROR_BUSY));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateGetChannelError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(-1))
      .WillOnce(Return(-1));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateGetRawDataError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Return(-1))
      .WillOnce(Return(-1));
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateOdAnalyzeError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData));
  is_od_analyze_ok = false;
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateOdAnalyzeNoDetection) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData));
  is_od_analyze_detected = false;
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateIcAnalyzeError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0))   // image
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0));  // image
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))  // meta
      .WillOnce(Return(0))                              // image
      .WillOnce(Invoke(StubIcSensorChannelGetRawData))  // meta
      .WillOnce(Return(0));                             // image
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))   // get camera size
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))   // stream crop => (OD)
      .WillOnce(Invoke(StubIcSensorStreamGetProperty));  // stream crop => (IC)
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(
          Invoke(StubOdSensorChannelGetProperty))  // channel network_id => 1
      .WillOnce(
          Invoke(StubIcSensorChannelGetProperty));  // channel network_id => 2
  is_ic_analyze_ok = false;
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateIcAnalyzeWithInvalidOdResult) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))   // meta
      .WillOnce(Return(0));  // image
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))  // meta
      .WillOnce(Return(0));                             // image
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))   // get camera size
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));  // stream crop => (OD)
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(
          Invoke(StubOdSensorChannelGetProperty));  // channel network_id => 1
  is_od_analyze_normal = false;
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateErrorBeforeSwitchDnn) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData));
  EXPECT_CALL(*mock_, SensorStreamSetProperty(_, _, _, _))
      .WillOnce(Return(-1));  // set crop

  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateSwitchDnnError) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(-1));  // break loop
  EXPECT_CALL(*mock_, SensorFrameGetChannelFromChannelId(_, _, _))
      .WillOnce(Return(0))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_, SensorStreamGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty))
      .WillOnce(Invoke(StubOdSensorStreamGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetProperty(_, _, _, _))
      .WillOnce(Invoke(StubOdSensorChannelGetProperty));
  EXPECT_CALL(*mock_, SensorChannelGetRawData(_, _))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData))
      .WillOnce(Invoke(StubOdSensorChannelGetRawData));
  EXPECT_CALL(*mock_, SensorStreamSetProperty(_, _, _, _))
      .WillOnce(Return(0))    // set crop
      .WillOnce(Return(-1));  // set network id
  int result = onIterate();
  EXPECT_EQ(0, result);
}

TEST_F(SwitchDnnTest, onIterateRetryOver) {
  EXPECT_CALL(*mock_, SensorGetFrame(_, _, _)).WillRepeatedly(Return(0));
  int result = onIterate();
  EXPECT_EQ(0, result);
}
