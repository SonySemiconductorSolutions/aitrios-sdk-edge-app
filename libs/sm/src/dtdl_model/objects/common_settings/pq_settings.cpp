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

#include "dtdl_model/objects/common_settings/pq_settings.hpp"

#include <cstdlib>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

extern "C" {
#include "base64.h"
}

#define CAMERA_IMAGE_SIZE "camera_image_size"
#define CAMERA_IMAGE_FLIP "camera_image_flip"
#define DIGITAL_ZOOM "digital_zoom"
#define EXPOSURE_MODE "exposure_mode"
#define AUTO_EXPOSURE "auto_exposure"
#define AUTO_EXPOSURE_METERING "auto_exposure_metering"
#define EV_COMPENSATION "ev_compensation"
#define AE_ANTI_FLICKER_MODE "ae_anti_flicker_mode"
#define MANUAL_EXPOSURE "manual_exposure"
#define FRAME_RATE "frame_rate"
#define WHITE_BALANCE_MODE "white_balance_mode"
#define AUTO_WHITE_BALANCE "auto_white_balance"
#define MANUAL_WHITE_BALANCE_PRESET "manual_white_balance_preset"
#define IMAGE_CROPPING "image_cropping"
#define IMAGE_ROTATION "image_rotation"
#define REGISTER_ACCESS "register_access"
#define GAMMA_MODE "gamma_mode"
#define GAMMA_PARAMETER "gamma_parameter"

using namespace EdgeAppLib;

typedef int (PqSettings::*ApplyFunc)(double);
typedef struct {
  ApplyFunc func;
  const char *property;
} ApplyFuncType;

PqSettings::PqSettings() {
  static Validation s_validations[] = {
      {CAMERA_IMAGE_SIZE, kType, JSONObject},
      {CAMERA_IMAGE_FLIP, kType, JSONObject},
      {DIGITAL_ZOOM, kType, JSONNumber},
      {EXPOSURE_MODE, kType, JSONNumber},
      {AUTO_EXPOSURE, kType, JSONObject},
      {AUTO_EXPOSURE_METERING, kType, JSONObject},
      {EV_COMPENSATION, kType, JSONNumber},
      {AE_ANTI_FLICKER_MODE, kType, JSONNumber},
      {MANUAL_EXPOSURE, kType, JSONObject},
      {FRAME_RATE, kType, JSONObject},
      {WHITE_BALANCE_MODE, kType, JSONNumber},
      {AUTO_WHITE_BALANCE, kType, JSONObject},
      {MANUAL_WHITE_BALANCE_PRESET, kType, JSONObject},
      {IMAGE_CROPPING, kType, JSONObject},
      {IMAGE_ROTATION, kType, JSONNumber},
      // range of image_rotation
      {IMAGE_ROTATION, kGe, AITRIOS_SENSOR_ROTATION_ANGLE_0_DEG},
      {IMAGE_ROTATION, kLe, AITRIOS_SENSOR_ROTATION_ANGLE_270_DEG},
      // range of ae_anti_flicker
      {AE_ANTI_FLICKER_MODE, kGe, AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_OFF},
      {AE_ANTI_FLICKER_MODE, kLe,
       AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_FORCE_60HZ},
      // range of white_balance_mode
      {WHITE_BALANCE_MODE, kGe,
       AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_AUTO},
      {WHITE_BALANCE_MODE, kLe,
       AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_MANUAL_PRESET},
      // range of exposure_mode
      {EXPOSURE_MODE, kGe, AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO},
      {EXPOSURE_MODE, kLe, AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_MANUAL},
      {EXPOSURE_MODE, kNe, 1},
      {EXPOSURE_MODE, kNe, 2},
      {REGISTER_ACCESS, kType, JSONArray},
      {GAMMA_MODE, kGe, AITRIOS_SENSOR_INFERENCE_GAMMA_MODE_STANDARD},
      {GAMMA_MODE, kLe, AITRIOS_SENSOR_INFERENCE_GAMMA_MODE_AUTO},
      {GAMMA_PARAMETER, kType, JSONString},
  };
  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
  static Property s_properties[] = {
      {.property = CAMERA_IMAGE_SIZE, .obj = &camera_image_size},
      {.property = CAMERA_IMAGE_FLIP, .obj = &camera_image_flip},
      {.property = AUTO_EXPOSURE, .obj = &auto_exposure},
      {.property = AUTO_EXPOSURE_METERING, .obj = &auto_exposure_metering},
      {.property = MANUAL_EXPOSURE, .obj = &manual_exposure},
      {.property = FRAME_RATE, .obj = &frame_rate},
      {.property = AUTO_WHITE_BALANCE, .obj = &auto_white_balance},
      {.property = MANUAL_WHITE_BALANCE_PRESET,
       .obj = &manual_white_balance_preset},
      {.property = IMAGE_CROPPING,
       .obj = &image_cropping}}; /* LCOV_EXCL_BR_LINE: array check */
  SetProperties(s_properties, sizeof(s_properties) / sizeof(Property));

  _gamma_param = (EdgeAppLibSensorInferenceGammaParameterProperty *)malloc(
      sizeof(EdgeAppLibSensorInferenceGammaParameterProperty));
  if (!_gamma_param) {
    LOG_ERR("Malloc for _gamma_param failed.");
    return;
  }
  memset(_gamma_param, 0,
         sizeof(EdgeAppLibSensorInferenceGammaParameterProperty));

  json_object_set_value(
      json_obj, CAMERA_IMAGE_SIZE,
      json_object_get_wrapping_value(GetCameraImageSize()->GetJsonObject()));
  json_object_set_value(
      json_obj, CAMERA_IMAGE_FLIP,
      json_object_get_wrapping_value(GetCameraImageFlip()->GetJsonObject()));
  json_object_set_null(json_obj, DIGITAL_ZOOM);
  json_object_set_null(json_obj, EXPOSURE_MODE);
  json_object_set_value(
      json_obj, AUTO_EXPOSURE,
      json_object_get_wrapping_value(GetAutoExposure()->GetJsonObject()));
  json_object_set_value(json_obj, AUTO_EXPOSURE_METERING,
                        json_object_get_wrapping_value(
                            GetAutoExposureMetering()->GetJsonObject()));
  json_object_set_null(json_obj, EV_COMPENSATION);
  json_object_set_null(json_obj, AE_ANTI_FLICKER_MODE);
  json_object_set_value(
      json_obj, MANUAL_EXPOSURE,
      json_object_get_wrapping_value(GetManualExposure()->GetJsonObject()));
  json_object_set_value(
      json_obj, FRAME_RATE,
      json_object_get_wrapping_value(GetFrameRate()->GetJsonObject()));
  json_object_set_null(json_obj, WHITE_BALANCE_MODE);
  json_object_set_value(
      json_obj, AUTO_WHITE_BALANCE,
      json_object_get_wrapping_value(GetAutoWhiteBalance()->GetJsonObject()));
  json_object_set_value(json_obj, MANUAL_WHITE_BALANCE_PRESET,
                        json_object_get_wrapping_value(
                            GetManualWhiteBalancePreset()->GetJsonObject()));
  json_object_set_value(
      json_obj, IMAGE_CROPPING,
      json_object_get_wrapping_value(GetImageCropping()->GetJsonObject()));
  json_object_set_null(json_obj, IMAGE_ROTATION);
  json_object_set_value(
      json_obj, REGISTER_ACCESS,
      json_array_get_wrapping_value(GetRegisterAccessArray()->GetJsonArray()));
  json_object_set_null(json_obj, GAMMA_MODE);
  json_object_set_null(json_obj, GAMMA_PARAMETER);
}

void PqSettings::InitializeValues() {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();

  GetCameraImageSize()->InitializeValues();
  GetCameraImageFlip()->InitializeValues();

  EdgeAppLibSensorCameraDigitalZoomProperty digital_zoom_prop = {};
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY,
                          &digital_zoom_prop, sizeof(digital_zoom_prop));
  json_object_set_number(json_obj, DIGITAL_ZOOM,
                         digital_zoom_prop.magnification);
  _digital_zoom = digital_zoom_prop.magnification;

  EdgeAppLibSensorCameraExposureModeProperty exposure_mode = {};
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY,
                          &exposure_mode, sizeof(exposure_mode));
  json_object_set_number(json_obj, EXPOSURE_MODE, exposure_mode.mode);
  _exposure_mode = exposure_mode.mode;

  GetAutoExposure()->InitializeValues();

  GetAutoExposureMetering()->InitializeValues();

  EdgeAppLibSensorCameraEvCompensationProperty ev_compensation_prop = {};
  SensorStreamGetProperty(stream,
                          AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY,
                          &ev_compensation_prop, sizeof(ev_compensation_prop));
  json_object_set_number(json_obj, EV_COMPENSATION,
                         ev_compensation_prop.ev_compensation);
  _ev_compensation_val = ev_compensation_prop.ev_compensation;

  EdgeAppLibSensorCameraAntiFlickerModeProperty ae_anti_flicker_mode_prop = {};
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY,
      &ae_anti_flicker_mode_prop, sizeof(ae_anti_flicker_mode_prop));
  json_object_set_number(json_obj, AE_ANTI_FLICKER_MODE,
                         ae_anti_flicker_mode_prop.anti_flicker_mode);
  _ae_anti_flicker_mode = ae_anti_flicker_mode_prop.anti_flicker_mode;

  GetManualExposure()->InitializeValues();
  GetFrameRate()->InitializeValues();

  EdgeAppLibSensorWhiteBalanceModeProperty white_balance_mode_prop = {};
  SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY,
      &white_balance_mode_prop, sizeof(white_balance_mode_prop));
  json_object_set_number(json_obj, WHITE_BALANCE_MODE,
                         white_balance_mode_prop.mode);
  _white_balance_mode = white_balance_mode_prop.mode;

  GetAutoWhiteBalance()->InitializeValues();
  GetManualWhiteBalancePreset()->InitializeValues();
  GetImageCropping()->InitializeValues();

  EdgeAppLibSensorImageRotationProperty image_rotation_property = {};
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY,
                          &image_rotation_property,
                          sizeof(image_rotation_property));
  json_object_set_number(json_obj, IMAGE_ROTATION,
                         image_rotation_property.rotation_angle);
  _image_rotation = image_rotation_property.rotation_angle;

  EdgeAppLibSensorInferenceGammaModeProperty gamma_mode_property = {};
  SensorStreamGetProperty(stream, AITRIOS_SENSOR_GAMMA_MODE_PROPERTY_KEY,
                          &gamma_mode_property, sizeof(gamma_mode_property));
  json_object_set_number(json_obj, GAMMA_MODE, gamma_mode_property.gamma_mode);
  _gamma_mode = gamma_mode_property.gamma_mode;

  if (!_gamma_param) return;
  memset(_gamma_param, 0,
         sizeof(EdgeAppLibSensorInferenceGammaParameterProperty));
  _gamma_param->gamma_mode = (EdgeAppLibSensorInferenceGammaMode)_gamma_mode;
  _gamma_param->param_size = 0;
  json_object_set_string(json_obj, GAMMA_PARAMETER, "");
}

int PqSettings::Verify(JSON_Object *obj) {
  int ret = JsonObject::Verify(obj);
  if (!ret) {
    if (json_object_has_value(obj, REGISTER_ACCESS)) {
      JSON_Array *array = json_object_dotget_array(obj, REGISTER_ACCESS);
      if (array) {
        ret = GetRegisterAccessArray()->Verify(array);
      }
    }
  }
  return ret;
}

int PqSettings::Apply(JSON_Object *obj) {
  int ret = 0;
  ApplyFuncType funcs[] = {
      {&PqSettings::ApplyDigitalZoom, DIGITAL_ZOOM},
      {&PqSettings::ApplyExposureMode, EXPOSURE_MODE},
      {&PqSettings::ApplyEvCompensationVal, EV_COMPENSATION},
      {&PqSettings::ApplyAeAntiFlickerMode, AE_ANTI_FLICKER_MODE},
      {&PqSettings::ApplyWhiteBalanceMode, WHITE_BALANCE_MODE},
      {&PqSettings::ApplyImageRotation, IMAGE_ROTATION},
      {&PqSettings::ApplyGammaMode, GAMMA_MODE}};
  int num_funcs = sizeof(funcs) / sizeof(ApplyFuncType);
  /* LCOV_EXCL_BR_START */
  for (int i = 0; i < num_funcs; ++i) {
    if (json_object_has_value(obj, funcs[i].property)) {
      int ret2 = (this->*(funcs[i].func))(
          json_object_dotget_number(obj, funcs[i].property));
      if (ret2 != 0) {
        ret = ret2;
      }
    }
  }
  /* LCOV_EXCL_BR_STOP */

  if (json_object_has_value(obj, GAMMA_PARAMETER)) {
    const char *gamma_parameter = json_object_get_string(obj, GAMMA_PARAMETER);
    if (gamma_parameter) this->ApplyGammaParameter(gamma_parameter);
  }

  if (json_object_has_value(obj, REGISTER_ACCESS)) {
    JSON_Array *array = json_object_dotget_array(obj, REGISTER_ACCESS);
    if (array) {
      GetRegisterAccessArray()->Apply(array);
      json_object_set_value(json_obj, REGISTER_ACCESS,
                            json_array_get_wrapping_value(
                                GetRegisterAccessArray()->GetJsonArray()));
    }
  }

  int ret3 = JsonObject::Apply(obj);
  if (ret3 != 0) {
    ret = ret3;
  }
  return ret;
}

int PqSettings::ApplyDigitalZoom(double digital_zoom) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorCameraDigitalZoomProperty digital_zoom_prop = {
      .magnification = (float)digital_zoom};
  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY,
      &digital_zoom_prop, sizeof(digital_zoom_prop));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Digital Zoom property failed to be set. Please use valid "
        "values for digital_zoom.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void PqSettings::StoreDigitalZoom(float digital_zoom) {
  if (IsAlmostEqual(_digital_zoom, digital_zoom)) return;

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating DigitalZoom");
  json_object_set_number(json_obj, DIGITAL_ZOOM, digital_zoom);
  _digital_zoom = digital_zoom;
}

int PqSettings::ApplyExposureMode(double mode) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorCameraExposureModeProperty exposure_mode = {
      .mode = (EdgeAppLibSensorCameraExposureMode)mode};
  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY, &exposure_mode,
      sizeof(exposure_mode));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Camera Exposure Mode property failed to be set. Please use valid "
        "values for exposure_mode.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void PqSettings::StoreExposureMode(int mode) {
  if (_exposure_mode == mode) return;

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating ExposureMode");
  json_object_set_number(json_obj, EXPOSURE_MODE, mode);
  _exposure_mode = mode;
}

int PqSettings::ApplyImageRotation(double image_rotation) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorImageRotationProperty image_rotation_property = {
      .rotation_angle = (EdgeAppLibSensorRotationAngle)image_rotation};
  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY,
      &image_rotation_property, sizeof(image_rotation_property));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Image Rotation property failed to be set. Please use valid values "
        "for image_rotation.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void PqSettings::StoreImageRotation(int image_rotation) {
  if (_image_rotation == image_rotation) return;

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating ImageRotation");
  json_object_set_number(json_obj, IMAGE_ROTATION, image_rotation);
  _image_rotation = image_rotation;
}

int PqSettings::ApplyEvCompensationVal(double ev_compensation_val) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorCameraEvCompensationProperty ev_compensation_prop = {
      .ev_compensation = (float)ev_compensation_val};
  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY,
      &ev_compensation_prop, sizeof(ev_compensation_prop));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Ev Compensation property failed to be set. Please use valid values "
        "for ev_compensation.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void PqSettings::StoreEvCompensationVal(float ev_compensation_val) {
  if (IsAlmostEqual(_ev_compensation_val, ev_compensation_val)) return;

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating EvCompensation");
  json_object_set_number(json_obj, EV_COMPENSATION, ev_compensation_val);
  _ev_compensation_val = ev_compensation_val;
}

int PqSettings::ApplyAeAntiFlickerMode(double ae_anti_flicker_mode) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorCameraAntiFlickerModeProperty ae_anti_flicker_mode_prop = {
      .anti_flicker_mode =
          (EdgeAppLibSensorCameraAntiFlickerMode)ae_anti_flicker_mode};
  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY,
      &ae_anti_flicker_mode_prop, sizeof(ae_anti_flicker_mode_prop));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Anti Flicker Mode property failed to be set. Please use valid values "
        "for ae_anti_flicker_mode.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void PqSettings::StoreAeAntiFlickerMode(int ae_anti_flicker_mode) {
  if (_ae_anti_flicker_mode == ae_anti_flicker_mode) return;

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating AeAntiFlickerMode");
  json_object_set_number(json_obj, AE_ANTI_FLICKER_MODE, ae_anti_flicker_mode);
  _ae_anti_flicker_mode = ae_anti_flicker_mode;
}

int PqSettings::ApplyWhiteBalanceMode(double white_balance_mode) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorWhiteBalanceModeProperty white_balance_mode_prop = {
      .mode = (EdgeAppLibSensorInferenceWhiteBalanceMode)white_balance_mode};
  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY,
      &white_balance_mode_prop, sizeof(white_balance_mode_prop));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "White Balance Mode property failed to be set. Please use valid values "
        "for white_balance_mode.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void PqSettings::StoreWhiteBalanceMode(int white_balance_mode) {
  if (_white_balance_mode == white_balance_mode) return;

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating WhiteBalanceMode");
  json_object_set_number(json_obj, WHITE_BALANCE_MODE, white_balance_mode);
  _white_balance_mode = white_balance_mode;
}

int PqSettings::ApplyGammaMode(double gamma_mode) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorInferenceGammaModeProperty gamma_mode_prop = {
      .gamma_mode = (EdgeAppLibSensorInferenceGammaMode)gamma_mode};
  int32_t result =
      SensorStreamSetProperty(stream, AITRIOS_SENSOR_GAMMA_MODE_PROPERTY_KEY,
                              &gamma_mode_prop, sizeof(gamma_mode_prop));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Gamma Mode property failed to be set. Please use valid values "
        "for gamma_mode.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  return result;
}

void PqSettings::StoreGammaMode(int gamma_mode) {
  if (_gamma_mode == gamma_mode) return;
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating GammaMode to mode %d", gamma_mode);
  json_object_set_number(json_obj, GAMMA_MODE, gamma_mode);
  _gamma_mode = gamma_mode;
  if (!_gamma_param) return;
  _gamma_param->gamma_mode = (EdgeAppLibSensorInferenceGammaMode)gamma_mode;
}

int PqSettings::ApplyGammaParameter(const char *gamma_parameter) {
  if (!gamma_parameter) {
    LOG_ERR("Null pointer for Gamma Parameter");
    return -1;
  }
  size_t max_string_len = b64e_size(AI_MODEL_GAMMA_PARAMETER_SIZE) + 1;
  size_t gamma_parameter_len = strnlen(gamma_parameter, max_string_len);
  if (gamma_parameter_len == max_string_len) {
    LOG_ERR("Gamma Parameter string too long or not null-terminated");
    return -1;
  }

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorInferenceGammaParameterProperty *gamma_param_prop =
      (EdgeAppLibSensorInferenceGammaParameterProperty *)malloc(
          sizeof(EdgeAppLibSensorInferenceGammaParameterProperty));
  if (!gamma_param_prop) {
    LOG_ERR("Malloc for gamma_param_prop failed.");
    return -1;
  }
  memset(gamma_param_prop, 0,
         sizeof(EdgeAppLibSensorInferenceGammaParameterProperty));
  gamma_param_prop->gamma_mode =
      (EdgeAppLibSensorInferenceGammaMode)_gamma_mode;
  gamma_param_prop->param_size =
      b64_decode((const unsigned char *)gamma_parameter, gamma_parameter_len,
                 gamma_param_prop->gamma_parameter);
  LOG_INFO("Real binary size: %u", gamma_param_prop->param_size);

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_GAMMA_PARAMETER_PROPERTY_KEY, gamma_param_prop,
      sizeof(EdgeAppLibSensorInferenceGammaParameterProperty));

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Gamma Parameter property failed to be set. Please use valid values "
        "for gamma_parameter.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }
  free(gamma_param_prop);
  return result;
}

void PqSettings::StoreGammaParameter(uint8_t *bin, uint32_t bin_size) {
  if (!_gamma_param || !bin || bin_size == 0) return;

  if (bin_size == _gamma_param->param_size &&
      !memcmp(bin, _gamma_param->gamma_parameter, bin_size))
    return;

  size_t encoded_buffer_size = b64e_size(bin_size) + 1;
  unsigned char *encoded_buffer = (unsigned char *)malloc(encoded_buffer_size);
  if (!encoded_buffer) {
    LOG_ERR("Malloc failed for encoded_buffer");
    return;
  }

  memset(encoded_buffer, 0, encoded_buffer_size);
  b64_encode((const unsigned char *)bin, bin_size, encoded_buffer);

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  LOG_INFO("Updating GammaParameter");
  json_object_set_string(json_obj, GAMMA_PARAMETER,
                         (const char *)encoded_buffer);
  memset(_gamma_param->gamma_parameter, 0, _gamma_param->param_size);
  memcpy(_gamma_param->gamma_parameter, (const uint8_t *)bin, bin_size);
  _gamma_param->param_size = bin_size;

  free(encoded_buffer);
}

CameraImageSize *PqSettings::GetCameraImageSize() { return &camera_image_size; }

CameraImageFlip *PqSettings::GetCameraImageFlip() { return &camera_image_flip; }

FrameRate *PqSettings::GetFrameRate() { return &frame_rate; }

AutoExposure *PqSettings::GetAutoExposure() { return &auto_exposure; }

AutoExposureMetering *PqSettings::GetAutoExposureMetering() {
  return &auto_exposure_metering;
}

ManualExposure *PqSettings::GetManualExposure() { return &manual_exposure; }

AutoWhiteBalance *PqSettings::GetAutoWhiteBalance() {
  return &auto_white_balance;
}

ManualWhiteBalancePreset *PqSettings::GetManualWhiteBalancePreset() {
  return &manual_white_balance_preset;
}

ImageCropping *PqSettings::GetImageCropping() { return &image_cropping; }

RegisterAccessArray *PqSettings::GetRegisterAccessArray() {
  return &register_access_array;
}
