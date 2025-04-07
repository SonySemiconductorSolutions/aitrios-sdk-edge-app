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

#include "process_format.hpp"

#include <stdlib.h>

#include "device.h"
#include "log.h"
#include "memory_manager.hpp"
#include "sensor.h"
#include "sm_api.hpp"
#include "time.h"
extern "C" {
#include "base64.h"
}

/**
 * @brief Handles raw format processing by mapping or reading memory.
 * @param in_data      Input memory reference containing data to be processed.
 * @param in_size      Size of the input data.
 * @param image        Pointer to store the output image data.
 * @param image_size   Pointer to store the size of the output image data.
 * @return ProcessFormatResult
 */
static ProcessFormatResult HandleRawFormat(MemoryRef in_data, size_t in_size,
                                           void **image, int32_t *image_size) {
  if (!image || !image_size) {
    LOG_ERR("Invalid input arguments.");
    return kProcessFormatResultInvalidParam;
  }

  if (in_data.type == MEMORY_MANAGER_MAP_TYPE) {
    *image = malloc(in_size);
    if (!*image) {
      LOG_ERR("Memory allocation failed.");
      return kProcessFormatResultOther;
    }
    memcpy(*image, in_data.u.p, in_size);
    *image_size = in_size;
  } else {
    *image = malloc(in_size);
    if (!*image) {
      LOG_ERR("Memory allocation failed.");
      return kProcessFormatResultOther;
    }

    /* Copy data from Himem using handle */
    size_t size = 0;
    EsfMemoryManagerResult mem_err =
        EsfMemoryManagerPread(in_data.u.esf_handle, *image, in_size, 0, &size);
    if (mem_err != kEsfMemoryManagerResultSuccess) {
      LOG_ERR("EsfMemoryManagerPread failed. %d", mem_err);
      free(*image);
      return kProcessFormatResultOther;
    }

    *image_size = size;
  }
  return kProcessFormatResultOk;
}

/**
 * @brief Initializes JPEG encoding parameters based on sensor stream
 * properties.
 * @param enc_info  Pointer to a structure to store JPEG encoding information.
 * @param enc_param Pointer to a structure to store JPEG encoding parameters.
 * @return true if the parameters are successfully initialized, false otherwise.
 */
static bool InitializeJpegEncodingParameters(EsfCodecJpegInfo *enc_info,
                                             EsfCodecJpegEncParam *enc_param) {
  // Retrieve the current sensor stream
  EdgeAppLibSensorStream stream = GetSensorStream();
  EdgeAppLibSensorImageProperty property = {};

  // Get image properties from the sensor stream
  int32_t ret = EdgeAppLib::SensorStreamGetProperty(
      stream, AITRIOS_SENSOR_IMAGE_PROPERTY_KEY, &property, sizeof(property));
  if (ret != 0) {
    LOG_ERR("SensorStreamGetProperty failed for %s",
            AITRIOS_SENSOR_IMAGE_PROPERTY_KEY);
    return false;
  }

  enc_info->width = property.width;
  enc_info->height = property.height;
  enc_info->stride = property.stride_bytes;
  enc_param->width = property.width;
  enc_param->height = property.height;
  enc_param->stride = property.stride_bytes;
  enc_param->quality = enc_info->quality = 80;

  // Set input format and calculate output buffer size based on pixel format
  if (strncmp(property.pixel_format, AITRIOS_SENSOR_PIXEL_FORMAT_RGB24,
              strlen(AITRIOS_SENSOR_PIXEL_FORMAT_RGB24)) == 0) {
    enc_info->input_fmt = kJpegInputRgbPacked_8;
    enc_param->input_fmt = kJpegInputRgbPacked_8;
    enc_param->out_buf.output_buf_size =
        property.stride_bytes * property.height;
  } else if (strncmp(property.pixel_format,
                     AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR,
                     strlen(AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR)) == 0) {
    enc_info->input_fmt = kJpegInputRgbPlanar_8;
    enc_param->input_fmt = kJpegInputRgbPlanar_8;
    enc_param->out_buf.output_buf_size =
        property.stride_bytes * property.height * 3;
  } else {
    LOG_ERR("Unsupported pixel format: %s",
            property.pixel_format);  // Log an error for unsupported formats
    return false;
  }

  return true;  // Return success if all parameters are initialized correctly
}

/**
 * @brief Handles JPEG format encoding for raw input data.
 * @param in_data      Input memory reference containing raw data.
 * @param in_size      Size of the input raw data.
 * @param image        Pointer to store the encoded JPEG image.
 * @param image_size   Pointer to store the size of the encoded JPEG image.
 */
static ProcessFormatResult HandleJpegFormat(MemoryRef in_data, size_t in_size,
                                            void **image, int32_t *image_size) {
  EsfCodecJpegInfo enc_info = {};
  EsfCodecJpegEncParam enc_param = {};

  // Validate input arguments
  if (!image || !image_size) {
    LOG_ERR("Invalid input arguments.");
    return kProcessFormatResultInvalidParam;
  }

  // Initialize JPEG encoding parameters
  if (!InitializeJpegEncodingParameters(&enc_info, &enc_param)) {
    LOG_ERR("Failed to initialize JPEG encoding parameters.");
    return kProcessFormatResultInvalidParam;
  }

  // Check if output buffer size is valid
  if (enc_param.out_buf.output_buf_size > in_size) {
    LOG_ERR("Invalid output buffer size.");
    return kProcessFormatResultMemoryError;
  }

  EsfCodecJpegError jpeg_err;
  if (in_data.type == MEMORY_MANAGER_MAP_TYPE) {
    // Set input and allocate output buffer for encoding
    enc_param.input_adr_handle = (uint64_t)(uintptr_t)in_data.u.p;
    enc_param.out_buf.output_adr_handle =
        (uint64_t)(uintptr_t)malloc(enc_param.out_buf.output_buf_size);
    LOG_WARN("JPEG encoding: input_adr_handle=%p, output_adr_handle=%p",
             (void *)enc_param.input_adr_handle,
             (void *)enc_param.out_buf.output_adr_handle);

    if (!enc_param.out_buf.output_adr_handle) {
      LOG_ERR("Memory allocation failed.");
      return kProcessFormatResultMemoryError;
    }

    // Perform JPEG encoding
    jpeg_err = EsfCodecEncodeJpeg(&enc_param, image_size);
    if (jpeg_err != kJpegSuccess) {
      LOG_ERR("EsfCodecEncodeJpeg failed. %d", jpeg_err);
      free((void *)(uintptr_t)enc_param.out_buf.output_adr_handle);
      return kProcessFormatResultOther;
    }

    *image = (void *)(uintptr_t)enc_param.out_buf.output_adr_handle;
  } else {
    EsfMemoryManagerHandle output_adr_handle;

    // Perform JPEG encoding with himem memory handle
    jpeg_err = EsfCodecJpegEncodeHandle(
        in_data.u.esf_handle, &output_adr_handle, &enc_info, image_size);

    if (jpeg_err != kJpegSuccess) {
      LOG_ERR("EsfCodecJpegEncodeHandle failed. %d", jpeg_err);
      return kProcessFormatResultOther;
    }

    size_t size = 0;
    *image = malloc(*image_size);
    if (!*image) {
      LOG_ERR("Memory allocation failed.");
      return kProcessFormatResultOther;
    }

    // Get encoded data into Wasm memory
    EsfMemoryManagerResult mem_err =
        EsfMemoryManagerPread(output_adr_handle, *image, *image_size, 0, &size);
    if (mem_err != kEsfMemoryManagerResultSuccess) {
      LOG_ERR("EsfMemoryManagerPread failed. %d", mem_err);
      free(*image);
      return kProcessFormatResultOther;
    }

    *image_size = size;
    // Release himem memory handle
    jpeg_err = EsfCodecJpegEncodeRelease(output_adr_handle);
    if (jpeg_err != kJpegSuccess) {
      LOG_ERR("EsfCodecJpegEncodeRelease failed. %d", jpeg_err);
      free(*image);
      return kProcessFormatResultOther;
    }
  }

  return kProcessFormatResultOk;
}

ProcessFormatResult ProcessFormatInput(MemoryRef in_data, uint32_t in_size,
                                       ProcessFormatImageType datatype,
                                       uint64_t timestamp, void **image,
                                       int32_t *image_size) {
  if (!image || !image_size) {
    LOG_ERR("Invalid input arguments.");
    return kProcessFormatResultInvalidParam;
  }
  if (in_data.type == MEMORY_MANAGER_MAP_TYPE && in_data.u.p == nullptr) {
    LOG_ERR("Invalid input data.");
    return kProcessFormatResultInvalidParam;
  }

  switch (datatype) {
    case kProcessFormatImageTypeRaw:
      return HandleRawFormat(in_data, in_size, image, image_size);

    case kProcessFormatImageTypeJpeg:
      return HandleJpegFormat(in_data, in_size, image, image_size);

    default:
      LOG_ERR("Invalid datatype.");
      return kProcessFormatResultInvalidParam;
  }
}

ProcessFormatResult ProcessFormatMeta(void *in_data, uint32_t in_size,
                                      EdgeAppLibSendDataType datatype,
                                      uint64_t timestamp, char *json_buffer,
                                      size_t buffer_size) {
  if (json_buffer == NULL) {
    LOG_ERR("Invalid json_buffer.");
    return kProcessFormatResultInvalidParam;
  }

  size_t offset = 0;

  // Get AI model version ID
  EdgeAppLibSensorStream stream = GetSensorStream();
  int32_t ret = -1;
  EdgeAppLibSensorInfoStringProperty sensor_name = {0};
  sensor_name.category = AITRIOS_SENSOR_INFO_STRING_SENSOR_NAME;
  if ((ret = EdgeAppLib::SensorStreamGetProperty(
           stream, AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY,
           (void *)&sensor_name, sizeof(sensor_name))) != 0) {
    const char *error_msg = "Error GET device name.";
    LOG_ERR("%s : SensorStreamGetProperty=%d", error_msg, ret);
    return kProcessFormatResultFailure;
  }
  EdgeAppLibSensorInfoStringProperty sensor_version_id = {0};
  if (!strncmp("IMX500", sensor_name.info, strlen("IMX500"))) {
    sensor_version_id.category = AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION;
  } else if (!strncmp("AI-ISP", sensor_name.info, strlen("AI-ISP"))) {
    sensor_version_id.category =
        AITRIOS_SENSOR_INFO_STRING_AIISP_AI_MODEL_VERSION;
  }
  if ((ret = EdgeAppLib::SensorStreamGetProperty(
           stream, AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY,
           (void *)&sensor_version_id, sizeof(sensor_version_id))) != 0) {
    const char *error_msg = "Error GET version id.";
    LOG_ERR("%s : SensorStreamGetProperty=%d", error_msg, ret);
    return kProcessFormatResultFailure;
  }
  // Set AI model bundle ID directly into the JSON buffer
  offset += snprintf(json_buffer + offset, buffer_size - offset,
                     "{\"ModelID\":\"%s\",", sensor_version_id.info);

  // Get Device ID
  char device_id[WASM_BINDING_DEVICEID_MAX_SIZE] = {0};
  if ((ret = EsfSystemGetDeviceID(device_id)) != kEsfDeviceIdResultOk) {
    const char *error_msg = "Error GET device id.";
    LOG_ERR("%s : EsfSystemGetDeviceID=%d", error_msg, ret);
    snprintf(device_id, sizeof(device_id), "000000000000000");
  }
  // Set Device ID
  offset += snprintf(json_buffer + offset, buffer_size - offset,
                     "\"DeviceID\":\"%s\",", device_id);

  // Get Image Flag
  bool image_flg = false;
  JSON_Object *object = getPortSettings();
  if (object && json_object_has_value(object, "input_tensor")) {
    JSON_Object *port_setting = json_object_get_object(object, "input_tensor");
    if (port_setting && json_object_has_value(port_setting, "enabled")) {
      image_flg = json_object_get_boolean(port_setting, "enabled");
    }
  }

  // Set Image Flag
  if (image_flg) {
    offset +=
        snprintf(json_buffer + offset, buffer_size - offset, "\"Image\":true,");
  } else {
    offset += snprintf(json_buffer + offset, buffer_size - offset,
                       "\"Image\":false,");
  }

  // Set "T"
  char inf_timestamp[32];
  // convert nanoseconds to milliseconds
  uint64_t timestamp_ms = timestamp / 1000000;
  // ...and to seconds, since time_t is in seconds
  time_t timestamp_sec = timestamp_ms / 1000;
  struct tm tm;
  gmtime_r(&timestamp_sec, &tm);
  int remaining_ms = timestamp_ms % 1000;
  int num = strftime(inf_timestamp, sizeof(inf_timestamp), "%Y%m%d%H%M%S", &tm);
  if (num > 0) {
    snprintf(inf_timestamp + num, sizeof(inf_timestamp) - num, "%03d",
             remaining_ms);
  }

  /* Add Inference result into Json */
  offset += snprintf(json_buffer + offset, buffer_size - offset,
                     "\"Inferences\":[{\"T\":\"%s\",", inf_timestamp);

  if (datatype == EdgeAppLibSendDataBase64) {
    int inf_o_size = b64e_size(in_size);
    offset += snprintf(json_buffer + offset, buffer_size - offset, "\"O\":\"");
    char *inf_o = json_buffer + offset;
    /* Memory in-place Base64 encoding */
    int encoded_size =
        b64_encode((unsigned char *)in_data, in_size, (unsigned char *)inf_o);
    offset += encoded_size;
    int written =
        snprintf(json_buffer + offset, buffer_size - offset, "\",\"F\":0}]}");
    if (written < 0 || (size_t)written >= buffer_size - offset) {
      LOG_ERR("Buffer overflow when writing final JSON.");
      return kProcessFormatResultMemoryError;
    }
    offset += written;
  } else if (datatype == EdgeAppLibSendDataJson) {
    int written = snprintf(json_buffer + offset, buffer_size - offset,
                           "\"O\":%s,\"F\":1}]}", (char *)in_data);
    if (written < 0 || (size_t)written >= buffer_size - offset) {
      LOG_ERR("Buffer overflow when writing JSON inference data.");
      return kProcessFormatResultMemoryError;
    }
    offset += written;
  } else {
    LOG_ERR("Invalid datatype: %d", datatype);
    return kProcessFormatResultInvalidParam;
  }

  return kProcessFormatResultOk;
}
