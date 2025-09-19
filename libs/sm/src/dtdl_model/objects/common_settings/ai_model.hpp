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

#ifndef DTDL_MODEL_OBJECTS_COMMON_SETTINGS_AI_MODEL_HPP
#define DTDL_MODEL_OBJECTS_COMMON_SETTINGS_AI_MODEL_HPP
#include <cstdint>

#include "dtdl_model/objects/json_object.hpp"
#include "sensor.h"

class AiModel : public JsonObject {
 public:
  AiModel();

  void InitializeValues();
  int Verify(JSON_Object *obj);
  int Apply(JSON_Object *obj);
  void StoreValue(const char *name, const char *target, const char *url_path,
                  const char *hash);

  void SetFailed();
  void Reuse();

 private:
  const char *name = nullptr;
  const char *target = nullptr;
  const char *url_path = nullptr;  // URL path to the AI model
  const char *hash = nullptr;      // Hash of the AI model file
  bool failed = false;
  char *get_filename_from_url(const char *url);
};

#endif /* DTDL_MODEL_OBJECTS_COMMON_SETTINGS_AI_MODEL_HPP \
        */
