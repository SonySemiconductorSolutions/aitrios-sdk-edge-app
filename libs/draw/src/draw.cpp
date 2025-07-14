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

static bool IsValidDrawBuffer(const struct EdgeAppLibDrawBuffer *buffer) {
  return buffer != nullptr && buffer->address != nullptr &&
         buffer->format != AITRIOS_DRAW_FORMAT_UNDEFINED &&
         buffer->size == (buffer->width * buffer->height * 3);
}

template <enum EdgeAppLibDrawFormat>
struct FormatTraits {
  static constexpr int kPixelComponentStride = 0;
  static void PixelComponents(struct EdgeAppLibDrawBuffer *buffer, uint8_t **r,
                              uint8_t **g, uint8_t **b) {}
};

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
  for (uint32_t x = left; x <= right; x++) {
    PIXEL_PUT(x, top);
    PIXEL_PUT(x, bottom);
  }

  // Draw vertical lines
  for (uint32_t y = top; y <= bottom; y++) {
    PIXEL_PUT(left, y);
    PIXEL_PUT(right, y);
  }
#undef PIXEL_PUT
}

template <enum EdgeAppLibDrawFormat FMT>
static void CropRectangle(const struct EdgeAppLibDrawBuffer *src,
                          struct EdgeAppLibDrawBuffer *dst, uint32_t left,
                          uint32_t top, uint32_t right, uint32_t bottom) {
  uint8_t *src_r, *src_g, *src_b;
  uint8_t *dst_r, *dst_g, *dst_b;

  FormatTraits<FMT>::PixelComponents((struct EdgeAppLibDrawBuffer *)src, &src_r,
                                     &src_g, &src_b);
  FormatTraits<FMT>::PixelComponents(dst, &dst_r, &dst_g, &dst_b);

  uint32_t src_width = src->width;
  uint32_t crop_width = right - left + 1;
  uint32_t crop_height = bottom - top + 1;

  for (uint32_t y = 0; y < crop_height; ++y) {
    for (uint32_t x = 0; x < crop_width; ++x) {
      int src_index = PixelOffset<FMT>(src_width, left + x, top + y);
      int dst_index = PixelOffset<FMT>(dst->width, x, y);

      dst_r[dst_index] = src_r[src_index];
      dst_g[dst_index] = src_g[src_index];
      dst_b[dst_index] = src_b[src_index];
    }
  }
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

int32_t CropRectangle(const struct EdgeAppLibDrawBuffer *src,
                      struct EdgeAppLibDrawBuffer *dst, uint32_t left,
                      uint32_t top, uint32_t right, uint32_t bottom) {
  if (!IsValidDrawBuffer(src) || !IsValidDrawBuffer(dst)) {
    LOG_ERR("CropRectangle: Invalid buffer");
    return -1;
  }

  uint32_t width = src->width;
  uint32_t height = src->height;

  if (left >= width) left = width - 1;
  if (right >= width) right = width - 1;
  if (top >= height) top = height - 1;
  if (bottom >= height) bottom = height - 1;

  switch (src->format) {
    case AITRIOS_DRAW_FORMAT_RGB8:
      CropRectangle<AITRIOS_DRAW_FORMAT_RGB8>(src, dst, left, top, right,
                                              bottom);
      break;
    case AITRIOS_DRAW_FORMAT_RGB8_PLANAR:
      CropRectangle<AITRIOS_DRAW_FORMAT_RGB8_PLANAR>(src, dst, left, top, right,
                                                     bottom);
      break;
    default:
      LOG_ERR("CropRectangle: Unknown format %d", src->format);
      return -1;
  }

  return 0;
}
