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

#include "log_private.h"

#include <time.h>

namespace log_level {
const char *level_str[] = {
    "[CRITICAL]", "[ERROR]   ", "[WARN]    ",
    "[INFO]    ", "[DEBUG]   ", "[TRACE]   ",
};
}

// LogConfig class
LogConfig &LogConfig::GetInstance() {
  static LogConfig instance;
  return instance;
}

void LogConfig::SetLoggingLevel(LogLevel level) { level_ = level; }

LogLevel LogConfig::GetLoggingLevel() const { return level_; }

// SimpleLogger class
SimpleLogger::SimpleLogger() {
  if (setvbuf(stdout, NULL, _IOFBF, BUFSIZ) != 0) {
    fprintf(stderr, "fail setvbuf\n");
  }
}

SimpleLogger::~SimpleLogger() {}

void SimpleLogger::GetTimestamp(char *timestamp, size_t length) const {
  struct timespec ts;
  struct tm t;
  clock_gettime(CLOCK_REALTIME, &ts);
  localtime_r(&ts.tv_sec, &t);

  // Create timestamp string
  char timestamp_sec[20];
  strftime(timestamp_sec, 20, "%Y-%m-%dT%H:%M:%S", &t);

  // Add msec to timestamp string
  const int msec = ts.tv_nsec / 1000000;
  snprintf(timestamp, length, "%s.%03d", timestamp_sec, msec);

  return;
}

void SimpleLogger::Log(const char *level, const char *context,
                       const char *message) const {
  char timestamp[24];
  GetTimestamp(timestamp, sizeof(timestamp));
  printf("%s %s %s %s\n", timestamp, level, context, message);
  fflush(stdout);
}

// Logger class
Logger::Logger() {}

Logger::~Logger() {}

// DevLogger class
DevLogger &DevLogger::GetInstance() {
  static DevLogger instance;
  return instance;
}

void DevLogger::Log(LogLevel level, const char *context,
                    const char *message) const {
  if (IsLoggable(level)) {
    logger_.Log(log_level::level_str[level], context, message);
  }
}

void DevLogger::Trace(const char *context, const char *message) const {
  Log(LogLevel::kTraceLevel, context, message);
}

void DevLogger::Debug(const char *context, const char *message) const {
  Log(LogLevel::kDebugLevel, context, message);
}

void DevLogger::Info(const char *context, const char *message) const {
  Log(LogLevel::kInfoLevel, context, message);
}

void DevLogger::Warn(const char *context, const char *message) const {
  Log(LogLevel::kWarnLevel, context, message);
}

void DevLogger::Error(const char *context, const char *message) const {
  Log(LogLevel::kErrorLevel, context, message);
}

void DevLogger::Critical(const char *context, const char *message) const {
  Log(LogLevel::kCriticalLevel, context, message);
}

bool DevLogger::IsLoggable(LogLevel level) const {
  return LogConfig::GetInstance().GetLoggingLevel() >= level;
}

// static method
Logger &GetDevLogger() { return DevLogger::GetInstance(); }
