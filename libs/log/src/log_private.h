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

#ifndef _AITRIOS_LOG_PRIVATE_H_
#define _AITRIOS_LOG_PRIVATE_H_

#include <stdio.h>

#include "log.h"
#include "log_internal.h"

class LogConfig {
 public:
  LogConfig(const LogConfig &) = delete;
  LogConfig &operator=(const LogConfig &) = delete;
  LogConfig(LogConfig &&) = delete;
  LogConfig &operator=(LogConfig &&) = delete;

  static LogConfig &GetInstance();
  void SetLoggingLevel(LogLevel level);
  LogLevel GetLoggingLevel() const;

 private:
  LogConfig() = default;
  ~LogConfig() = default;

  LogLevel level_ = LogLevel::kWarnLevel;
};

class SimpleLogger {
 public:
  SimpleLogger();
  virtual ~SimpleLogger();

  void GetTimestamp(char *timestamp, size_t length) const;
  void Log(const char *level, const char *context, const char *message) const;
};

class Logger {
 public:
  Logger();
  virtual ~Logger();

  virtual void Trace(const char *context, const char *message) const = 0;
  virtual void Debug(const char *context, const char *message) const = 0;
  virtual void Info(const char *context, const char *message) const = 0;
  virtual void Warn(const char *context, const char *message) const = 0;
  virtual void Error(const char *context, const char *message) const = 0;
  virtual void Critical(const char *context, const char *message) const = 0;
};

class DevLogger : public Logger {
 public:
  DevLogger(const DevLogger &) = delete;
  DevLogger &operator=(const DevLogger &) = delete;
  DevLogger(DevLogger &&) = delete;
  DevLogger &operator=(DevLogger &&) = delete;

  static DevLogger &GetInstance();
  void Log(LogLevel level, const char *context, const char *message) const;
  void Trace(const char *context, const char *message) const override;
  void Debug(const char *context, const char *message) const override;
  void Info(const char *context, const char *message) const override;
  void Warn(const char *context, const char *message) const override;
  void Error(const char *context, const char *message) const override;
  void Critical(const char *context, const char *message) const override;

 private:
  DevLogger() = default;
  ~DevLogger() = default;

  bool IsLoggable(LogLevel level) const;

  SimpleLogger logger_;
};

Logger &GetDevLogger();

#endif  // _AITRIOS_LOG_PRIVATE_H_
