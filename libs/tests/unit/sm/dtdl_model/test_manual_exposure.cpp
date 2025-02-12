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

#include <cstdlib>

#include "dtdl_model/objects/common_settings.hpp"
#include "dtdl_model/properties.h"
#include "parson.h"
#include "sensor.h"
#include "sensor/mock_sensor.hpp"
#include "sm_context.hpp"

#define TEST_INPUT "{\"exposure_time\": 8, \"gain\": 0.55}"
#define EXPOSURE_TIME "exposure_time"
#define GAIN "gain"

using namespace EdgeAppLib;

class ManualExposureParam : public ::testing::Test {
 public:
  void SetUp() override { context = StateMachineContext::GetInstance(nullptr); }
  void TearDown() override { context->Delete(); }

  StateMachineContext *context = nullptr;
};

TEST(ManualExposure, Parse) {
  ManualExposure obj;

  JSON_Value *value = json_parse_string(TEST_INPUT);
  ASSERT_EQ(obj.Verify(json_object(value)), 0);
  obj.Delete();
  json_value_free(value);
}

TEST_F(ManualExposureParam, CheckNotification) {
  JSON_Value *value1 = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value1);

  ManualExposure *obj = context->GetDtdlModel()
                            ->GetCommonSettings()
                            ->GetPqSettings()
                            ->GetManualExposure();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
  obj->Apply(obj1);
  ASSERT_FALSE(context->IsPendingNotification());

  EdgeAppLibSensorStream stream = context->GetSensorStream();
  EdgeAppLibSensorCameraManualExposureProperty manual_exposure;

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY,
                          &manual_exposure, sizeof(manual_exposure));

  ASSERT_EQ(manual_exposure.exposure_time, 8);
  ASSERT_LT(abs(manual_exposure.gain - (float)0.55), TOLERANCE);

  json_object_set_number(obj1, "exposure_time", 15);
  json_object_set_number(obj1, "gain", 0.7355);
  obj->Apply(obj1);

  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY,
                          &manual_exposure, sizeof(manual_exposure));

  ASSERT_EQ(manual_exposure.exposure_time, 15);
  ASSERT_LT(abs(manual_exposure.gain - (float)0.7355), TOLERANCE);

  json_object_set_number(obj1, "exposure_time", 14);
  json_object_set_number(obj1, "gain", 0.6354);
  setEdgeAppLibSensorStreamSetPropertyFail();
  obj->Apply(obj1);
  resetEdgeAppLibSensorStreamSetPropertySuccess();
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY,
                          &manual_exposure, sizeof(manual_exposure));
  ASSERT_EQ(manual_exposure.exposure_time, 15);
  ASSERT_LT(abs(manual_exposure.gain - (float)0.7355), TOLERANCE);
}

TEST_F(ManualExposureParam, VerifyFailExposureTime) {
  ManualExposure obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "exposure_time", -1);

  ASSERT_EQ(obj.Verify(json_object(value)), -1);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(),
               "exposure_time not >= 0.000000");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(),
            CODE_INVALID_ARGUMENT);

  json_value_free(value);
}

TEST_F(ManualExposureParam, VerifyFailGain) {
  ManualExposure obj;
  JSON_Value *value = json_parse_string(TEST_INPUT);
  JSON_Object *obj1 = json_object(value);
  json_object_set_number(obj1, "gain", -1);

  ASSERT_EQ(obj.Verify(json_object(value)), 0);

  ASSERT_STREQ(context->GetDtdlModel()->GetResInfo()->GetDetailMsg(), "");
  ASSERT_EQ(context->GetDtdlModel()->GetResInfo()->GetCode(), CODE_OK);

  json_value_free(value);
}

TEST(ManualExposure, InitializeValues) {
  ManualExposure obj;

  JSON_Object *json = obj.GetJsonObject();
  ASSERT_TRUE(json_object_has_value(json, EXPOSURE_TIME) == 0);
  ASSERT_TRUE(json_object_has_value(json, GAIN) == 0);
  obj.InitializeValues();
  ASSERT_TRUE(json_object_has_value(json, EXPOSURE_TIME) == 1);
  ASSERT_TRUE(json_object_has_value(json, GAIN) == 1);
  obj.Delete();
}
