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

#include "dtdl_model/objects/common_settings/port_settings.hpp"
#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_context.hpp"

#define NUMBER_OF_ITERATIONS "number_of_iterations"

InferenceSettings::InferenceSettings() {
  static Validation s_validations[] = {
      {.property = NUMBER_OF_ITERATIONS, .validation = kGe, .value = 0},
  };
  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));

  json_object_set_number(json_obj, NUMBER_OF_ITERATIONS, number_iterations);
}

int InferenceSettings::Apply(JSON_Object *obj) {
  if (!json_object_has_value(obj, NUMBER_OF_ITERATIONS)) return 0;

  uint32_t value = GetNumberOfIterations(obj);
  LOG_INFO("Desired number of iterations: %d", value);
  /* LCOV_EXCL_BR_START: check if initialized */
  if (value == number_iterations) return 0;
  /* LCOV_EXCL_BR_STOP */

  StateMachineContext::GetInstance(nullptr)->EnableNotification();

  json_object_set_number(json_obj, NUMBER_OF_ITERATIONS, value);
  number_iterations = value;
  return 0;
}

uint32_t InferenceSettings::GetNumberOfIterations(JSON_Object *obj) const {
  return (uint32_t)json_object_get_number(obj, NUMBER_OF_ITERATIONS);
}

uint32_t InferenceSettings::GetNumberOfIterations() const {
  return number_iterations;
}
