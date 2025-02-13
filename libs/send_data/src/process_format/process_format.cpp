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
    *image = in_data.u.p;
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
                                      uint64_t timestamp,
                                      JSON_Value *output_tensor_value) {
  if (output_tensor_value == NULL) {
    LOG_ERR("Invalid output_tensor_value.");
    return kProcessFormatResultInvalidParam;
  }

  JSON_Object *output_tensor_object =
      json_value_get_object(output_tensor_value);

  // Get AI model bundle ID
  struct EdgeAppLibSensorAiModelBundleIdProperty stream_property = {};
  EdgeAppLibSensorStream stream = GetSensorStream();
  int32_t ret = -1;
  if ((ret = EdgeAppLib::SensorStreamGetProperty(
           stream, AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY,
           &stream_property, sizeof(stream_property))) != 0) {
    const char *error_msg = "Error GET AI model bundle id.";
    LOG_ERR("%s : SensorStreamGetProperty=%d", error_msg, ret);
    return kProcessFormatResultFailure;
  }
  // Set AI model bundle ID
  json_object_set_string(output_tensor_object, "ModelVersionID",
                         stream_property.ai_model_bundle_id);

  // Get Device ID
  char device_id[WASM_BINDING_DEVICEID_MAX_SIZE] = {0};
  if ((ret = EsfSystemGetDeviceID(device_id)) != kEsfDeviceIdResultOk) {
    const char *error_msg = "Error GET device id.";
    LOG_ERR("%s : EsfSystemGetDeviceID=%d", error_msg, ret);
    snprintf(device_id, sizeof(device_id), "000000000000000");
  }
  // Set Device ID
  json_object_set_string(output_tensor_object, "DeviceID", device_id);

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
  json_object_set_boolean(output_tensor_object, "Image", image_flg);

  // Initialize one Inference for appending to "Inferences"
  JSON_Value *inf_value = json_value_init_object();
  JSON_Object *inf_object = json_value_get_object(inf_value);

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
  json_object_set_string(inf_object, "T", (const char *)inf_timestamp);

  // Set "O" and "F"
  switch (datatype) {
    case EdgeAppLibSendDataBase64: {
      int inf_o_size = b64e_size((unsigned int)in_size) + 1;
      unsigned char *inf_o =
          (unsigned char *)malloc((sizeof(char) * inf_o_size));
      if (inf_o == nullptr) {
        LOG_ERR("Error while allocating memory for \"O\".");
        json_value_free(inf_value);
        return kProcessFormatResultMemoryError;
      }
      inf_o_size = b64_encode((unsigned char *)in_data, in_size, inf_o);
      json_object_set_string(inf_object, "O", (const char *)inf_o);
      json_object_set_number(inf_object, "F", 0);
      free(inf_o);
    } break;
    case EdgeAppLibSendDataJson:
      json_object_set_string(inf_object, "O", (const char *)in_data);
      json_object_set_number(inf_object, "F", 1);
      break;
    default:
      const char *error_msg = "Invalid datatype.";
      LOG_ERR("%s : datatype=%d", error_msg, datatype);
      json_value_free(inf_value);
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
