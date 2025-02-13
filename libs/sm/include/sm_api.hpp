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

#ifndef AITRIOS_SM_API_H
#define AITRIOS_SM_API_H

#include "parson.h"
#include "sensor.h"

/**
 * Called from EdgeAppLibSensorStreamSetProperty
 */
void updateProperty(EdgeAppLibSensorStream stream, const char *property_key,
                    const void *value, size_t value_size);

void updateCustomSettings(void *state, int statelen);

JSON_Object *getPortSettings(void);

JSON_Object *getCodecSettings(void);

uint32_t getNumOfInfPerMsg(void);

EdgeAppLibSensorStream GetSensorStream(void);
#endif /* AITRIOS_SM_API_H */
