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

#include "event_functions/mock_sm.hpp"

#include <stdlib.h>
#include <string.h>

/* Execution count */
static int onCreateCount = 0;
static int onCreateReturn = 0;
static int onStartCount = 0;
static int onStartReturn = 0;
static int onConfigureCount = 0;
static int onConfigureReturn = 0;
static void *onConfigureValue = nullptr;
static int onStopCount = 0;
static int onStopReturn = 0;
static int onDestroyCount = 0;
static int onDestroyReturn = 0;

/* 0 = no called, 1 = called */
static int onIterateStatus = 0;
static int onIterateReturn = 0;

int onCreate() {
  onCreateCount++;
  return onCreateReturn;
}
int wasOnCreateCalled() { return onCreateCount; }
void resetOnCreate() {
  onCreateCount = 0;
  onCreateReturn = 0;
}
void setOnCreateError() { onCreateReturn = -1; }

int onStart() {
  onStartCount++;
  return onStartReturn;
}
int wasOnStartCalled() { return onStartCount; }
void resetOnStart() {
  onStartCount = 0;
  onStartReturn = 0;
}
void setOnStartError() { onStartReturn = -1; }

int onConfigure(char *topic, void *value, int valuelen) {
  onConfigureCount++;
  onConfigureValue = strdup((char *)value);
  free(value);
  return onConfigureReturn;
}
int wasOnConfigureCalled() { return onConfigureCount; }
void resetOnConfigure() {
  onConfigureCount = 0;
  onConfigureReturn = 0;
  if (onConfigureValue) free(onConfigureValue);
  onConfigureValue = nullptr;
}
void setOnConfigureError() { onConfigureReturn = -1; }
void *OnConfigureInput() { return onConfigureValue; }

int onIterate() {
  onIterateStatus = 1;
  return onIterateReturn;
}
int wasOnIterateCalled() { return onIterateStatus; }
void resetOnIterate() {
  onIterateStatus = 0;
  onIterateReturn = 0;
}
void setOnIterateError() { onIterateReturn = -1; }

int onStop() {
  onStopCount++;
  return onStopReturn;
}
int wasOnStopCalled() { return onStopCount; }
void resetOnStop() {
  onStopCount = 0;
  onStopReturn = 0;
}
void setOnStopError() { onStopReturn = -1; }

int onDestroy() {
  onDestroyCount++;
  return onDestroyReturn;
}
int wasOnDestroyCalled() { return onDestroyCount; }
void resetOnDestroy() {
  onDestroyCount = 0;
  onDestroyReturn = 0;
}
void setOnDestroyError() { onDestroyReturn = -1; }
