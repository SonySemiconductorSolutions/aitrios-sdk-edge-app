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

#ifndef DTDL_MODEL_OBJECTS_COMMON_SETTINGS_HPP
#define DTDL_MODEL_OBJECTS_COMMON_SETTINGS_HPP

#include <cstdint>

#include "dtdl_model/objects/common_settings/codec_settings.hpp"
#include "dtdl_model/objects/common_settings/inference_settings.hpp"
#include "dtdl_model/objects/common_settings/port_settings.hpp"
#include "dtdl_model/objects/common_settings/pq_settings.hpp"
#include "dtdl_model/objects/json_object.hpp"
#include "macros.h"

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

class CommonSettings : public JsonObject {
#ifdef UNIT_TEST
  FRIEND_TEST(CommonSettingsIdleToRunning, ApplyLogLevel);
#endif
 public:
  CommonSettings();
  UT_ATTRIBUTE int Apply(JSON_Object *obj);

  uint32_t GetProcessState() const;
  UT_ATTRIBUTE PortSettings *GetPortSettings();
  UT_ATTRIBUTE PqSettings *GetPqSettings();
  UT_ATTRIBUTE InferenceSettings *GetInferenceSettings();
  UT_ATTRIBUTE CodecSettings *GetCodecSettings();
  uint32_t getNumOfInfPerMsg() const;

  int SetProcessState(uint32_t value);
  int SetLoggingLevel(uint32_t level);
  UT_ATTRIBUTE int SetInferencePerMessage(uint32_t value);

 private:
  PortSettings port_settings;
  PqSettings pq_settings;
  InferenceSettings inference_settings;
  CodecSettings codec_settings;

  uint32_t GetProcessState(JSON_Object *obj) const;
  uint32_t GetLoggingLevel(JSON_Object *obj) const;
  uint32_t GetInferencePerMessage(JSON_Object *obj) const;

  JSON_Object *GetSettingsJson(const char *setting);
};

#endif /* DTDL_MODEL_OBJECTS_COMMON_SETTINGS_HPP */
