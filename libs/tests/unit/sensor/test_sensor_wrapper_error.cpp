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

#include "sensor.h"
#include "sensor_unit_test.h"
#include "sensor_unit_test_mock.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;

namespace aitrios_sensor_ut {

int32_t stub_senscord_get_last_error_cause(senscord_status_t *status) {
  if (status) {
    status->level = SENSCORD_LEVEL_FAIL;
    status->message = "dummy message";
    status->block = "dummy block";
    status->cause = SENSCORD_ERROR_BUSY;
  }

  return 0;
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorGetLastErrorLevel_Normal_Success) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  EXPECT_CALL(*mock_, senscord_get_last_error_level())
      .WillRepeatedly(testing::Return(SENSCORD_LEVEL_FAIL));

  EdgeAppLibSensorErrorLevel error_level =
      EdgeAppLib::SensorGetLastErrorLevel();
  ASSERT_EQ(error_level, AITRIOS_SENSOR_LEVEL_FAIL);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorGetLastErrorCause_Normal_Success) {
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_CALL(*mock_, senscord_get_last_error_cause())
      .WillRepeatedly(testing::Return(SENSCORD_ERROR_BUSY));

  EdgeAppLibSensorErrorCause error_cause =
      EdgeAppLib::SensorGetLastErrorCause();
  ASSERT_EQ(error_cause, AITRIOS_SENSOR_ERROR_BUSY);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorGetLastErrorString_Normal_Success) {
  EXPECT_CALL(*mock_, senscord_get_last_error_string(_, _, _))
      .WillOnce(testing::Return(0));

  EdgeAppLibSensorStatusParam param = AITRIOS_SENSOR_STATUS_PARAM_MESSAGE;
  uint32_t buffer_length = 256;
  char *buffer = (char *)malloc(buffer_length);
  int32_t ret =
      EdgeAppLib::SensorGetLastErrorString(param, buffer, &buffer_length);
  ASSERT_EQ(ret, 0);
  free(buffer);
}

TEST_F(EdgeAppLibSensorUnitTest,
       EdgeAppLibSensorGetLastErrorString_SensCordError_Error) {
  EXPECT_CALL(*mock_, senscord_get_last_error_string(_, _, _))
      .WillOnce(testing::Return(-1));

  EdgeAppLibSensorStatusParam param = AITRIOS_SENSOR_STATUS_PARAM_MESSAGE;
  uint32_t buffer_length = 256;
  char *buffer = (char *)malloc(buffer_length);
  int32_t ret =
      EdgeAppLib::SensorGetLastErrorString(param, buffer, &buffer_length);
  ASSERT_EQ(ret, -1);
  free(buffer);
}
}  // namespace aitrios_sensor_ut
