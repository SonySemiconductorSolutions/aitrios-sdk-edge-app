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

#ifndef _AITRIOS_SENSOR_UNIT_TEST_H_
#define _AITRIOS_SENSOR_UNIT_TEST_H_

#include "gmock/gmock.h"
#include "gtest/gtest.h"
// #include "parson/parson.h"

#include "edge_app/senscord.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;

extern int8_t mapped_flag;

namespace aitrios_sensor_ut {

class EdgeAppLibSensorUnitTestMock;

class EdgeAppLibSensorUnitTest : public ::testing::Test {
 public:
  static void SetUpTestCase();
  static void TearDownTestCase();
  virtual void SetUp();
  virtual void TearDown();

  static EdgeAppLibSensorUnitTestMock *mock_;
  static std::string pq_settings_;
  static std::string pq_image_settings_;
  static std::string streaming_settings_;
  static uint64_t pq_settings_count_;
  static uint64_t pq_image_settings_count_;
  static uint64_t streaming_settings_count_;
};

#ifdef DEBUG_TRACE
void print_stack() {
  void *stack[128];
  int len = backtrace(stack, sizeof(stack) / sizeof(stack[0]));
  char **strings = backtrace_symbols(stack, len);
  if (strings) {
    size_t size = 512;
    char *demanged = (char *)malloc(size);
    if (demanged) {
      for (int i = 0; i < len; ++i) {
        char *top = index(strings[i], '(');
        char *end = index(top, '+');
        if ((end - top) > 1) {
          *top = '\0';
          *end = '\0';
          char *symbol = top + 1;
          char *result = abi::__cxa_demangle(symbol, demanged, &size, nullptr);
          if (result) {
            demanged = result;
            symbol = demanged;
          }
          printf("%s(%s+%s\n", strings[i], symbol, end + 1);
        } else {
          printf("%s", strings[i]);
        }
      }
      free(demanged);
    }
    free(strings);
  }
}
#endif

}  // namespace aitrios_sensor_ut

#endif  // _AITRIOS_SENSOR_UNIT_TEST_H_
