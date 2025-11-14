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

#ifndef DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_HPP
#define DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_HPP

#include <math.h>
#include <stdlib.h>

#include <cstdint>

#include "dtdl_model/objects/common_settings/pq_settings/auto_exposure.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/auto_exposure_metering.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/auto_white_balance.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/camera_image_flip.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/camera_image_size.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/frame_rate.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/image_cropping.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/manual_exposure.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/manual_white_balance_preset.hpp"
#include "dtdl_model/objects/common_settings/pq_settings/register_access_array.hpp"
#include "dtdl_model/objects/json_object.hpp"
#include "macros.h"

class PqSettings : public JsonObject {
 public:
  PqSettings();
  ~PqSettings() {
    if (_gamma_param) free(_gamma_param);
  }

  void InitializeValues();
  int Verify(JSON_Object *obj);
  int Apply(JSON_Object *obj);

  int ApplyDigitalZoom(double digital_zoom);
  int ApplyExposureMode(double mode);
  int ApplyImageRotation(double image_rotation);
  int ApplyEvCompensationVal(double ev_compensation_val);
  int ApplyAeAntiFlickerMode(double ae_anti_flicker_mode);
  int ApplyWhiteBalanceMode(double white_balance_mode);
  int ApplyGammaMode(double gamma_mode);
  int ApplyGammaParameter(const char *gamma_parameter);

  void StoreDigitalZoom(float digital_zoom);
  void StoreExposureMode(int mode);
  void StoreImageRotation(int image_rotation);
  void StoreEvCompensationVal(float ev_compensation_val);
  void StoreAeAntiFlickerMode(int ae_anti_flicker_mode);
  void StoreWhiteBalanceMode(int white_balance_mode);
  void StoreGammaMode(int gamma_mode);
  void StoreGammaParameter(uint8_t *bin, uint32_t bin_size);

  UT_ATTRIBUTE CameraImageSize *GetCameraImageSize();
  UT_ATTRIBUTE CameraImageFlip *GetCameraImageFlip();
  UT_ATTRIBUTE FrameRate *GetFrameRate();
  UT_ATTRIBUTE AutoExposure *GetAutoExposure();
  UT_ATTRIBUTE AutoExposureMetering *GetAutoExposureMetering();
  UT_ATTRIBUTE ManualExposure *GetManualExposure();
  UT_ATTRIBUTE AutoWhiteBalance *GetAutoWhiteBalance();
  UT_ATTRIBUTE ManualWhiteBalancePreset *GetManualWhiteBalancePreset();
  UT_ATTRIBUTE ImageCropping *GetImageCropping();
  UT_ATTRIBUTE RegisterAccessArray *GetRegisterAccessArray();

 private:
  CameraImageSize camera_image_size;
  CameraImageFlip camera_image_flip;
  FrameRate frame_rate;
  AutoWhiteBalance auto_white_balance;
  ManualWhiteBalancePreset manual_white_balance_preset;
  ImageCropping image_cropping;
  AutoExposure auto_exposure;
  AutoExposureMetering auto_exposure_metering;
  ManualExposure manual_exposure;
  RegisterAccessArray register_access_array;

  // NAN to identify uninitialized value
  float _digital_zoom = NAN;
  float _exposure_mode = NAN;
  float _image_rotation = NAN;
  float _ev_compensation_val = NAN;
  float _ae_anti_flicker_mode = NAN;
  float _white_balance_mode = NAN;
  float _gamma_mode = NAN;
  EdgeAppLibSensorInferenceGammaParameterProperty *_gamma_param = NULL;
};

#endif /* DTDL_MODEL_OBJECTS_COMMON_SETTINGS_PQ_SETTINGS_HPP */
