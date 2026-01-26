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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dtdl_model/objects/common_settings.hpp"
#include "mock_receive_data.hpp"
#include "parson.h"
#include "sm_context.hpp"

#define AM_CONFIG_0 \
  "{\"name\":\"ai_model\",\"target\":\"cpu\",\"url_path\":\"path_string\", \
  \"hash\":\"1234\"}"
#define AM_CONFIG_1 "{}"

TEST(AiModel, Verify) {
  AiModel am;
  am.InitializeValues();
  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();

  JSON_Value *value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Verify(json_object(value)), 0);
  json_value_free(value);

  value = json_parse_string(AM_CONFIG_1);
  ASSERT_EQ(am.Verify(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(),
               "Some register access property missing. Please set valid values "
               "for name, target, url_path and hash.");
  json_value_free(value);

  am.Delete();
}

TEST(AiModel, Apply) {
  AiModel am;
  am.InitializeValues();
  DtdlModel *dtdl = StateMachineContext::GetInstance(nullptr)->GetDtdlModel();

  setReceiveDataResult(EdgeAppLibReceiveDataResultSuccess);
  JSON_Value *value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Apply(json_object(value)), 0);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(), "");
  json_value_free(value);
  resetReceiveDataResult();

  setReceiveDataResult(EdgeAppLibReceiveDataResultSuccess);
  value = json_parse_string(AM_CONFIG_1);
  ASSERT_EQ(am.Apply(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(),
               "Some AI model property missing. Please set valid values for "
               "name, target, url_path, and hash.");
  json_value_free(value);
  resetReceiveDataResult();

  setReceiveDataResult(EdgeAppLibReceiveDataResultTimeout);
  value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Apply(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(), "ReceiveDataAwait timeout.");
  json_value_free(value);
  resetReceiveDataResult();

  setReceiveDataResult(EdgeAppLibReceiveDataResultUninitialized);
  value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Apply(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(),
               "EVP client or workspace is not initialized.");
  json_value_free(value);
  resetReceiveDataResult();

  setReceiveDataResult(EdgeAppLibReceiveDataResultDenied);
  value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Apply(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(), "EVP_BLOB_CALLBACK denied.");
  json_value_free(value);
  resetReceiveDataResult();

  setReceiveDataResult(EdgeAppLibReceiveDataResultDataTooLarge);
  value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Apply(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(), "map_set or malloc failed.");
  json_value_free(value);
  resetReceiveDataResult();

  setReceiveDataResult(EdgeAppLibReceiveDataResultInvalidParam);
  value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Apply(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(),
               "Invalid parameters for EdgeAppLibReceiveData.");
  json_value_free(value);
  resetReceiveDataResult();

  setReceiveDataResult(EdgeAppLibReceiveDataResultFailure);
  value = json_parse_string(AM_CONFIG_0);
  ASSERT_EQ(am.Apply(json_object(value)), -1);
  ASSERT_STREQ(dtdl->GetResInfo()->GetDetailMsg(),
               "EVP_blobOperation AI Model Download failed.");
  json_value_free(value);
  resetReceiveDataResult();

  am.Delete();
}
