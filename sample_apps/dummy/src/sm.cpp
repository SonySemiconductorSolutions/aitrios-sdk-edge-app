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

#include "sm.h"

#include "log.h"

int onCreate() {
  LOG_INFO("Inside onCreate. Not implemented.");
  return 0;
}

int onConfigure(char *topic, void *value, int valuesize) {
  LOG_INFO("Inside onConfigure. Not implemented.");
  return 0;
}

int onIterate() {
  LOG_INFO("Inside onIterate. Not implemented.");
  return 0;
}

int onStop() {
  LOG_INFO("Inside onStop. Not implemented.");
  return 0;
}

int onStart() {
  LOG_INFO("Inside onStart. Not implemented.");
  return 0;
}

int onDestroy() {
  LOG_INFO("Inside onDestroy. Not implemented.");
  return 0;
}
