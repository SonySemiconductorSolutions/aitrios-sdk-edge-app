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
#include "dtdl_model/objects/common_settings/pq_settings/register_access.hpp"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

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

RegisterAccess::RegisterAccess() {
  static Validation s_validations[8] = {
      {BIT_LENGTH, kType, JSONNumber},
      {BIT_LENGTH, kGe, 0},
      {BIT_LENGTH, kLe, 3},
      {ID, kGe, 0},
      {ID, kLe, 4294967295},
      {ID, kType, JSONNumber},
      {ADDRESS, kType, JSONString},
      {DATA, kType, JSONString},
  }; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void RegisterAccess::InitializeValues() {
  json_object_set_number(json_obj, BIT_LENGTH,
                         (EdgeAppLibSensorRegisterBitLength)0);
  json_object_set_number(json_obj, ID, 0);

  json_object_set_string(json_obj, ADDRESS, "");
  json_object_set_string(json_obj, DATA, "");
}

int RegisterAccess::Verify(JSON_Object *obj) {
  int result = JsonObject::Verify(obj);
  if (!result) {
    if (!json_object_has_value(obj, BIT_LENGTH) ||
        !json_object_has_value(obj, ID) ||
        !json_object_has_value(obj, ADDRESS) ||
        !json_object_has_value(obj, DATA)) {
      LOG_ERR("Some property missing");
      result = -1;
      DtdlModel *dtdl =
          StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
      dtdl->GetResInfo()->SetDetailMsg(
          "Some register access property missing. Please set valid "
          "values for bit_length, id, address and data.");
      dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    }
  }

  return result;
}

int RegisterAccess::Apply(JSON_Object *obj) {
  EdgeAppLibSensorRegisterAccessProperty aux_register_access_property =
      register_access_property;

  enum EdgeAppLibSensorRegisterBitLength bit_length =
      (EdgeAppLibSensorRegisterBitLength)0;
  if (json_object_has_value(obj, BIT_LENGTH)) {
    bit_length = (enum EdgeAppLibSensorRegisterBitLength)json_object_get_number(
        obj, BIT_LENGTH);
  }
  aux_register_access_property.id =
      json_object_has_value(obj, ID) ? json_object_get_number(obj, ID) : 0;
  aux_register_access_property.address =
      json_object_has_value(obj, ADDRESS)
          ? strtoull(json_object_get_string(obj, ADDRESS), NULL, 16)
          : 0;
  uint64_t data = json_object_has_value(obj, DATA)
                      ? strtoull(json_object_get_string(obj, DATA), NULL, 10)
                      : 0;

  EdgeAppLibSensorStream stream =
      StateMachineContext::GetInstance(nullptr)->GetSensorStream();
  int32_t result = 0;

  switch (bit_length) {
    case AITRIOS_SENSOR_REGISTER_8BIT: {
      aux_register_access_property.data.data8 = (uint8_t)data;
      EdgeAppLibSensorRegisterAccess8Property aux_register_access_8_property =
          register_access_8_property;
      aux_register_access_8_property.id = aux_register_access_property.id;
      aux_register_access_8_property.address =
          aux_register_access_property.address;
      aux_register_access_8_property.data =
          aux_register_access_property.data.data8;
      result = SensorStreamSetProperty(
          stream, AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY,
          &aux_register_access_8_property,
          sizeof(aux_register_access_8_property));
    } break;
    case AITRIOS_SENSOR_REGISTER_16BIT: {
      aux_register_access_property.data.data16 = (uint16_t)data;
      EdgeAppLibSensorRegisterAccess16Property aux_register_access_16_property =
          register_access_16_property;
      aux_register_access_16_property.id = aux_register_access_property.id;
      aux_register_access_16_property.address =
          aux_register_access_property.address;
      aux_register_access_16_property.data =
          aux_register_access_property.data.data16;
      result = SensorStreamSetProperty(
          stream, AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY,
          &aux_register_access_16_property,
          sizeof(aux_register_access_16_property));
    } break;
    case AITRIOS_SENSOR_REGISTER_32BIT: {
      aux_register_access_property.data.data32 = (uint32_t)data;
      EdgeAppLibSensorRegisterAccess32Property aux_register_access_32_property =
          register_access_32_property;
      aux_register_access_32_property.id = aux_register_access_property.id;
      aux_register_access_32_property.address =
          aux_register_access_property.address;
      aux_register_access_32_property.data =
          aux_register_access_property.data.data32;
      result = SensorStreamSetProperty(
          stream, AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY,
          &aux_register_access_32_property,
          sizeof(aux_register_access_32_property));
    } break;
    case AITRIOS_SENSOR_REGISTER_64BIT: {
      aux_register_access_property.data.data64 = (uint64_t)data;
      EdgeAppLibSensorRegisterAccess64Property aux_register_access_64_property =
          register_access_64_property;
      aux_register_access_64_property.id = aux_register_access_property.id;
      aux_register_access_64_property.address =
          aux_register_access_property.address;
      aux_register_access_64_property.data =
          aux_register_access_property.data.data64;
      result = SensorStreamSetProperty(
          stream, AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY,
          &aux_register_access_64_property,
          sizeof(aux_register_access_64_property));
    } break;
    default:
      LOG_ERR("Invalid bit_length is set");
      result = -1;
      break;
  }

  if (result != 0) {
    SmUtilsPrintSensorError();
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Register access property failed to be set. Please use valid "
        "values for bit_length, id, address and data.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
  }

  return result;
}

void RegisterAccess::StoreValue(uint32_t id, uint64_t address, uint64_t data,
                                int bit_length) {
  /* LCOV_EXCL_BR_START: check if initialized */
  if (register_access_property.id == id &&
      register_access_property.address == address &&
      register_access_property.bit_length == bit_length &&
      (bit_length == AITRIOS_SENSOR_REGISTER_8BIT &&
           register_access_property.data.data8 == (uint8_t)data ||
       bit_length == AITRIOS_SENSOR_REGISTER_16BIT &&
           register_access_property.data.data16 == (uint16_t)data ||
       bit_length == AITRIOS_SENSOR_REGISTER_32BIT &&
           register_access_property.data.data32 == (uint32_t)data ||
       bit_length == AITRIOS_SENSOR_REGISTER_64BIT &&
           register_access_property.data.data64 == (uint64_t)data)) {
  } else {
    StateMachineContext::GetInstance(nullptr)->EnableNotification();
  }
  /* LCOV_EXCL_BR_STOP */

  LOG_INFO("Updating RegisterAccessProperty");
  json_object_set_number(json_obj, BIT_LENGTH, bit_length);
  json_object_set_number(json_obj, ID, id);

  char address_str[17] = {};
  snprintf(address_str, 17, "%016" PRIX64, address);
  json_object_set_string(json_obj, ADDRESS, address_str);

  char data_str[21] = {};
  snprintf(data_str, 21, "%" PRIu64, data);
  json_object_set_string(json_obj, DATA, data_str);

  switch (bit_length) {
    case AITRIOS_SENSOR_REGISTER_8BIT: {
      register_access_property.data.data8 = (uint8_t)data;
    } break;
    case AITRIOS_SENSOR_REGISTER_16BIT: {
      register_access_property.data.data16 = (uint16_t)data;
    } break;
    case AITRIOS_SENSOR_REGISTER_32BIT: {
      register_access_property.data.data32 = (uint32_t)data;
    } break;
    case AITRIOS_SENSOR_REGISTER_64BIT: {
      register_access_property.data.data64 = (uint64_t)data;
    } break;
    default:
      LOG_ERR("Invalid bit_length is set");
      break;
  }
  register_access_property.id = id;
  register_access_property.address = address;
  register_access_property.bit_length =
      (EdgeAppLibSensorRegisterBitLength)bit_length;
}

void RegisterAccess::SetFailed() {
  if (!failed) {
    failed = true;
    // json_obj isn't used. So delete manually.
    Delete();
  }
}

void RegisterAccess::Reuse() {
  // json_obj has been freed. So init manually.
  json_obj = json_value_get_object(json_value_init_object());
  failed = false;
}
