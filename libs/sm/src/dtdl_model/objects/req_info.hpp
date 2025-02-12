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

#ifndef DTDL_MODEL_OBJECTS_REQ_INFO_HPP
#define DTDL_MODEL_OBJECTS_REQ_INFO_HPP

#include <cstdint>

#include "dtdl_model/objects/json_object.hpp"

class ReqInfo : public JsonObject {
 public:
  ReqInfo();

  int Verify(JSON_Object *obj);
  int Apply(JSON_Object *obj);

  const char *GetReqId() const;
};

#endif /* DTDL_MODEL_OBJECTS_REQ_INFO_HPP */
