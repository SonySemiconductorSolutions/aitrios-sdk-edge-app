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

#include "log.h"
#include "log_internal.h"

void log_function(LogLevel level, const char *file, int line, const char *fmt,
                  ...) {
#ifndef LOGDISABLE
#ifndef MOCK_INTEGRATION_TEST
  if (GetLogLevel() >= level) {
#else   // MOCK_INTEGRATION_TEST
  if (1) {
#endif  // MOCK_INTEGRATION_TEST
    char buf[LOGBUGSIZE] = {0};
    va_list args;
    va_start(args, fmt);
    char full_fmt[LOGBUGSIZE] = {0};
    snprintf(full_fmt, LOGBUGSIZE, "[%s:%d] %s", FILENAME(file), line, fmt);
    vsnprintf(buf, LOGBUGSIZE - 1, full_fmt, args);
    va_end(args);
    // Ensure null termination
    buf[LOGBUGSIZE - 1] = '\0';

    const char *context = "";
    switch (level) {
      case kCriticalLevel:
        EdgeAppLibLogCritical(context, buf);
        break;
      case kErrorLevel:
        EdgeAppLibLogError(context, buf);
        break;
      case kWarnLevel:
        EdgeAppLibLogWarn(context, buf);
        break;
      case kInfoLevel:
        EdgeAppLibLogInfo(context, buf);
        break;
      case kDebugLevel:
        EdgeAppLibLogDebug(context, buf);
        break;
      case kTraceLevel:
        EdgeAppLibLogTrace(context, buf);
        break;
      default:
        break;
    }
  }
#endif  // LOGDISABLE
}
