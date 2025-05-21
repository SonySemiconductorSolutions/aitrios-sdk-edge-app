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

#include "sm_utils.hpp"

#include <edgeapp/log.h>
#include <edgeapp/sensor.h>
#include <stdlib.h>

#include <cstdint>

#include "data_processor_utils.hpp"

#define BUFSIZE 128

using namespace EdgeAppLib;

void PrintSensorError() {
  uint32_t length = BUFSIZE;
  char message_buffer[BUFSIZE] = {0};
  SensorGetLastErrorString(
      EdgeAppLibSensorStatusParam::AITRIOS_SENSOR_STATUS_PARAM_MESSAGE,
      message_buffer, &length);

  LOG_ERR("level: %d - cause: %d - message: %s", SensorGetLastErrorLevel(),
          SensorGetLastErrorCause(), message_buffer);
}

int SetEdgeAppLibNetwork(EdgeAppLibSensorStream stream, JSON_Object *json) {
  const char *ai_model_bundle_id_str =
      json_object_get_string(json, "ai_model_bundle_id");
  if (!ai_model_bundle_id_str) {
    LOG_WARN("AI model bundle ID is not available");
    return -1;
  }
  struct EdgeAppLibSensorAiModelBundleIdProperty ai_model_bundle;
  size_t id_length = strlen(ai_model_bundle_id_str);
  if (id_length >= sizeof(ai_model_bundle.ai_model_bundle_id)) {
    LOG_ERR("AI model bundle ID is too long");
    return -1;
  }
  snprintf(ai_model_bundle.ai_model_bundle_id,
           sizeof(ai_model_bundle.ai_model_bundle_id), "%s",
           ai_model_bundle_id_str);

  LOG_DBG("Copied AI model bundle ID: %s", ai_model_bundle.ai_model_bundle_id);
  if (SensorStreamSetProperty(
          stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
          &ai_model_bundle,
          sizeof(struct EdgeAppLibSensorAiModelBundleIdProperty)) < 0) {
    LOG_ERR("Error while setting desired AI model bundle ID");
    PrintSensorError();
    return -1;
  }
  LOG_INFO("Successfully set ai bundle id %s",
           ai_model_bundle.ai_model_bundle_id);
  return 0;
}
