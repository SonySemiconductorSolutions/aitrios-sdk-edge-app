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

#ifndef DTDL_MODEL_OBJECTS_JSON_OBJECT_HPP
#define DTDL_MODEL_OBJECTS_JSON_OBJECT_HPP

#include "log.h"
#include "macros.h"
#include "parson.h"

typedef enum { kGt = 0, kGe, kLt, kLe, kNe, kType, kCount } Constraint;

static const char *ConstraintStr[] = {">", ">=", "<", "<=", "!="};
static const char *JSONTypesStr[] = {"number", "string"};
static_assert(sizeof(ConstraintStr) == sizeof(char *) * (kCount - 1));

typedef struct {
  const char *property;
  Constraint validation;
  double value;
} Validation;

class JsonObject;

typedef struct {
  const char *property;
  JsonObject *obj;
} Property;

class JsonObject {
 public:
  JsonObject();

  void SetValidations(Validation *validations, int validations_size);
  void SetProperties(Property *properties, int properties_size);

  /**
   * @brief Verifies if the given JSON object represents a valid DTDL object.
   *
   * @param obj Pointer to a JSON_Object to verify.
   * @return int Returns 0 if the object is valid, -1 otherwise.
   */
  virtual int Verify(JSON_Object *obj);

  /**
   * @brief Applies changes from the provided DTDL object to the internal
   * representation. This method assumes that the JSON object has been
   * previously verified.
   *
   * @param obj Pointer to a JSON_Object containing DTDL changes.
   * @return int Returns 0 if applying successful, -1 otherwise.
   */
  virtual int Apply(JSON_Object *obj);

  /**
   * @brief Remove internal representation.
   */
  void Delete();

  JSON_Object *GetJsonObject() const { return json_obj; }

 protected:
  JSON_Object *json_obj = nullptr;

  /**
   * @brief List of validations to apply to subproperties
   *
   */
  Validation *validations = nullptr;
  int validations_size = 0;
  /**
   * @brief List of properties
   *
   */
  Property *properties = nullptr;
  int properties_size = 0;
};

#endif /* DTDL_MODEL_OBJECTS_JSON_OBJECT_HPP */
