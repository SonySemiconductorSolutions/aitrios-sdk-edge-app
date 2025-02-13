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

#include "sensor_unit_test.h"

#include "sensor_unit_test_mock.h"

namespace aitrios_sensor_ut {

EdgeAppLibSensorUnitTestMock *EdgeAppLibSensorUnitTest::mock_ = nullptr;
std::string EdgeAppLibSensorUnitTest::pq_settings_;
std::string EdgeAppLibSensorUnitTest::pq_image_settings_;
std::string EdgeAppLibSensorUnitTest::streaming_settings_;
uint64_t EdgeAppLibSensorUnitTest::pq_settings_count_;
uint64_t EdgeAppLibSensorUnitTest::pq_image_settings_count_;
uint64_t EdgeAppLibSensorUnitTest::streaming_settings_count_;

void EdgeAppLibSensorUnitTest::SetUpTestCase() {}
void EdgeAppLibSensorUnitTest::TearDownTestCase() {}
void EdgeAppLibSensorUnitTest::SetUp() {
  mock_ = new ::testing::NiceMock<EdgeAppLibSensorUnitTestMock>();
}
void EdgeAppLibSensorUnitTest::TearDown() { delete mock_; }

}  // namespace aitrios_sensor_ut
