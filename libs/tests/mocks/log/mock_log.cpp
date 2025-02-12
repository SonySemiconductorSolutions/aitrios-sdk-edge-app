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
#include <time.h>

#include "log.h"

#define INTEGRATION_TEST_LOG "./integration_test.log"

static bool s_log_enable = true;

bool isEdgeAppLibLogEnable() { return s_log_enable; }

void setEdgeAppLibLogEnable(bool enable) { s_log_enable = enable; }

const char *level_str[] = {
    "[CRITICAL]", "[ERROR]   ", "[WARN]    ",
    "[INFO]    ", "[DEBUG]   ", "[TRACE]   ",
};

void MockLoggerGetTimestamp(char *timestamp, size_t length) {
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

void MockLoggerLog(const char *level, const char *context,
                   const char *message) {
  char timestamp[24];
  MockLoggerGetTimestamp(timestamp, sizeof(timestamp));
  FILE *fp;
  fp = fopen(INTEGRATION_TEST_LOG, "a");
  fprintf(fp, "%s %s %s %s\n", timestamp, level, context, message);
  fflush(fp);
  fclose(fp);
}

#ifdef LOGDISABLE
void EdgeAppLibLogTrace(const char *context, const char *message) {}
void EdgeAppLibLogDebug(const char *context, const char *message) {}
void EdgeAppLibLogInfo(const char *context, const char *message) {}
void EdgeAppLibLogWarn(const char *context, const char *message) {}
void EdgeAppLibLogError(const char *context, const char *message) {}
void EdgeAppLibLogCritical(const char *context, const char *message) {}
#else
void EdgeAppLibLogTrace(const char *context, const char *message) {
  if (s_log_enable) {
    MockLoggerLog(level_str[5], context, message);
  }
}

void EdgeAppLibLogDebug(const char *context, const char *message) {
  if (s_log_enable) {
    MockLoggerLog(level_str[4], context, message);
  }
}

void EdgeAppLibLogInfo(const char *context, const char *message) {
  if (s_log_enable) {
    MockLoggerLog(level_str[3], context, message);
  }
}

void EdgeAppLibLogWarn(const char *context, const char *message) {
  if (s_log_enable) {
    MockLoggerLog(level_str[2], context, message);
  }
}

void EdgeAppLibLogError(const char *context, const char *message) {
  if (s_log_enable) {
    MockLoggerLog(level_str[1], context, message);
  }
}

void EdgeAppLibLogCritical(const char *context, const char *message) {
  if (s_log_enable) {
    MockLoggerLog(level_str[0], context, message);
  }
}
#endif
