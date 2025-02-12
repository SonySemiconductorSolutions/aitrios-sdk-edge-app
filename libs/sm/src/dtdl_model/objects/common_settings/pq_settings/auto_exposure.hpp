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

#ifndef DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_AUTO_EXPOSURE_HPP
#define DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_AUTO_EXPOSURE_HPP

#include <cstdint>

#include "dtdl_model/objects/json_object.hpp"
#include "sensor.h"

class AutoExposure : public JsonObject {
 public:
  AutoExposure();

  void InitializeValues();
  int Verify(JSON_Object *obj);
  int Apply(JSON_Object *obj);
  void StoreValue(uint32_t max_exp, uint32_t min_exp, float max_gain,
                  uint32_t max_conv_sp);

 private:
  EdgeAppLibSensorCameraAutoExposureProperty auto_exposure = {0};
};

#endif /* DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_AUTO_EXPOSURE_HPP \
        */
