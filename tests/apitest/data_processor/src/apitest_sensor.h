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

#ifndef DATA_PROCSSOR_APITEST_SENSOR_H
#define DATA_PROCSSOR_APITEST_SENSOR_H

#include <cstdint>

int32_t RunApiTestScenarioSensorCore();

int32_t RunApiTestScenarioSensorStream();

int32_t RunApiTestScenarioSensorAct();

int32_t RunApiTestScenarioFrame();

int32_t RunApiTestScenarioProperty();

int32_t RunApiTestScenarioChannel();

int32_t RunApiTestScenarioError();

#endif  // DATA_PROCSSOR_APITEST_SENSOR_H
