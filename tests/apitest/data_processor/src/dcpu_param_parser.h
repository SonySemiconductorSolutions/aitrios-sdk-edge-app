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
#ifndef DCPU_PARAM_PARSER_H
#define DCPU_PARAM_PARSER_H

#include <edgeapp/sensor.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "parson.h"

/* enum */
typedef enum {
  E_PPL_OK,
  E_PPL_INVALID_PARAM,
  E_PPL_E_MEMORY_ERROR,
  E_PPL_INVALID_STATE,
  E_PPL_OTHER
} EPPL_RESULT_CODE;

EPPL_RESULT_CODE PPL_GetProperty(EdgeAppLibSensorStream stream);

EPPL_RESULT_CODE PPL_NMS_Op3pp_SetProperty(EdgeAppLibSensorStream stream);

int ParsePostProcessParameter(JSON_Value *root_value, const char *value);

#endif  // DCPU_PARAM_PARSER_H
