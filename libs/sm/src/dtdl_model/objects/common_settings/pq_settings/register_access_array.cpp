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
#include <assert.h>
#include <stdlib.h>

#include "dtdl_model/objects/common_settings/pq_settings/register_access.hpp"
#include "dtdl_model/properties.h"
#include "log.h"
#include "register_access.hpp"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *BIT_LENGTH = "bit_length";
static const char *ID = "id";
static const char *ADDRESS = "address";
static const char *DATA = "data";

RegisterAccessArray::RegisterAccessArray() {
  json_array = json_value_get_array(json_value_init_array());
  JSON_Value_Type type =
      json_value_get_type(json_array_get_wrapping_value(json_array));
}

RegisterAccessArray::~RegisterAccessArray() {
  if (json_array) {
    for (int i = 0; i < register_access_array_count; i++) {
      delete register_access_array[i];
      register_access_array[i] = nullptr;
    }
    register_access_array_count = 0;
  }
}

int RegisterAccessArray::Verify(JSON_Array *array) {
  int32_t result = 0;

  size_t array_count = json_array_get_count(array);
  if (array_count > MAX_REGISTER_ACCESS_COUNT) {
    char msg[LOGBUGSIZE];
    snprintf(msg, LOGBUGSIZE,
             "register_access array (%zu) over max length (%d).", array_count,
             MAX_REGISTER_ACCESS_COUNT);
    LOG_WARN(msg);
    result = -1;
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(msg);
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    return result;
  }

  RegisterAccess *verifier = nullptr;
  if (register_access_array_count == 0) {
    verifier = new RegisterAccess();
  } else {
    verifier = register_access_array[0];
  }
  int ret2 = 0;
  for (size_t k = 0; k < array_count; k++) {
    ret2 = verifier->Verify(json_array_get_object(array, k));
    if (ret2 == -1) {
      LOG_WARN("register_access array (%d) verify failed", k);
      result = -1;
      break;
    }
  }

  if (register_access_array_count == 0) {
    verifier->Delete();
    delete verifier;
  }

  return result;
}

int RegisterAccessArray::Apply(JSON_Array *array) {
  int32_t result = 0;

  // clear old state json
  json_array_clear(json_array);

  // apply new items
  size_t array_count = json_array_get_count(array);
  size_t valid_count = array_count;
  if (valid_count > MAX_REGISTER_ACCESS_COUNT) {
    valid_count = MAX_REGISTER_ACCESS_COUNT;
  }
  int ret2 = 0;
  size_t k = 0;
  for (; k < valid_count; k++) {
    if (k >= register_access_array_count) {
      // append new item
      RegisterAccess *register_access = new RegisterAccess();
      register_access->InitializeValues();

      register_access_array[k] = register_access;
      register_access_array_count = k + 1;
    } else {
      // reuse item
      register_access_array[k]->Reuse();
    }

    register_access_array_index = k;
    ret2 = register_access_array[k]->Apply(json_array_get_object(array, k));
    if (ret2 == -1) {
      LOG_WARN("register_access array (%d) apply failed", k);
      result = -1;
      // preserve as configuration information, but not append to state json
      register_access_array[k]->SetFailed();
    } else {
      json_array_append_value(json_array,
                              json_object_get_wrapping_value(
                                  register_access_array[k]->GetJsonObject()));
    }
  }

  // remove old unused items
  for (; k < register_access_array_count; k++) {
    delete register_access_array[k];
    register_access_array[k] = nullptr;
  }
  register_access_array_count = valid_count;

  return result;
}

void RegisterAccessArray::StoreValue(uint32_t id, uint64_t address,
                                     uint64_t data, int bit_length) {
  if (register_access_array_index >= register_access_array_count) {
    LOG_WARN(
        "register_access_array_index(%d) >= register_access_array_count(%d)",
        register_access_array_index, register_access_array_count);
    return;
  }

  register_access_array[register_access_array_index]->StoreValue(
      id, address, data, bit_length);
}
