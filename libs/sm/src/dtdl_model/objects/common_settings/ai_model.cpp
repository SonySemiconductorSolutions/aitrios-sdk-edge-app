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

char *AiModel::get_filename_from_url(const char *url) {
  if (url == nullptr) return nullptr;

  const char *last_slash = strrchr(url, '/');
  if (last_slash == nullptr || *(last_slash + 1) == '\0') return nullptr;

  const char *filename = last_slash + 1;

  // SAS query string may be appended after '?'
  const char *qmark = strchr(filename, '?');
  size_t len = qmark ? (size_t)(qmark - filename) : strlen(filename);

  char *clean_name = (char *)malloc(len + 1);
  if (!clean_name) return nullptr;

  strncpy(clean_name, filename, len);
  clean_name[len] = '\0';

  return clean_name;
}

int AiModel::Apply(JSON_Object *obj) {
  int32_t result = 0;
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
    result = -1;
    LOG_ERR("Some property missing");
    DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();
    dtdl->GetResInfo()->SetDetailMsg(
        "Some AI model property missing. Please set valid values for name, "
        "target, url_path, and hash.");
  }

  json_object_set_string(json_obj, NAME, name);
  json_object_set_string(json_obj, TARGET, target);
  json_object_set_string(json_obj, URL_PATH, url_path);
  json_object_set_string(json_obj, HASH, hash);

  LOG_INFO("name: %s", name);
  LOG_INFO("target: %s", target);
  LOG_INFO("url_path: %s", url_path);
  LOG_INFO("hash: %s", hash);

  /*
  EdgeAppLibReceiveDataInfo info;
  info.filename = get_filename_from_url(url_path);
  info.filenamelen = strlen(info.filename);
  info.url = strdup(url_path);
  info.urllen = strlen(url_path);
  info.hash = strdup(hash);
  EdgeAppLibReceiveDataResult ret = EdgeAppLibReceiveData(&info, -1);
  if (ret != EdgeAppLibReceiveDataResultSuccess) {
    LOG_ERR("EdgeAppLibReceiveDatafailed with EdgeAppLibReceiveDataResult: %d",
            ret);
    return -1;
  }

  free(info.filename);
  free(info.url);
  free(info.hash);
  */

  LOG_INFO("AiModel::Apply exits");
  return result;
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
