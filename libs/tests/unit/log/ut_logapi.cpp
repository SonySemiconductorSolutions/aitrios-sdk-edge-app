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

class LogAPIUnitTest
    : public ::testing::TestWithParam< ::std::tuple<LogLevel, std::string> > {
 public:
  LogAPIUnitTest();
  virtual ~LogAPIUnitTest();
  virtual void SetUp() {}
  virtual void TearDown() {}

  static std::unique_ptr<LogAPIUnitTestMock> _mock;

  void CheckEqual(std::string expect_log, std::string output);
};

class TestLogTrace : public LogAPIUnitTest {};
class TestLogDebug : public LogAPIUnitTest {};
class TestLogInfo : public LogAPIUnitTest {};
class TestLogWarn : public LogAPIUnitTest {};
class TestLogError : public LogAPIUnitTest {};
class TestLogCritical : public LogAPIUnitTest {};

const char *context = "testcontext";
const char *message = "testmessage";

std::unique_ptr<LogAPIUnitTestMock> LogAPIUnitTest::_mock;

LogAPIUnitTest::LogAPIUnitTest() {
  _mock.reset(new ::testing::NiceMock<LogAPIUnitTestMock>());
}

LogAPIUnitTest::~LogAPIUnitTest() { _mock.reset(); }

void LogAPIUnitTest::CheckEqual(std::string expect_log, std::string output) {
  if (expect_log == "") {
    EXPECT_STREQ(expect_log.c_str(), output.c_str());
  } else {
    std::string expect_str = expect_log + " " + context + " " + message + "\n";
    std::string result_timestamp = output.substr(0, 23);
    EXPECT_THAT(result_timestamp.c_str(),
                ::testing::MatchesRegex(
                    "^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])T([01]["
                    "0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9].[0-9]{3}"));

    std::string result_str = output.substr(24);
    EXPECT_STREQ(expect_str.c_str(), result_str.c_str());
  }
}

TEST_F(LogAPIUnitTest, PrintFailsetvbuf) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();

  EXPECT_CALL(*_mock, setvbuf(_, _, _, _)).WillRepeatedly(Return(-1));
  EdgeAppLibLogCritical(context, message);

  std::string error_log = testing::internal::GetCapturedStderr().c_str();
  EXPECT_STREQ("fail setvbuf\n", error_log.c_str());

  std::string output = testing::internal::GetCapturedStdout().c_str();

  CheckEqual("[CRITICAL]", output);
}

TEST_F(LogAPIUnitTest, CheckGetLogLevelAsDefault) {
  LogLevel defaultLevel = LogLevel::kWarnLevel;
  LogLevel actualLevel = GetLogLevel();
  EXPECT_TRUE(actualLevel == defaultLevel);
}

TEST_F(LogAPIUnitTest, CheckSetLogLevel) {
  LogLevel expectLevel = LogLevel::kErrorLevel;
  SetLogLevel(expectLevel);
  LogLevel actualLevel = GetLogLevel();
  EXPECT_TRUE(actualLevel == expectLevel);
}

TEST_P(TestLogTrace, CheckLogTrace) {
  ::std::tuple<LogLevel, std::string> param = GetParam();
  LogLevel settingLevel = ::std::get<0>(param);
  std::string expect_str = ::std::get<1>(param);

  SetLogLevel(settingLevel);

  testing::internal::CaptureStdout();
  EdgeAppLibLogTrace(context, message);
  std::string output = testing::internal::GetCapturedStdout();

  CheckEqual(expect_str, output);
}

INSTANTIATE_TEST_CASE_P(
    InstantiateCheckLogTrace, TestLogTrace,
    ::testing::Values(::std::make_tuple(LogLevel::kTraceLevel, "[TRACE]   "),
                      ::std::make_tuple(LogLevel::kDebugLevel, ""),
                      ::std::make_tuple(LogLevel::kInfoLevel, ""),
                      ::std::make_tuple(LogLevel::kWarnLevel, ""),
                      ::std::make_tuple(LogLevel::kErrorLevel, ""),
                      ::std::make_tuple(LogLevel::kCriticalLevel, "")));

TEST_P(TestLogDebug, CheckLogDebug) {
  ::std::tuple<LogLevel, std::string> param = GetParam();
  LogLevel settingLevel = ::std::get<0>(param);
  std::string expect_str = ::std::get<1>(param);

  SetLogLevel(settingLevel);

  testing::internal::CaptureStdout();
  EdgeAppLibLogDebug(context, message);
  std::string output = testing::internal::GetCapturedStdout();

  CheckEqual(expect_str, output);
}

INSTANTIATE_TEST_CASE_P(
    InstantiateCheckLogDebug, TestLogDebug,
    ::testing::Values(::std::make_tuple(LogLevel::kTraceLevel, "[DEBUG]   "),
                      ::std::make_tuple(LogLevel::kDebugLevel, "[DEBUG]   "),
                      ::std::make_tuple(LogLevel::kInfoLevel, ""),
                      ::std::make_tuple(LogLevel::kWarnLevel, ""),
                      ::std::make_tuple(LogLevel::kErrorLevel, ""),
                      ::std::make_tuple(LogLevel::kCriticalLevel, "")));

TEST_P(TestLogInfo, CheckLogInfo) {
  ::std::tuple<LogLevel, std::string> param = GetParam();
  LogLevel settingLevel = ::std::get<0>(param);
  std::string expect_str = ::std::get<1>(param);

  SetLogLevel(settingLevel);

  testing::internal::CaptureStdout();
  EdgeAppLibLogInfo(context, message);
  std::string output = testing::internal::GetCapturedStdout();

  CheckEqual(expect_str, output);
}

INSTANTIATE_TEST_CASE_P(
    InstantiateCheckLogInfo, TestLogInfo,
    ::testing::Values(::std::make_tuple(LogLevel::kTraceLevel, "[INFO]    "),
                      ::std::make_tuple(LogLevel::kDebugLevel, "[INFO]    "),
                      ::std::make_tuple(LogLevel::kInfoLevel, "[INFO]    "),
                      ::std::make_tuple(LogLevel::kWarnLevel, ""),
                      ::std::make_tuple(LogLevel::kErrorLevel, ""),
                      ::std::make_tuple(LogLevel::kCriticalLevel, "")));

TEST_P(TestLogWarn, CheckLogWarn) {
  ::std::tuple<LogLevel, std::string> param = GetParam();
  LogLevel settingLevel = ::std::get<0>(param);
  std::string expect_str = ::std::get<1>(param);

  SetLogLevel(settingLevel);

  testing::internal::CaptureStdout();
  EdgeAppLibLogWarn(context, message);
  std::string output = testing::internal::GetCapturedStdout();

  CheckEqual(expect_str, output);
}

INSTANTIATE_TEST_CASE_P(
    InstantiateCheckLogWarn, TestLogWarn,
    ::testing::Values(::std::make_tuple(LogLevel::kTraceLevel, "[WARN]    "),
                      ::std::make_tuple(LogLevel::kDebugLevel, "[WARN]    "),
                      ::std::make_tuple(LogLevel::kInfoLevel, "[WARN]    "),
                      ::std::make_tuple(LogLevel::kWarnLevel, "[WARN]    "),
                      ::std::make_tuple(LogLevel::kErrorLevel, ""),
                      ::std::make_tuple(LogLevel::kCriticalLevel, "")));

TEST_P(TestLogError, CheckLogError) {
  ::std::tuple<LogLevel, std::string> param = GetParam();
  LogLevel settingLevel = ::std::get<0>(param);
  std::string expect_str = ::std::get<1>(param);

  SetLogLevel(settingLevel);

  testing::internal::CaptureStdout();
  EdgeAppLibLogError(context, message);
  std::string output = testing::internal::GetCapturedStdout();

  CheckEqual(expect_str, output);
}

INSTANTIATE_TEST_CASE_P(
    InstantiateCheckLogError, TestLogError,
    ::testing::Values(::std::make_tuple(LogLevel::kTraceLevel, "[ERROR]   "),
                      ::std::make_tuple(LogLevel::kDebugLevel, "[ERROR]   "),
                      ::std::make_tuple(LogLevel::kInfoLevel, "[ERROR]   "),
                      ::std::make_tuple(LogLevel::kWarnLevel, "[ERROR]   "),
                      ::std::make_tuple(LogLevel::kErrorLevel, "[ERROR]   "),
                      ::std::make_tuple(LogLevel::kCriticalLevel, "")));

TEST_P(TestLogCritical, CheckLogCritical) {
  ::std::tuple<LogLevel, std::string> param = GetParam();
  LogLevel settingLevel = ::std::get<0>(param);
  std::string expect_str = ::std::get<1>(param);

  SetLogLevel(settingLevel);

  testing::internal::CaptureStdout();
  EdgeAppLibLogCritical(context, message);
  std::string output = testing::internal::GetCapturedStdout();

  CheckEqual(expect_str, output);
}

INSTANTIATE_TEST_CASE_P(
    InstantiateCheckLogCritical, TestLogCritical,
    ::testing::Values(::std::make_tuple(LogLevel::kTraceLevel, "[CRITICAL]"),
                      ::std::make_tuple(LogLevel::kDebugLevel, "[CRITICAL]"),
                      ::std::make_tuple(LogLevel::kInfoLevel, "[CRITICAL]"),
                      ::std::make_tuple(LogLevel::kWarnLevel, "[CRITICAL]"),
                      ::std::make_tuple(LogLevel::kErrorLevel, "[CRITICAL]"),
                      ::std::make_tuple(LogLevel::kCriticalLevel,
                                        "[CRITICAL]")));

};  // namespace logapi_unittest

int setvbuf(FILE *__restrict__ __stream, char *__restrict__ __buf, int __modes,
            size_t __n) {
  return logapi_unittest::LogAPIUnitTest::_mock.get()->setvbuf(__stream, __buf,
                                                               __modes, __n);
}
