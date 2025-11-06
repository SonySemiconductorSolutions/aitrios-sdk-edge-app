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

#include "sm_api.hpp"

#include <string.h>

#include "log.h"
#include "sm_context.hpp"

static inline bool compare_string(const char *property_key,
                                  const char *target_key) {
  return strcmp(property_key, target_key) == 0;
}

void updateProperty(EdgeAppLibSensorStream stream, const char *property_key,
                    const void *value, size_t value_size) {
  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
  PqSettings *pq = dtdl->GetCommonSettings()->GetPqSettings();
  if (compare_string(property_key,
                     AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraImageSizeProperty *camera_size =
        (EdgeAppLibSensorCameraImageSizeProperty *)value;
    pq->GetCameraImageSize()->StoreValue(
        camera_size->width, camera_size->height, camera_size->scaling_policy);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraImageFlipProperty *camera_flip =
        (EdgeAppLibSensorCameraImageFlipProperty *)value;
    pq->GetCameraImageFlip()->StoreValue(camera_flip->flip_horizontal,
                                         camera_flip->flip_vertical);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraDigitalZoomProperty *digital_zoom =
        (EdgeAppLibSensorCameraDigitalZoomProperty *)value;
    pq->StoreDigitalZoom(digital_zoom->magnification);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraExposureModeProperty *exposure_mode =
        (EdgeAppLibSensorCameraExposureModeProperty *)value;
    pq->StoreExposureMode(exposure_mode->mode);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraAutoExposureProperty *auto_exp =
        (EdgeAppLibSensorCameraAutoExposureProperty *)value;
    pq->GetAutoExposure()->StoreValue(
        auto_exp->max_exposure_time, auto_exp->min_exposure_time,
        auto_exp->max_gain, auto_exp->convergence_speed);
  } else if (compare_string(
                 property_key,
                 AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraAutoExposureMeteringProperty *auto_exposure_metering =
        (EdgeAppLibSensorCameraAutoExposureMeteringProperty *)value;
    pq->GetAutoExposureMetering()->StoreValue(
        auto_exposure_metering->mode, auto_exposure_metering->top,
        auto_exposure_metering->left, auto_exposure_metering->bottom,
        auto_exposure_metering->right);
  } else if (compare_string(
                 property_key,
                 AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraEvCompensationProperty *ev_componsation =
        (EdgeAppLibSensorCameraEvCompensationProperty *)value;
    pq->StoreEvCompensationVal(ev_componsation->ev_compensation);
  } else if (compare_string(
                 property_key,
                 AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraAntiFlickerModeProperty *anti_flicker =
        (EdgeAppLibSensorCameraAntiFlickerModeProperty *)value;
    pq->StoreAeAntiFlickerMode(anti_flicker->anti_flicker_mode);
  } else if (compare_string(
                 property_key,
                 AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraManualExposureProperty *manual_exposure =
        (EdgeAppLibSensorCameraManualExposureProperty *)value;
    pq->GetManualExposure()->StoreValue(manual_exposure->exposure_time,
                                        manual_exposure->gain);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraFrameRateProperty *frameRate =
        (EdgeAppLibSensorCameraFrameRateProperty *)value;
    pq->GetFrameRate()->StoreValue(frameRate->num, frameRate->denom);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_ISP_FRAME_RATE_PROPERTY_KEY)) {
    EdgeAppLibSensorCameraFrameRateProperty *isp_frame_rate =
        (EdgeAppLibSensorCameraFrameRateProperty *)value;
    pq->GetFrameRate()->StoreValue(isp_frame_rate->num, isp_frame_rate->denom);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY)) {
    EdgeAppLibSensorWhiteBalanceModeProperty *white_balance_mode =
        (EdgeAppLibSensorWhiteBalanceModeProperty *)value;
    pq->StoreWhiteBalanceMode(white_balance_mode->mode);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY)) {
    EdgeAppLibSensorAutoWhiteBalanceProperty *auto_white_balance =
        (EdgeAppLibSensorAutoWhiteBalanceProperty *)value;
    pq->GetAutoWhiteBalance()->StoreValue(
        auto_white_balance->convergence_speed);
  } else if (compare_string(
                 property_key,
                 AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY)) {
    EdgeAppLibSensorManualWhiteBalancePresetProperty *manual_wb_preset =
        (EdgeAppLibSensorManualWhiteBalancePresetProperty *)value;
    pq->GetManualWhiteBalancePreset()->StoreValue(
        manual_wb_preset->color_temperature);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY)) {
    EdgeAppLibSensorImageCropProperty *image_crop =
        (EdgeAppLibSensorImageCropProperty *)value;
    pq->GetImageCropping()->StoreValue(image_crop->left, image_crop->top,
                                       image_crop->width, image_crop->height);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY)) {
    EdgeAppLibSensorImageRotationProperty *image_rotation =
        (EdgeAppLibSensorImageRotationProperty *)value;
    pq->StoreImageRotation(image_rotation->rotation_angle);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY)) {
    EdgeAppLibSensorRegisterAccess8Property *register_access =
        (EdgeAppLibSensorRegisterAccess8Property *)value;
    pq->GetRegisterAccessArray()->StoreValue(
        register_access->id, register_access->address, register_access->data,
        AITRIOS_SENSOR_REGISTER_8BIT);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY)) {
    EdgeAppLibSensorRegisterAccess16Property *register_access =
        (EdgeAppLibSensorRegisterAccess16Property *)value;
    pq->GetRegisterAccessArray()->StoreValue(
        register_access->id, register_access->address, register_access->data,
        AITRIOS_SENSOR_REGISTER_16BIT);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY)) {
    EdgeAppLibSensorRegisterAccess32Property *register_access =
        (EdgeAppLibSensorRegisterAccess32Property *)value;
    pq->GetRegisterAccessArray()->StoreValue(
        register_access->id, register_access->address, register_access->data,
        AITRIOS_SENSOR_REGISTER_32BIT);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY)) {
    EdgeAppLibSensorRegisterAccess64Property *register_access =
        (EdgeAppLibSensorRegisterAccess64Property *)value;
    pq->GetRegisterAccessArray()->StoreValue(
        register_access->id, register_access->address, register_access->data,
        AITRIOS_SENSOR_REGISTER_64BIT);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY)) {
    // inside custom_settings, set by the user
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_GAMMA_MODE_PROPERTY_KEY)) {
    EdgeAppLibSensorInferenceGammaModeProperty *gamma_mode =
        (EdgeAppLibSensorInferenceGammaModeProperty *)value;
    pq->StoreGammaMode(gamma_mode->gamma_mode);
  } else if (compare_string(property_key,
                            AITRIOS_SENSOR_GAMMA_PARAMETER_PROPERTY_KEY)) {
    EdgeAppLibSensorInferenceGammaParameterProperty *gamma_param =
        (EdgeAppLibSensorInferenceGammaParameterProperty *)value;
    pq->StoreGammaParameter(gamma_param->gamma_parameter,
                            gamma_param->param_size);
  } else {
    LOG_INFO("Unknown property: %s", property_key);
  }
}

void updateCustomSettings(void *state, int statelen) {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->GetDtdlModel()->GetCustomSettings()->Store(state, statelen);
}

JSON_Object *getPortSettings(void) {
  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
  JSON_Object *portSettings =
      dtdl->GetCommonSettings()->GetPortSettings()->GetJsonObject();
  return portSettings;
}
JSON_Object *getCodecSettings(void) {
  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
  JSON_Object *codeSettings =
      dtdl->GetCommonSettings()->GetCodecSettings()->GetJsonObject();
  return codeSettings;
}

uint32_t getNumOfInfPerMsg(void) {
  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
  return dtdl->GetCommonSettings()->getNumOfInfPerMsg();
}

EdgeAppLibSensorStream GetSensorStream(void) {
  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();
  return stream;
}
