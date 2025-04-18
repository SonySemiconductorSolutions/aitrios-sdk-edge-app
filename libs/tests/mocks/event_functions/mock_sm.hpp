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

#ifndef MOCKS_AITRIOS_SM_HPP
#define MOCKS_AITRIOS_SM_HPP

#include "sm.h"

int wasOnCreateCalled();
void resetOnCreate();
void setOnCreateError();

int wasOnStartCalled();
void resetOnStart();
void setOnStartError();

int wasOnConfigureCalled();
void resetOnConfigure();
void setOnConfigureError();
void *OnConfigureInput();

int wasOnIterateCalled();
void resetOnIterate();
void setOnIterateError();

int wasOnStopCalled();
void resetOnStop();
void setOnStopError();

int wasOnDestroyCalled();
void resetOnDestroy();
void setOnDestroyError();

#endif /* MOCKS_AITRIOS_SM_HPP */
