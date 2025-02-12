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

/**
 * @file draw.h
 * @brief Header file for Draw API
 * @details This file defines basic functions for drawing on an image buffer.
 */

#ifndef _AITRIOS_DRAW_H_
#define _AITRIOS_DRAW_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @struct EdgeAppLibColor
 * @brief Represents an RGB color.
 */
struct EdgeAppLibColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

#define AITRIOS_COLOR_RED ((struct EdgeAppLibColor){0xFF, 0x00, 0x00})
#define AITRIOS_COLOR_GREEN ((struct EdgeAppLibColor){0x00, 0xFF, 0x00})
#define AITRIOS_COLOR_BLUE ((struct EdgeAppLibColor){0x00, 0x00, 0xFF})

/**
 * @enum EdgeAppLibDrawFormat
 * @brief
 */
enum EdgeAppLibDrawFormat {
  AITRIOS_DRAW_FORMAT_UNDEFINED = 0,
  AITRIOS_DRAW_FORMAT_RGB8, /**< RGB, 8-bits per component, interleaved */
  AITRIOS_DRAW_FORMAT_RGB8_PLANAR, /**< RGB, 8-bits per component, planar */
};

/**
 * @struct EdgeAppLibDrawBuffer
 * @brief Represents an image buffer in which to perform drawing operations.
 */
struct EdgeAppLibDrawBuffer {
  void *address;                    /**< image pixel buffer */
  size_t size;                      /**< image pixel buffer size in bytes */
  enum EdgeAppLibDrawFormat format; /**< image pixel format */
  uint32_t width;                   /**< image width in pixels */
  uint32_t height;                  /**< image height in pixels */
};

/**
 * @brief Draw a rectangle outline on an image buffer.
 *
 * @param[in] buffer Image to draw in.
 * @param[in] left Rectangle left side coordinate in pixels.
 * @param[in] top Rectangle top side coordinate in pixels.
 * @param[in] right Rectangle right side coordinate in pixels.
 * @param[in] bottom Rectangle bottom side coordinate in pixels.
 * @param[in] color Color of the rectangle.
 *
 * @return Zero for success or negative value for failure
 *
 * @details If the rectangle is not fully inside the image bounds, the rectangle
 * is clamped to the image bounds.
 */
int32_t DrawRectangle(struct EdgeAppLibDrawBuffer *buffer, uint32_t left,
                      uint32_t top, uint32_t right, uint32_t bottom,
                      struct EdgeAppLibColor color);

#ifdef __cplusplus
}
#endif

#endif /* _AITRIOS_LOG_H_ */
