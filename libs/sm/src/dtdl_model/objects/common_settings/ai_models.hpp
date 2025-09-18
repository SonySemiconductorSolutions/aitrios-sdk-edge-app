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

#ifndef DTDL_MODEL_OBJECTS_COMMON_SETTINGS_AI_MODELS_HPP
#define DTDL_MODEL_OBJECTS_COMMON_SETTINGS_AI_MODELS_HPP

#include <cstdint>

#include "dtdl_model/objects/common_settings/ai_model.hpp"
#include "dtdl_model/objects/json_object.hpp"
#include "macros.h"

#define MAX_AI_MODELS_COUNT 3

class AiModels {
 public:
  AiModels();
  ~AiModels();

  int Verify(JSON_Array *array);
  int Apply(JSON_Array *array);
  void StoreValue(const char *name, const char *target, const char *url_path,
                  const char *hash);

  JSON_Array *GetJsonArray() const { return json_array; }

 private:
  AiModel *ai_model_array[MAX_AI_MODELS_COUNT];
  uint32_t ai_model_array_count = 0;
  int32_t ai_model_array_index = -1;

  JSON_Array *json_array = nullptr;
};

#endif /* DTDL_MODEL_OBJECTS_COMMON_SETTINGS_AI_MODELS_HPP */
