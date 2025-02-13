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
#include "mock_process_format.hpp"

#include <stdlib.h>

#include "log.h"
#include "memory_manager.hpp"
#include "time.h"

static ProcessFormatResult ProcessFormatMetaSuccess = kProcessFormatResultOk;
static const char *s_model_version_id = "1000";
static uint8_t s_jpeg_buffer[100];

void setProcessFormatMetaFail(ProcessFormatResult result) {
  ProcessFormatMetaSuccess = result;
}

void resetProcessFormatMetaSuccess() {
  ProcessFormatMetaSuccess = kProcessFormatResultOk;
}

void setProcessFormatMetaOutput(const char *model_version_id) {
  s_model_version_id = model_version_id;
}

ProcessFormatResult ProcessFormatMeta(void *in_data, uint32_t in_size,
                                      EdgeAppLibSendDataType datatype,
                                      uint64_t timestamp,
                                      JSON_Value *output_tensor_value) {
  if (ProcessFormatMetaSuccess != kProcessFormatResultOk)
    return ProcessFormatMetaSuccess;

  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);

  // Set AI model bundle ID
  json_object_set_string(output_tensor_object, "ModelVersionID",
                         s_model_version_id);

  // Initialize one Inference for appending to "Inferences"
  JSON_Value *inf_value = json_value_init_object();
  JSON_Object *inf_object = json_value_get_object(inf_value);
  // Set "T"
  json_object_set_string(inf_object, "T", "19700101000000000");

  // Set "O" and "F"
  switch (datatype) {
    case EdgeAppLibSendDataBase64: {
      json_object_set_string(inf_object, "O", "abcdef");
      json_object_set_number(inf_object, "F", 0);
    } break;
    case EdgeAppLibSendDataJson:
      json_object_set_value(inf_object, "O", (JSON_Value *)in_data);
      json_object_set_number(inf_object, "F", 1);
      break;
    default:
      const char *error_msg = "Invalid datatype.";
      LOG_ERR("%s : datatype=%d", error_msg, datatype);
      return kProcessFormatResultInvalidParam;
  }
  // Set "inferences"
  JSON_Value *infs_json_array_value = json_value_init_array();
  JSON_Array *infs_json_array = json_value_get_array(infs_json_array_value);
  json_array_append_value(infs_json_array, inf_value);
  json_object_set_value(output_tensor_object, "Inferences",
                        infs_json_array_value);

  return kProcessFormatResultOk;
}

ProcessFormatResult ProcessFormatInput(MemoryRef data, uint32_t datalen,
                                       ProcessFormatImageType codec_number,
                                       uint64_t timestamp, void **jpeg_buffer,
                                       int32_t *jpeg_size) {
  // Simulate behavior based on codec_number
  if (codec_number == kProcessFormatImageTypeRaw) {
    // RAW: Pass the data directly
    *jpeg_buffer = s_jpeg_buffer;
    if (!*jpeg_buffer) {
      printf("ProcessFormatInput: Memory allocation failed for RAW data.\n");
      return kProcessFormatResultOther;
    }
    memcpy(*jpeg_buffer, data.u.p, datalen);
    *jpeg_size = datalen;
    return kProcessFormatResultOk;
  } else if (codec_number == kProcessFormatImageTypeJpeg) {
    // JPEG: Simulate JPEG encoding
    const int encoded_size = datalen / 2;  // Assume encoded JPEG is smaller
    *jpeg_buffer = s_jpeg_buffer;
    if (!*jpeg_buffer) {
      printf("ProcessFormatInput: Memory allocation failed for RAW data.\n");
      return kProcessFormatResultOther;
    }

    // Fill the buffer with mock JPEG data
    memset(*jpeg_buffer, 0xFF, encoded_size);
    *jpeg_size = encoded_size;
    return kProcessFormatResultOk;
  } else {
    // Unsupported codec type
    return kProcessFormatResultInvalidParam;
  }
}
