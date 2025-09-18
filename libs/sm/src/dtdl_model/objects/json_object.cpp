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

#include "json_object.hpp"

#include <assert.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "sm_context.hpp"

#define BUFSIZE 256

JsonObject::JsonObject() {
  json_obj = json_value_get_object(json_value_init_object());
  assert(json_obj); /* LCOV_EXCL_LINE: error check */
}

void JsonObject::SetValidations(Validation *validations, int validations_size) {
  this->validations = validations;
  this->validations_size = validations_size;
}

void JsonObject::SetProperties(Property *properties, int properties_size) {
  this->properties = properties;
  this->properties_size = properties_size;
}

int JsonObject::Verify(JSON_Object *obj) {
  char msg[BUFSIZE];
  bool is_valid = true;
  for (int i = 0; is_valid; ++i) {
    const char *name = json_object_get_name(obj, i);
    if (name == nullptr) break;

    for (int j = 0; j < properties_size; ++j) {
      if (strcmp(name, properties[j].property) == 0 &&
          properties[j].obj->Verify(json_object_get_object(obj, name)) != 0)
        return -1;
    }

    for (int j = 0; j < validations_size && is_valid; ++j) {
      if (strcmp(name, validations[j].property) == 0) {
        double value = json_object_get_number(obj, name);
        LOG_DBG("%s = %f", name, value);
        /* LCOV_EXCL_START */
        if ((validations[j].validation == kGt &&
             value <= validations[j].value) ||
            (validations[j].validation == kGe &&
             value < validations[j].value) ||
            (validations[j].validation == kLt &&
             value >= validations[j].value) ||
            (validations[j].validation == kLe &&
             value > validations[j].value) ||
            (validations[j].validation == kNe &&
             value == validations[j].value)) {
          snprintf(msg, BUFSIZE, "%s not %s %f", name,
                   ConstraintStr[validations[j].validation],
                   validations[j].value);
          /* LCOV_EXCL_STOP */
          is_valid = false;
        } else if ((validations[j].validation == kType &&
                    !json_object_has_value_of_type(obj, name,
                                                   validations[j].value))) {
          // using relative position of kType
          snprintf(msg, BUFSIZE, "%s not of type %s", name,
                   JSONTypesStr[validations[j].validation - kType]);
          is_valid = false;
        }
      }
    }
  }
  if (!is_valid) {
    ResInfo *res_info =
        StateMachineContext::GetInstance(nullptr)->GetDtdlModel()->GetResInfo();
    res_info->SetDetailMsg(msg);
    res_info->SetCode(CODE_INVALID_ARGUMENT);
    LOG_DBG("invalid param: %s", msg);
    return -1;
  }
  return 0;
}

int JsonObject::Apply(JSON_Object *obj) {
  int ret = 0;
  int ret2 = 0;
  for (int i = 0;; ++i) {
    const char *name = json_object_get_name(obj, i);
    if (name == nullptr) break;
    LOG_INFO("Applying json object %s.", name);
    for (int j = 0; j < properties_size; ++j) {
      if (strcmp(name, properties[j].property) == 0) {
        JSON_Value *value = json_object_get_value(obj, name);
        if (value == nullptr) continue;

        ret2 = properties[j].obj->Apply(json_object_get_object(obj, name));
        if (ret2 != 0) {
          ret = -1;
        }
      }
    }
  }
  return ret;
}

void JsonObject::Delete() {
  json_value_free(json_object_get_wrapping_value(json_obj));
}
