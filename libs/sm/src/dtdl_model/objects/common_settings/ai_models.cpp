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

#include "dtdl_model/objects/common_settings/ai_models.hpp"

#include <assert.h>
#include <string.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

AiModels::AiModels() {
  json_array = json_value_get_array(json_value_init_array());
  JSON_Value_Type type =
      json_value_get_type(json_array_get_wrapping_value(json_array));
}

AiModels::~AiModels() {
  if (json_array) {
    for (int i = 0; i < ai_model_array_count; i++) {
      delete ai_model_array[i];
      ai_model_array[i] = nullptr;
    }
    ai_model_array_count = 0;
  }
}

int AiModels::Verify(JSON_Array *array) {
  int32_t result = 0;

  size_t array_count = json_array_get_count(array);
  if (array_count > MAX_AI_MODELS_COUNT) {
    char msg[LOGBUGSIZE];
    snprintf(msg, LOGBUGSIZE, "ai_model array (%zu) over max length (%d).",
             array_count, MAX_AI_MODELS_COUNT);
    LOG_WARN(msg);
    result = -1;
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(msg);
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    return result;
  }

  AiModel *verifier = nullptr;
  if (ai_model_array_count == 0) {
    verifier = new AiModel();
  } else {
    verifier = ai_model_array[0];
  }
  int ret2 = 0;
  for (size_t k = 0; k < array_count; k++) {
    ret2 = verifier->Verify(json_array_get_object(array, k));
    if (ret2 == -1) {
      LOG_WARN("ai_model array (%d) verify failed", k);
      result = -1;
      break;
    }
  }

  if (ai_model_array_count == 0) {
    verifier->Delete();
    delete verifier;
  }

  return result;
}

int AiModels::Apply(JSON_Array *array) {
  int32_t result = 0;
  LOG_INFO("AiModels::Apply enters");

  // clear old state json
  json_array_clear(json_array);

  // apply new items
  size_t array_count = json_array_get_count(array);
  size_t valid_count = array_count;
  if (valid_count > MAX_AI_MODELS_COUNT) {
    valid_count = MAX_AI_MODELS_COUNT;
  }
  int ret2 = 0;
  size_t k = 0;
  for (; k < valid_count; k++) {
    if (k >= ai_model_array_count) {
      // append new item
      AiModel *ai_model = new AiModel();
      ai_model->InitializeValues();

      ai_model_array[k] = ai_model;
      ai_model_array_count = k + 1;
    } else {
      // reuse item
      ai_model_array[k]->Reuse();
    }

    ai_model_array_index = k;
    ret2 = ai_model_array[k]->Apply(json_array_get_object(array, k));
    if (ret2 == -1) {
      LOG_WARN("ai_model array (%d) apply failed", k);
      result = -1;
      // preserve as configuration information, but not append to state json
      ai_model_array[k]->SetFailed();
    } else {
      json_array_append_value(
          json_array,
          json_object_get_wrapping_value(ai_model_array[k]->GetJsonObject()));
    }
  }

  // remove old unused items
  for (; k < ai_model_array_count; k++) {
    delete ai_model_array[k];
    ai_model_array[k] = nullptr;
  }
  ai_model_array_count = valid_count;

  LOG_INFO("AiModels::Apply exits");
  return result;
}

void AiModels::StoreValue(const char *name, const char *target,
                          const char *url_path, const char *hash) {
  if (ai_model_array_index >= ai_model_array_count) {
    LOG_WARN("ai_model_array_index(%d) >= ai_model_array_count(%d)",
             ai_model_array_index, ai_model_array_count);
    return;
  }

  ai_model_array[ai_model_array_index]->StoreValue(name, target, url_path,
                                                   hash);
}
