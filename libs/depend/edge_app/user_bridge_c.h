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

#ifndef USER_BRIDGE_C_H_
#define USER_BRIDGE_C_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Pixel formats
 */
// For backward compatibility.
#define SENSCORD_UB_MONO8 "mono8"                 /* 8-bit Greyscale */
#define SENSCORD_UB_MONO16 "mono16"               /* 16-bit Greyscale */
#define SENSCORD_UB_DEPTH16 "depth_z16"           /* 16-bit depth */
#define SENSCORD_UB_ITOFIMAGERAW "itof_image_raw" /* Image raw */

// senscord definition.
#define SENSCORD_PIXEL_FORMAT_GREY "image_grey"
#define SENSCORD_PIXEL_FORMAT_Y10 "image_y10"
#define SENSCORD_PIXEL_FORMAT_Y12 "image_y12"
#define SENSCORD_PIXEL_FORMAT_Y16 "image_y16"
#define SENSCORD_PIXEL_IMAGERAW "itof_image_raw"  // TBD
#define SENSCORD_PIXEL_FORMAT_RGB24 "image_rgb24"
#define SENSCORD_PIXEL_FORMAT_RGB16_PLANAR "image_rgb16_planar"
#define SENSCORD_PIXEL_FORMAT_NV16 "image_nv16"
#define SENSCORD_PIXEL_FORMAT_Z16 "depth_z16"
#define SENSCORD_PIXEL_FORMAT_Z32F "depth_z32f"
#define SENSCORD_PIXEL_FORMAT_D16 "depth_d16"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Create stream.
 * @param[in] (stream_name) Name of the stream.
 * @param[in] (width) width of the columns.
 * @param[in] (height) height of the rows.
 * @param[in] (stride_bytes) length of a row in bytes.
 * @param[in] (pixel_format) pixel format.
 * @return handle number.
 */
uint64_t senscord_ub_create_stream(const char *name, uint32_t width,
                                   uint32_t height, uint32_t stride_bytes,
                                   const char *pixel_format);

/**
 * @brief Create stream.
 * @param[in] (stream_name) Name of the stream.
 * @param[in] (width) width of the columns.
 * @param[in] (height) height of the rows.
 * @param[in] (stride_bytes) length of a row in bytes.
 * @param[in] (pixel_format) pixel format.
 * @param[in] (scale) scale.
 * @param[in] (min_range) the minimum value of Depth Raw Data.
 * @param[in] (max_range) the maximum value of Depth Raw Data.
 * @return handle number.
 */
uint64_t senscord_ub_create_stream_depth(const char *name, uint32_t width,
                                         uint32_t height, uint32_t stride_bytes,
                                         const char *pixel_format, float scale,
                                         float min_range, float max_range);

/**
 * @brief Send serialized stream data.
 * @param[in] (handle) handle number.
 * @param[in] (data) actual stream data, size is (stride_bytes * height)
 * @return result code.
 */
int32_t senscord_ub_send_data(uint64_t handle, void *data);

/**
 * @brief Destroy stream.
 * @param[in] (handle) handle number.
 * @return result code.
 */
int32_t senscord_ub_destroy_stream(uint64_t handle);

#ifdef __cplusplus
}  // extern "C"
#endif /* __cplusplus */

#endif /* USER_BRIDGE_C_H_ */
