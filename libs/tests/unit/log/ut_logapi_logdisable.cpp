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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "log.h"
#include "log_internal.h"
#include "log_private.h"
#include "ut_logapi_mock.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;

namespace logapi_unittest {

class LogAPIUnitTest : public ::testing::Test {
 public:
  LogAPIUnitTest();
  virtual ~LogAPIUnitTest();
  virtual void SetUp() {}
  virtual void TearDown() {}

  static std::unique_ptr<LogAPIUnitTestMock> _mock;
};

const char *context = "testcontext";
const char *message = "testmessage";

std::unique_ptr<LogAPIUnitTestMock> LogAPIUnitTest::_mock;

LogAPIUnitTest::LogAPIUnitTest() {
  _mock.reset(new ::testing::NiceMock<LogAPIUnitTestMock>());
}

LogAPIUnitTest::~LogAPIUnitTest() { _mock.reset(); }

TEST_F(LogAPIUnitTest, CheckGetLogLevel) {
  LogLevel level = GetLogLevel();
  EXPECT_TRUE(level == LogLevel::kWarnLevel);
}

TEST_F(LogAPIUnitTest, CheckSetLogLevel) {
  SetLogLevel(LogLevel::kErrorLevel);
  LogLevel setLevel = GetLogLevel();
  EXPECT_TRUE(setLevel == LogLevel::kErrorLevel);
}

TEST_F(LogAPIUnitTest, CheckNoLogTrace) {
  std::string expect_log = "";

  testing::internal::CaptureStdout();
  EdgeAppLibLogTrace(context, message);
  std::string result_log = testing::internal::GetCapturedStdout().c_str();

  EXPECT_STREQ(expect_log.c_str(), result_log.c_str());
}

TEST_F(LogAPIUnitTest, CheckNoLogDebug) {
  std::string expect_log = "";

  testing::internal::CaptureStdout();
  EdgeAppLibLogDebug(context, message);
  std::string result_log = testing::internal::GetCapturedStdout().c_str();

  EXPECT_STREQ(expect_log.c_str(), result_log.c_str());
}

TEST_F(LogAPIUnitTest, CheckNoLogInfo) {
  std::string expect_log = "";

  testing::internal::CaptureStdout();
  EdgeAppLibLogInfo(context, message);
  std::string result_log = testing::internal::GetCapturedStdout().c_str();

  EXPECT_STREQ(expect_log.c_str(), result_log.c_str());
}

TEST_F(LogAPIUnitTest, CheckNoLogWarn) {
  std::string expect_log = "";

  testing::internal::CaptureStdout();
  EdgeAppLibLogWarn(context, message);
  std::string result_log = testing::internal::GetCapturedStdout().c_str();

  EXPECT_STREQ(expect_log.c_str(), result_log.c_str());
}

TEST_F(LogAPIUnitTest, CheckNoLogError) {
  std::string expect_log = "";

  testing::internal::CaptureStdout();
  EdgeAppLibLogError(context, message);
  std::string result_log = testing::internal::GetCapturedStdout().c_str();

  EXPECT_STREQ(expect_log.c_str(), result_log.c_str());
}

TEST_F(LogAPIUnitTest, CheckNoLogCritical) {
  std::string expect_log = "";

  testing::internal::CaptureStdout();
  EdgeAppLibLogCritical(context, message);
  std::string result_log = testing::internal::GetCapturedStdout().c_str();

  EXPECT_STREQ(expect_log.c_str(), result_log.c_str());
}

};  // namespace logapi_unittest
