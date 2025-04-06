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

/**
 * @file log.h
 * @brief Header file for Log API
 * @details This file defines the interface for logging.
 *          It provides functionalities to output logs at each log level.
 */

#ifndef _AITRIOS_LOG_H_
#define _AITRIOS_LOG_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Emit a trace level log message.
 * @param[in] context A context.
 * @param[in] message A message.
 * @details Emit a message at the trace level.
 */
void EdgeAppLibLogTrace(const char *context, const char *message);

/**
 * @brief Emit a debug level log message.
 * @param[in] context A context.
 * @param[in] message A message.
 * @details Emit a message at the debug level.
 */
void EdgeAppLibLogDebug(const char *context, const char *message);

/**
 * @brief Emit a info level log message.
 * @param[in] context A context.
 * @param[in] message A message.
 * @details Emit a message at the info level.
 */
void EdgeAppLibLogInfo(const char *context, const char *message);

/**
 * @brief Emit a warn level log message.
 * @param[in] context A context.
 * @param[in] message A message.
 * @details Emit a message at the warn level.
 */
void EdgeAppLibLogWarn(const char *context, const char *message);

/**
 * @brief Emit a error level log message.
 * @param[in] context A context.
 * @param[in] message A message.
 * @details Emit a message at the error level.
 */
void EdgeAppLibLogError(const char *context, const char *message);

/**
 * @brief Emit a critical level log message.
 * @param[in] context A context.
 * @param[in] message A message.
 * @details Emit a message at the critical level.
 */
void EdgeAppLibLogCritical(const char *context, const char *message);

typedef void (*EdgeAppLibLogType)(const char *context, const char *message);

#define LOGBUGSIZE 128

#define FILENAME(file) (strrchr(file, '/') ? strrchr(file, '/') + 1 : file)

/**
 * @enum LogLevel
 * @brief Represents the log level.
 */
typedef enum {
  kCriticalLevel =
      0,       /**< Describes the messages used when a fatal error occurs. */
  kErrorLevel, /**< Describes messages used when a serious error occurs. */
  kWarnLevel, /**< Describes messages used when a hazardous situation occurs. */
  kInfoLevel, /**< Describes messages used to monitor an application. */
  kDebugLevel, /**< Describes messages used to debug an application. */
  kTraceLevel, /**< Describes messages about the values of variables and the
                  flow of control within an application. */
} LogLevel;

void log_function(LogLevel level, const char *file, int line, const char *fmt,
                  ...);

#define LOG_TRACE(fmt, ...) \
  log_function(kTraceLevel, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) \
  log_function(kDebugLevel, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
  log_function(kInfoLevel, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
  log_function(kWarnLevel, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) \
  log_function(kErrorLevel, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) \
  log_function(kCriticalLevel, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _AITRIOS_LOG_H_ */
