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

#include "draw.h"

#include "log.h"

static bool IsValidDrawBuffer(struct EdgeAppLibDrawBuffer *buffer) {
  return buffer != nullptr && buffer->address != nullptr &&
         buffer->format != AITRIOS_DRAW_FORMAT_UNDEFINED &&
         buffer->size == (buffer->width * buffer->height * 3);
}

template <enum EdgeAppLibDrawFormat>
struct FormatTraits {};

template <>
struct FormatTraits<AITRIOS_DRAW_FORMAT_RGB8> {
  static constexpr int kPixelComponentStride = 3;

  static void PixelComponents(struct EdgeAppLibDrawBuffer *buffer, uint8_t **r,
                              uint8_t **g, uint8_t **b) {
    *r = (uint8_t *)buffer->address + 0;
    *g = (uint8_t *)buffer->address + 1;
    *b = (uint8_t *)buffer->address + 2;
  }
};

template <>
struct FormatTraits<AITRIOS_DRAW_FORMAT_RGB8_PLANAR> {
  static constexpr int kPixelComponentStride = 1;

  static void PixelComponents(struct EdgeAppLibDrawBuffer *buffer, uint8_t **r,
                              uint8_t **g, uint8_t **b) {
    uint32_t width = buffer->width;
    uint32_t height = buffer->height;
    *r = (uint8_t *)buffer->address + (width * height * 0);
    *g = (uint8_t *)buffer->address + (width * height * 1);
    *b = (uint8_t *)buffer->address + (width * height * 2);
  }
};

template <enum EdgeAppLibDrawFormat FMT>
static int PixelOffset(uint32_t width, int x, int y) {
  return (y * width + x) * FormatTraits<FMT>::kPixelComponentStride;
}

template <enum EdgeAppLibDrawFormat FMT>
static void DrawRectangle(struct EdgeAppLibDrawBuffer *buffer, uint32_t left,
                          uint32_t top, uint32_t right, uint32_t bottom,
                          struct EdgeAppLibColor color) {
  uint8_t *r, *g, *b;
  FormatTraits<FMT>::PixelComponents(buffer, &r, &g, &b);

  uint32_t width = buffer->width;

#define PIXEL_PUT(x, y)                    \
  {                                        \
    int i = PixelOffset<FMT>(width, x, y); \
    r[i] = color.red;                      \
    g[i] = color.green;                    \
    b[i] = color.blue;                     \
  }

  // Draw horizontal lines
  for (int x = left; x <= right; x++) {
    PIXEL_PUT(x, top);
    PIXEL_PUT(x, bottom);
  }

  // Draw vertical lines
  for (int y = top; y <= bottom; y++) {
    PIXEL_PUT(left, y);
    PIXEL_PUT(right, y);
  }
#undef PIXEL_PUT
}

int32_t DrawRectangle(struct EdgeAppLibDrawBuffer *buffer, uint32_t left,
                      uint32_t top, uint32_t right, uint32_t bottom,
                      struct EdgeAppLibColor color) {
  if (!IsValidDrawBuffer(buffer)) {
    LOG_ERR("DrawRectangle: Invalid buffer");
    return -1;
  }

  uint32_t width = buffer->width;
  uint32_t height = buffer->height;

  // Clamp rectangle to image bounds
  if (left >= width) left = width - 1;
  if (right >= width) right = width - 1;
  if (top >= height) top = height - 1;
  if (bottom >= height) bottom = height - 1;

  switch (buffer->format) {
    case AITRIOS_DRAW_FORMAT_RGB8:
      DrawRectangle<AITRIOS_DRAW_FORMAT_RGB8>(buffer, left, top, right, bottom,
                                              color);
      break;
    case AITRIOS_DRAW_FORMAT_RGB8_PLANAR:
      DrawRectangle<AITRIOS_DRAW_FORMAT_RGB8_PLANAR>(buffer, left, top, right,
                                                     bottom, color);
      break;
    default:
      LOG_ERR("DrawRectangle: Unknown format %d", buffer->format);
      return -1;
  }

  return 0;
}
