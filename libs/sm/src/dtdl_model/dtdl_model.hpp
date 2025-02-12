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

#ifndef DTDL_MODEL_HPP
#define DTDL_MODEL_HPP

#include "dtdl_model/objects/common_settings.hpp"
#include "dtdl_model/objects/custom_settings.hpp"
#include "dtdl_model/objects/json_object.hpp"
#include "dtdl_model/objects/req_info.hpp"
#include "dtdl_model/objects/res_info.hpp"
#include "macros.h"
#include "parson.h"
class DtdlModel : public JsonObject {
 public:
  DtdlModel();
  ~DtdlModel();

  /**
   * @brief Update the internal representation of the DTDL model based on the
   * provided JSON. If the update involves parameter changes, it may trigger
   * additional actions such as invoking APIs, updating state machines, etc.
   *
   * @param json Pointer to a JSON object in string format.
   * @return int Returns 0 on success, -1 otherwise.
   */
  UT_ATTRIBUTE int Update(void *json);
  int Verify(JSON_Object *obj);
  int Apply(JSON_Object *obj);

  /**
   * @brief Serialize the internal representation of the DTDL model to a JSON
   * string.
   *
   * @note Must be freed by the caller.
   *
   * @return char* Pointer to the serialized JSON string.
   */
  char *Serialize();

  void InitializeValues();

  ReqInfo *GetReqInfo();
  ResInfo *GetResInfo();
  UT_ATTRIBUTE CommonSettings *GetCommonSettings();
  UT_ATTRIBUTE CustomSettings *GetCustomSettings();

 private:
  ReqInfo req_info;
  ResInfo res_info;
  CommonSettings common_settings;
  CustomSettings custom_settings;
};

#endif /* DTDL_MODEL_HPP */
