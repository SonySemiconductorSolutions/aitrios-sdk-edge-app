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

#include <assert.h>
#include <string.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

#define METADATA "metadata"
#define INPUT_TENSOR "input_tensor"

PortSettings::PortSettings()
    : metadata(PS_METADATA), input_tensor(PS_INFERENCE) {
  static Validation s_validations[] = {
      {.property = METADATA, .validation = kType, .value = JSONObject},
      {.property = INPUT_TENSOR, .validation = kType, .value = JSONObject},
  };
  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));

  json_object_set_value(
      json_obj, METADATA,
      json_object_get_wrapping_value(metadata.GetJsonObject()));
  json_object_set_value(
      json_obj, INPUT_TENSOR,
      json_object_get_wrapping_value(input_tensor.GetJsonObject()));
}

int PortSettings::Apply(JSON_Object *obj) {
  bool is_updated = false;
  if (json_object_has_value(obj, METADATA)) {
    JSON_Object *json_metadata = json_object_get_object(obj, METADATA);
    if (!json_value_equals(
            json_object_get_wrapping_value(json_metadata),
            json_object_get_wrapping_value(metadata.GetJsonObject()))) {
      metadata.Apply(json_metadata);
      is_updated = true;
    }
  }
  if (json_object_has_value(obj, INPUT_TENSOR)) {
    JSON_Object *json_input_tensor = json_object_get_object(obj, INPUT_TENSOR);
    if (!json_value_equals(
            json_object_get_wrapping_value(json_input_tensor),
            json_object_get_wrapping_value(input_tensor.GetJsonObject()))) {
      input_tensor.Apply(json_input_tensor);
      is_updated = true;
    }
  }
  if (!is_updated) return 0;

  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  context->EnableNotification();

  ResInfo *res_info = context->GetDtdlModel()->GetResInfo();

  if (!input_tensor.GetEnabled() && !metadata.GetEnabled()) {
    res_info->SetCode(CODE_INVALID_ARGUMENT);
    res_info->SetDetailMsg("Neither input tensor or metadata are enabled");
    return -1;
  }

  if (ApplyStreamChannels() != 0) {
    return -1;
  }

  return 0;
}

PortSetting *PortSettings::GetMetadata() { return &metadata; }
PortSetting *PortSettings::GetInputTensor() { return &input_tensor; }

int PortSettings::ApplyStreamChannels() {
  StateMachineContext *context = StateMachineContext::GetInstance(nullptr);
  EdgeAppLibSensorStream stream = context->GetSensorStream();

  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  SensorInputDataTypeEnableChannel(&enabled,
                                   AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT,
                                   metadata.GetEnabled());
  SensorInputDataTypeEnableChannel(
      &enabled, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE,
      input_tensor.GetEnabled());

  int32_t result = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY, &enabled,
      sizeof(enabled));
  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = context->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Input Data Type property failed to be set.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    return result;
  }

  return 0;
}
