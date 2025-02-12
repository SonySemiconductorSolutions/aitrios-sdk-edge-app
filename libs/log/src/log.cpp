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

#include "log_private.h"

#ifdef LOGDISABLE
void EdgeAppLibLogTrace(const char *context, const char *message) {}
void EdgeAppLibLogDebug(const char *context, const char *message) {}
void EdgeAppLibLogInfo(const char *context, const char *message) {}
void EdgeAppLibLogWarn(const char *context, const char *message) {}
void EdgeAppLibLogError(const char *context, const char *message) {}
void EdgeAppLibLogCritical(const char *context, const char *message) {}
#else
void EdgeAppLibLogTrace(const char *context, const char *message) {
  GetDevLogger().Trace(context, message);
}

void EdgeAppLibLogDebug(const char *context, const char *message) {
  GetDevLogger().Debug(context, message);
}

void EdgeAppLibLogInfo(const char *context, const char *message) {
  GetDevLogger().Info(context, message);
}

void EdgeAppLibLogWarn(const char *context, const char *message) {
  GetDevLogger().Warn(context, message);
}

void EdgeAppLibLogError(const char *context, const char *message) {
  GetDevLogger().Error(context, message);
}

void EdgeAppLibLogCritical(const char *context, const char *message) {
  GetDevLogger().Critical(context, message);
}
#endif
