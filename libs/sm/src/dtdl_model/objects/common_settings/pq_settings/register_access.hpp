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

#ifndef DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_REGISTER_ACCESS_HPP
#define DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_REGISTER_ACCESS_HPP
#include <cstdint>

#include "dtdl_model/objects/json_object.hpp"
#include "sensor.h"

class RegisterAccess : public JsonObject {
 public:
  RegisterAccess();

  void InitializeValues();
  int Verify(JSON_Object *obj);
  int Apply(JSON_Object *obj);
  void StoreValue(uint32_t id, uint64_t address, uint64_t data, int bit_length);

  void SetFailed();
  void Reuse();

 private:
  EdgeAppLibSensorRegisterAccessProperty register_access_property = {0};
  EdgeAppLibSensorRegisterAccess64Property register_access_64_property = {0};
  EdgeAppLibSensorRegisterAccess32Property register_access_32_property = {0};
  EdgeAppLibSensorRegisterAccess16Property register_access_16_property = {0};
  EdgeAppLibSensorRegisterAccess8Property register_access_8_property = {0};
  bool failed = false;
};

#endif /* DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_REGISTER_ACCESS_HPP \
        */
