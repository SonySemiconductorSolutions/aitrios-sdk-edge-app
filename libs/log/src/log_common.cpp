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

void log_function(EdgeAppLibLogType level, const char *file, int line,
                  const char *fmt, ...) {
  char buf[LOGBUGSIZE];
  va_list args;
  va_start(args, fmt);
  char full_fmt[LOGBUGSIZE];
  snprintf(full_fmt, LOGBUGSIZE, "[%s:%d] %s", FILENAME(file), line, fmt);
  vsnprintf(buf, LOGBUGSIZE, full_fmt, args);
  va_end(args);
  level("", buf);
}
