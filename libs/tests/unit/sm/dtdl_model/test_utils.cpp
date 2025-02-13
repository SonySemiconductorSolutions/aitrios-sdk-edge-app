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

#include "utils.hpp"

// Test case for IsAlmostEqual function
TEST(PropertiesTest, IsAlmostEqualTest) {
  ASSERT_TRUE(IsAlmostEqual(0.0, 0.0));
  ASSERT_TRUE(IsAlmostEqual(1.0, 1.0));
  ASSERT_TRUE(IsAlmostEqual(3.141592653, 3.14159265358979));

  ASSERT_FALSE(IsAlmostEqual(0.0, 1.0));
  ASSERT_FALSE(IsAlmostEqual(1.0, 2.0));
}

// Test case for IsInteger function
TEST(PropertiesTest, IsIntegerTest) {
  // Test cases with integer values
  ASSERT_TRUE(IsInteger(0.0));
  ASSERT_TRUE(IsInteger(1.0));
  ASSERT_TRUE(IsInteger(123.0));

  // Test cases with non-integer values
  ASSERT_FALSE(IsInteger(0.5));
  ASSERT_FALSE(IsInteger(1.234));
}

TEST(ErrorCodeTest, CodeFromSensorErrorCauseTest) {
  // Test cases with AITRIOS_SENSOR_ERROR_OUT_OF_RANGE values
  ASSERT_EQ(CodeFromSensorErrorCause(AITRIOS_SENSOR_ERROR_OUT_OF_RANGE),
            CODE_OUT_OF_RANGE);

  // Test cases with other than AITRIOS_SENSOR_ERROR_OUT_OF_RANGE values
  ASSERT_EQ(CodeFromSensorErrorCause(AITRIOS_SENSOR_ERROR_NOT_SUPPORTED),
            CODE_INVALID_ARGUMENT);
}
