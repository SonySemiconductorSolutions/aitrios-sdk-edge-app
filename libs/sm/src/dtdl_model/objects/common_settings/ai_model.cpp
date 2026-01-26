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
#include "ai_model.hpp"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "dtdl_model/properties.h"
#include "log.h"
#include "receive_data.h"
#include "sensor.h"
#include "sm_context.hpp"
#include "utils.hpp"

using namespace EdgeAppLib;

static const char *NAME = "name";
static const char *TARGET = "target";
static const char *URL_PATH = "url_path";
static const char *HASH = "hash";

AiModel::AiModel() {
  static Validation s_validations[4] = {
      {NAME, kType, JSONString},
      {TARGET, kType, JSONString},
      {URL_PATH, kType, JSONString},
      {HASH, kType, JSONString},
  }; /* LCOV_EXCL_BR_LINE: array check */

  SetValidations(s_validations, sizeof(s_validations) / sizeof(Validation));
}

void AiModel::InitializeValues() {
  json_object_set_string(json_obj, NAME, "");
  json_object_set_string(json_obj, TARGET, "");
  json_object_set_string(json_obj, URL_PATH, "");
  json_object_set_string(json_obj, HASH, "");
}

int AiModel::Verify(JSON_Object *obj) {
  int result = JsonObject::Verify(obj);
  if (!result) {
    if (!json_object_has_value(obj, NAME) ||
        !json_object_has_value(obj, TARGET) ||
        !json_object_has_value(obj, URL_PATH) ||
        !json_object_has_value(obj, HASH)) {
      LOG_ERR("Some property missing");
      result = -1;
      DtdlModel *dtdl =
          StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
      dtdl->GetResInfo()->SetDetailMsg(
          "Some register access property missing. Please set valid "
          "values for name, target, url_path and hash.");
      dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    }
  }

  return result;
}

int AiModel::Apply(JSON_Object *obj) {
  LOG_INFO("AiModel::Apply enters");

  name = json_object_has_value(obj, NAME) ? json_object_get_string(obj, NAME)
                                          : nullptr;
  target = json_object_has_value(obj, TARGET)
               ? json_object_get_string(obj, TARGET)
               : nullptr;
  url_path = json_object_has_value(obj, URL_PATH)
                 ? json_object_get_string(obj, URL_PATH)
                 : nullptr;
  hash = json_object_has_value(obj, HASH) ? json_object_get_string(obj, HASH)
                                          : nullptr;

  if (!name || !target || !url_path || !hash) {
    LOG_ERR("Some property missing");
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Some AI model property missing. Please set valid values for name, "
        "target, url_path, and hash.");
    dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
    return -1;
  }

  json_object_set_string(json_obj, NAME, name);
  json_object_set_string(json_obj, TARGET, target);
  json_object_set_string(json_obj, URL_PATH, url_path);
  json_object_set_string(json_obj, HASH, hash);

  LOG_INFO("name: %s", name);
  LOG_INFO("target: %s", target);
  LOG_INFO("url_path: %s", url_path);
  LOG_INFO("hash: %s", hash);

  EdgeAppLibReceiveDataInfo info;
  info.filename = (char *)name;
  info.filenamelen = strlen(info.filename);
  info.url = strdup(url_path);
  info.urllen = strlen(url_path);
  info.hash = strdup(hash);
  EdgeAppLibReceiveDataResult ret = EdgeAppLibReceiveData(&info, 5000);
  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
  if (ret != EdgeAppLibReceiveDataResultSuccess) {
    LOG_ERR("EdgeAppLibReceiveData failed with EdgeAppLibReceiveDataResult: %d",
            ret);
    switch (ret) {
      case EdgeAppLibReceiveDataResultTimeout:
        dtdl->GetResInfo()->SetDetailMsg("ReceiveDataAwait timeout.");
        dtdl->GetResInfo()->SetCode(CODE_DEADLINE_EXCEEDED);
        break;
      case EdgeAppLibReceiveDataResultUninitialized:
        dtdl->GetResInfo()->SetDetailMsg(
            "EVP client or workspace is not initialized.");
        dtdl->GetResInfo()->SetCode(CODE_FAILED_PRECONDITION);
        break;
      case EdgeAppLibReceiveDataResultDenied:
        dtdl->GetResInfo()->SetDetailMsg("EVP_BLOB_CALLBACK denied.");
        dtdl->GetResInfo()->SetCode(CODE_CANCELLED);
        break;
      case EdgeAppLibReceiveDataResultDataTooLarge:
        dtdl->GetResInfo()->SetDetailMsg("map_set or malloc failed.");
        dtdl->GetResInfo()->SetCode(CODE_RESOURCE_EXHAUSTED);
        break;
      case EdgeAppLibReceiveDataResultInvalidParam:
        dtdl->GetResInfo()->SetDetailMsg(
            "Invalid parameters for EdgeAppLibReceiveData.");
        dtdl->GetResInfo()->SetCode(CODE_INVALID_ARGUMENT);
        break;
      default:
        dtdl->GetResInfo()->SetDetailMsg(
            "EVP_blobOperation AI Model Download failed.");
        dtdl->GetResInfo()->SetCode(CODE_INTERNAL);
    }
    return -1;
  }
  dtdl->GetResInfo()->SetDetailMsg("");
  dtdl->GetResInfo()->SetCode(CODE_OK);

  free(info.url);
  free(info.hash);
  LOG_INFO("AiModel::Apply exits");
  return 0;
}

void AiModel::StoreValue(const char *name, const char *target,
                         const char *url_path, const char *hash) {}

void AiModel::SetFailed() {
  if (!failed) {
    failed = true;
    // json_obj isn't used. So delete manually.
    Delete();
  }
}

void AiModel::Reuse() {
  // json_obj has been freed. So init manually.
  json_obj = json_value_get_object(json_value_init_object());
  failed = false;
}
