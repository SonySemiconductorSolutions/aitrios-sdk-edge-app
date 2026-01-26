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
static const char *s_model_id = "1000";

void setProcessFormatMetaFail(ProcessFormatResult result) {
  ProcessFormatMetaSuccess = result;
}

void resetProcessFormatMetaSuccess() {
  ProcessFormatMetaSuccess = kProcessFormatResultOk;
}

void setProcessFormatMetaOutput(const char *model_id) { s_model_id = model_id; }

ProcessFormatResult ProcessFormatMeta(void *in_data, uint32_t in_size,
                                      EdgeAppLibSendDataType datatype,
                                      uint64_t timestamp, char *json_buffer,
                                      size_t buffer_size) {
  if (!json_buffer || buffer_size == 0) {
    LOG_ERR("Invalid JSON buffer.");
    return kProcessFormatResultInvalidParam;
  }

  size_t offset = 0;
  offset += snprintf(json_buffer + offset, buffer_size - offset,
                     "{\"ModelID\":\"%s\",", s_model_id);
  offset +=
      snprintf(json_buffer + offset, buffer_size - offset, "\"Inferences\":[{");

  offset += snprintf(json_buffer + offset, buffer_size - offset,
                     "\"T\":\"19700101000000000\",");
  switch (datatype) {
    case EdgeAppLibSendDataBase64:
      offset += snprintf(json_buffer + offset, buffer_size - offset,
                         "\"O\":\"abcdef\",\"F\":0");
      break;
    case EdgeAppLibSendDataJson:
      offset += snprintf(json_buffer + offset, buffer_size - offset,
                         "\"O\":\"%s\",\"F\":1", (char *)in_data);
      break;
    default:
      LOG_ERR("Invalid datatype: %d", datatype);
      return kProcessFormatResultInvalidParam;
  }

  offset += snprintf(json_buffer + offset, buffer_size - offset, "}]}");
  return ProcessFormatMetaSuccess;
}

ProcessFormatResult ProcessFormatInput(MemoryRef data, uint32_t datalen,
                                       ProcessFormatImageType codec_number,
                                       EdgeAppLibImageProperty *image_property,
                                       uint64_t timestamp, void **jpeg_buffer,
                                       int32_t *jpeg_size) {
  // Simulate behavior based on codec_number
  if (codec_number == kProcessFormatImageTypeRaw) {
    // RAW: Pass the data directly
    *jpeg_buffer = malloc(datalen);
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
    *jpeg_buffer = malloc(encoded_size);
    if (!*jpeg_buffer) {
      printf("ProcessFormatInput: Memory allocation failed for JPEG data.\n");
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
