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

#ifndef DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_REGISTER_ACCESS_ARRAY_HPP
#define DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_REGISTER_ACCESS_ARRAY_HPP
#include <cstdint>

#include "dtdl_model/objects/common_settings/pq_settings/register_access.hpp"
#include "dtdl_model/objects/json_object.hpp"
#include "sensor.h"

// TODO: dynamic array
#define MAX_REGISTER_ACCESS_COUNT 4

class RegisterAccessArray {
 public:
  RegisterAccessArray();
  ~RegisterAccessArray();

  int Verify(JSON_Array *array);
  int Apply(JSON_Array *array);
  void StoreValue(uint32_t id, uint64_t address, uint64_t data, int bit_length);

  JSON_Array *GetJsonArray() const { return json_array; }

 private:
  RegisterAccess *register_access_array[MAX_REGISTER_ACCESS_COUNT];
  uint32_t register_access_array_count = 0;
  int32_t register_access_array_index = -1;

  JSON_Array *json_array = nullptr;
};

#endif /* DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_REGISTER_ACCESS_ARRAY_HPP \
        */
