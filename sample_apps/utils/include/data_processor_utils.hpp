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

#ifndef DATA_PROCESSOR_UTILS_H
#define DATA_PROCESSOR_UTILS_H

#include "data_processor_api.hpp"
#include "parson.h"
#include "sm_types.h"

/**
 * Retrieves a double associated with a parameter from a JSON object.
 *
 * @param json A pointer to the JSON_Object containing the data.
 * @param param The name of the parameter whose value is to be retrieved.
 * @param result A pointer to a double variable where the result will be stored.
 * @return An integer indicating the status:
 *         - 0 if the value was found and stored successfully.
 *         - 1 if the parameter was not found in the JSON object.
 *         - -1 if there was an error due to invalid input arguments.
 */
int GetValueNumber(JSON_Object *json, const char *param, double *result);
/**
 * Retrieves a string associated with a parameter from a JSON object.
 * @param json A pointer to the JSON_Object containing the data.
 * @param param The name of the parameter whose value is to be retrieved.
 * @param result A pointer to a char array where the result will be stored.
 * @return String
 */
int GetValueString(JSON_Object *json, const char *param, char *result);
/**
 * Retrieves a boolean associated with a parameter from a JSON object.
 *
 * @param json A pointer to the JSON_Object containing the data.
 * @param param The name of the parameter whose value is to be retrieved.
 * @param result A pointer to a boolean variable where the result will be
 * stored.
 * @return An integer indicating the status:
 *         - 0 if the value was found and stored successfully.
 *         - 1 if the parameter was not found in the JSON object.
 *         - -1 if there was an error due to invalid input arguments.
 */
int GetValueBoolean(JSON_Object *json, const char *param, bool *result);
char *GetConfigureErrorJson(ResponseCode code, const char *message,
                            const char *res_id);

#endif  // DATA_PROCESSOR_UTILS_H
