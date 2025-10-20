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

#include <cmath>

#include "log.h"

static bool IsValidDrawBuffer(struct EdgeAppLibDrawBuffer *buffer) {
  if (buffer == nullptr) {
    LOG_ERR("IsValidDrawBuffer: Buffer is null");
    return false;
  }
  if (buffer->width == 0 || buffer->height == 0) {
    LOG_ERR("IsValidDrawBuffer: Invalid dimensions %ux%u", buffer->width,
            buffer->height);
    return false;
  }
  if (buffer->format < AITRIOS_DRAW_FORMAT_RGB8 ||
      buffer->format > AITRIOS_DRAW_FORMAT_RGB8_PLANAR) {
    LOG_ERR("IsValidDrawBuffer: Invalid format %d", buffer->format);
    return false;
  }
  // To ensure compatibility with the legacy draw functions,
  if (buffer->stride_byte == 0) {
    if (buffer->format == AITRIOS_DRAW_FORMAT_RGB8) {
      buffer->stride_byte = static_cast<uint32_t>(buffer->width) *
                            3;  // RGB format, 3 bytes per pixel
    }
    if (buffer->format == AITRIOS_DRAW_FORMAT_RGB8_PLANAR) {
      buffer->stride_byte = static_cast<uint32_t>(
          buffer->width);  // Planar format, 1 byte per pixel
    }
  }

  uint32_t expected_size = (buffer->format == AITRIOS_DRAW_FORMAT_RGB8_PLANAR)
                               ? buffer->stride_byte * buffer->height * 3
                               : buffer->stride_byte * buffer->height;

  if (buffer->size != expected_size) {
    LOG_ERR("IsValidDrawBuffer: Buffer size mismatch");
    LOG_ERR("Expected size: %u, Actual size: %u", expected_size, buffer->size);
    LOG_ERR("Stride: %u, Height: %u", buffer->stride_byte, buffer->height);
    return false;
  }
  return buffer->address != nullptr &&
         buffer->format != AITRIOS_DRAW_FORMAT_UNDEFINED;
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
    uint32_t stride = buffer->stride_byte;
    uint32_t height = buffer->height;
    *r = (uint8_t *)buffer->address + (stride * height * 0);
    *g = (uint8_t *)buffer->address + (stride * height * 1);
    *b = (uint8_t *)buffer->address + (stride * height * 2);
  }
};

template <enum EdgeAppLibDrawFormat FMT>
static int PixelOffset(uint32_t stride_bytes, int x, int y) {
  return (y * stride_bytes + x * FormatTraits<FMT>::kPixelComponentStride);
}

template <enum EdgeAppLibDrawFormat FMT>
static void DrawRectangle(struct EdgeAppLibDrawBuffer *buffer, uint32_t left,
                          uint32_t top, uint32_t right, uint32_t bottom,
                          struct EdgeAppLibColor color) {
  uint8_t *r, *g, *b;
  FormatTraits<FMT>::PixelComponents(buffer, &r, &g, &b);

  uint32_t stride = buffer->stride_byte;

#define PIXEL_PUT(x, y)                     \
  {                                         \
    int i = PixelOffset<FMT>(stride, x, y); \
    r[i] = color.red;                       \
    g[i] = color.green;                     \
    b[i] = color.blue;                      \
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

  uint32_t src_stride = src->stride_byte;
  uint32_t dst_stride = dst->stride_byte;
  uint32_t crop_width = right - left + 1;
  uint32_t crop_height = bottom - top + 1;

  for (uint32_t y = 0; y < crop_height; ++y) {
    for (uint32_t x = 0; x < crop_width; ++x) {
      int src_index = PixelOffset<FMT>(src_stride, left + x, top + y);
      int dst_index = PixelOffset<FMT>(dst_stride, x, y);

      dst_r[dst_index] = src_r[src_index];
      dst_g[dst_index] = src_g[src_index];
      dst_b[dst_index] = src_b[src_index];
    }
  }
}

// Bilinear resize using float precision (RGB8 / RGB8_PLANAR)
template <enum EdgeAppLibDrawFormat FMT>
static void ResizeRectangle(const EdgeAppLibDrawBuffer *src,
                            EdgeAppLibDrawBuffer *dst) {
  uint8_t *src_r, *src_g, *src_b;
  uint8_t *dst_r, *dst_g, *dst_b;
  FormatTraits<FMT>::PixelComponents(const_cast<EdgeAppLibDrawBuffer *>(src),
                                     &src_r, &src_g, &src_b);
  FormatTraits<FMT>::PixelComponents(dst, &dst_r, &dst_g, &dst_b);

  const uint32_t src_w = src->width, src_h = src->height;
  const uint32_t dst_w = dst->width, dst_h = dst->height;
  const uint32_t src_stride = src->stride_byte, dst_stride = dst->stride_byte;

  // Pixel-center mapping
  const float scale_x = static_cast<float>(src_w) / static_cast<float>(dst_w);
  const float scale_y = static_cast<float>(src_h) / static_cast<float>(dst_h);

  for (uint32_t y = 0; y < dst_h; ++y) {
    float sy = (static_cast<float>(y) + 0.5f) * scale_y - 0.5f;
    int y0 = static_cast<int>(std::floor(sy));
    float wy = sy - static_cast<float>(y0);
    int y1 = y0 + 1;
    if (y0 < 0) {
      y0 = 0;
      wy = 1.0f;
    }
    if (y1 >= static_cast<int>(src_h)) y1 = static_cast<int>(src_h) - 1;
    if (y0 >= static_cast<int>(src_h)) y0 = static_cast<int>(src_h) - 1;

    for (uint32_t x = 0; x < dst_w; ++x) {
      float sx = (static_cast<float>(x) + 0.5f) * scale_x - 0.5f;
      int x0 = static_cast<int>(std::floor(sx));
      float wx = sx - static_cast<float>(x0);
      int x1 = x0 + 1;
      if (x0 < 0) {
        x0 = 0;
        wx = 1.0f;
      }
      if (x1 >= static_cast<int>(src_w)) x1 = static_cast<int>(src_w) - 1;
      if (x0 >= static_cast<int>(src_w)) x0 = static_cast<int>(src_w) - 1;

      const int i00 = PixelOffset<FMT>(src_stride, x0, y0);
      const int i10 = PixelOffset<FMT>(src_stride, x1, y0);
      const int i01 = PixelOffset<FMT>(src_stride, x0, y1);
      const int i11 = PixelOffset<FMT>(src_stride, x1, y1);
      const int o = PixelOffset<FMT>(dst_stride, x, y);

      // R
      {
        float v0 = src_r[i00] * (1.0f - wx) + src_r[i10] * wx;
        float v1 = src_r[i01] * (1.0f - wx) + src_r[i11] * wx;
        float v = v0 * (1.0f - wy) + v1 * wy;
        if (v < 0.0f)
          v = 0.0f;
        else if (v > 255.0f)
          v = 255.0f;
        dst_r[o] = static_cast<uint8_t>(v + 0.5f);
      }
      // G
      {
        float v0 = src_g[i00] * (1.0f - wx) + src_g[i10] * wx;
        float v1 = src_g[i01] * (1.0f - wx) + src_g[i11] * wx;
        float v = v0 * (1.0f - wy) + v1 * wy;
        if (v < 0.0f)
          v = 0.0f;
        else if (v > 255.0f)
          v = 255.0f;
        dst_g[o] = static_cast<uint8_t>(v + 0.5f);
      }
      // B
      {
        float v0 = src_b[i00] * (1.0f - wx) + src_b[i10] * wx;
        float v1 = src_b[i01] * (1.0f - wx) + src_b[i11] * wx;
        float v = v0 * (1.0f - wy) + v1 * wy;
        if (v < 0.0f)
          v = 0.0f;
        else if (v > 255.0f)
          v = 255.0f;
        dst_b[o] = static_cast<uint8_t>(v + 0.5f);
      }
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

int32_t ResizeRectangle(const struct EdgeAppLibDrawBuffer *src,
                        struct EdgeAppLibDrawBuffer *dst) {
  if (!IsValidDrawBuffer(const_cast<EdgeAppLibDrawBuffer *>(src)) ||
      !IsValidDrawBuffer(dst)) {
    LOG_ERR("ResizeRectangle: Invalid buffer");
    return -1;
  }

  if (src->format != dst->format) {
    LOG_ERR("ResizeRectangle: Format mismatch between source and destination");
    return -1;
  }

  if (src->width == dst->width && src->height == dst->height) {
    // No resizing needed, just copy the data
    memcpy(dst->address, src->address, src->size);
    return 0;
  }

  switch (src->format) {
    case AITRIOS_DRAW_FORMAT_RGB8:
      ResizeRectangle<AITRIOS_DRAW_FORMAT_RGB8>(src, dst);
      break;
    case AITRIOS_DRAW_FORMAT_RGB8_PLANAR:
      ResizeRectangle<AITRIOS_DRAW_FORMAT_RGB8_PLANAR>(src, dst);
      break;
    default:
      LOG_ERR("ResizeRectangle: Unknown format %d", src->format);
      return -1;
  }

  return 0;
}

int32_t CropRectangle(struct EdgeAppLibDrawBuffer *src,
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
