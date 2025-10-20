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

#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define TEST_IMG_WIDTH (100)
#define TEST_IMG_HEIGHT (100)
#define TEST_IMG_BUFFER_SIZE (TEST_IMG_WIDTH * TEST_IMG_HEIGHT * 3)
#define TEST_COLOR ((struct EdgeAppLibColor){0xFF, 0xFF, 0xFF})

class EdgeAppLibDrawApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    draw_buffer.address = malloc(TEST_IMG_BUFFER_SIZE);
    draw_buffer.size = TEST_IMG_BUFFER_SIZE;
    draw_buffer.format = AITRIOS_DRAW_FORMAT_RGB8_PLANAR;
    draw_buffer.width = TEST_IMG_WIDTH;
    draw_buffer.height = TEST_IMG_HEIGHT;
    draw_buffer.stride_byte = TEST_IMG_WIDTH * 1;
    ResetImage();
  }

  void TearDown() override {
    if (WasImageModified()) {
      // Save image in case we want to manually review the test output
      SaveImage();
    }

    free(draw_buffer.address);
  }

  bool WasImageModified() {
    // just check that 0xFF (TEST_COLOR) is somewhere in the image
    return memchr(draw_buffer.address, 0xFF, TEST_IMG_BUFFER_SIZE) != nullptr;
  }

  void ResetImage() {
    // init image to all 0s (black)
    memset(draw_buffer.address, 0, TEST_IMG_BUFFER_SIZE);
  }

  void SaveImage() {
    const char *test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();

    char filename[128];
    snprintf(filename, 128, "%s.bin", test_name);
    FILE *f = fopen(filename, "wb");
    if (f) {
      fwrite(draw_buffer.address, 1, draw_buffer.size, f);
      fclose(f);
    }
  }

  struct EdgeAppLibDrawBuffer draw_buffer = {0};
};

TEST_F(EdgeAppLibDrawApiTest, DrawRectangle_Normal) {
  int32_t ret = DrawRectangle(&draw_buffer, 10, 10, 90, 90, TEST_COLOR);
  ASSERT_EQ(ret, 0);

  ASSERT_TRUE(WasImageModified());
}

TEST_F(EdgeAppLibDrawApiTest, DrawRectangle_Normal_Interleaved) {
  draw_buffer.format = AITRIOS_DRAW_FORMAT_RGB8;
  draw_buffer.stride_byte =
      TEST_IMG_WIDTH * 3;  // RGB format, 3 bytes per pixel
  int32_t ret = DrawRectangle(&draw_buffer, 10, 10, 90, 90, TEST_COLOR);
  ASSERT_EQ(ret, 0);

  ASSERT_TRUE(WasImageModified());
}

TEST_F(EdgeAppLibDrawApiTest, DrawRectangle_OutsideBounds) {
  int32_t ret = DrawRectangle(&draw_buffer, 50, 50, 1000, 1000, TEST_COLOR);
  ASSERT_EQ(ret, 0);

  ASSERT_TRUE(WasImageModified());
}

TEST_F(EdgeAppLibDrawApiTest, DrawRectangle_Failure) {
  int32_t ret = DrawRectangle(nullptr, 10, 10, 90, 90, TEST_COLOR);
  ASSERT_EQ(ret, -1);

  struct EdgeAppLibDrawBuffer uninitialized_draw_buffer = {0};
  ret = DrawRectangle(&uninitialized_draw_buffer, 10, 10, 90, 90, TEST_COLOR);
  ASSERT_EQ(ret, -1);
}

TEST_F(EdgeAppLibDrawApiTest, CropCenterRegion_RGB8) {
  EdgeAppLibDrawBuffer src{};
  src.width = 4;
  src.height = 4;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.stride_byte = 4 * 3;  // 4 pixels, 3 bytes each (RGB)
  src.size = 4 * 4 * 3;
  src.address = new uint8_t[src.size];

  // Fill source with known RGB values
  auto *src_ptr = reinterpret_cast<uint8_t *>(src.address);
  for (uint32_t y = 0; y < src.height; ++y) {
    for (uint32_t x = 0; x < src.width; ++x) {
      size_t idx = (y * src.width + x) * 3;
      src_ptr[idx + 0] = 100;  // R
      src_ptr[idx + 1] = 150;  // G
      src_ptr[idx + 2] = 200;  // B
    }
  }

  EdgeAppLibDrawBuffer dst{};
  dst.width = 2;
  dst.height = 2;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.stride_byte = 2 * 3;  // 2 pixels, 3 bytes each (RGB)
  dst.size = 2 * 2 * 3;
  dst.address = new uint8_t[dst.size];

  // Crop center region: (1,1)-(2,2)
  ASSERT_EQ(CropRectangle(&src, &dst, 1, 1, 2, 2), 0);

  // Validate all cropped pixels have the expected RGB values
  auto *dst_ptr = reinterpret_cast<uint8_t *>(dst.address);
  for (uint32_t y = 0; y < dst.height; ++y) {
    for (uint32_t x = 0; x < dst.width; ++x) {
      size_t idx = (y * dst.width + x) * 3;
      EXPECT_EQ(dst_ptr[idx + 0], 100);
      EXPECT_EQ(dst_ptr[idx + 1], 150);
      EXPECT_EQ(dst_ptr[idx + 2], 200);
    }
  }

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, CropOutOfBoundsClamped_RGB8) {
  EdgeAppLibDrawBuffer src{};
  src.width = 3;
  src.height = 3;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.stride_byte = 3 * 3 + 3;  // 3 pixels, 3 bytes each (RGB) + padding 3
  src.size = src.stride_byte * src.height;
  src.address = new uint8_t[src.size];

  auto *src_ptr = reinterpret_cast<uint8_t *>(src.address);
  for (size_t i = 0; i < src.size; ++i) {
    src_ptr[i] = 123;  // All channels = 123
  }

  EdgeAppLibDrawBuffer dst{};
  dst.width = 1;
  dst.height = 1;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.stride_byte = 1 * 3;  // 1 pixel, 3 bytes (RGB)
  dst.size = 1 * 1 * 3;
  dst.address = new uint8_t[dst.size];

  // Crop area clearly out of bounds → should clamp to (2,2)-(2,2)
  ASSERT_EQ(CropRectangle(&src, &dst, 5, 5, 10, 10), 0);

  auto *dst_ptr = reinterpret_cast<uint8_t *>(dst.address);
  EXPECT_EQ(dst_ptr[0], 123);  // R
  EXPECT_EQ(dst_ptr[1], 123);  // G
  EXPECT_EQ(dst_ptr[2], 123);  // B

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, NullSourceBufferPointer) {
  // Destination buffer with valid allocation
  EdgeAppLibDrawBuffer dst{};
  dst.width = 2;
  dst.height = 2;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.size = 2 * 2 * 3;
  dst.address = new uint8_t[dst.size];

  // Passing nullptr as source buffer
  ASSERT_EQ(CropRectangle(nullptr, &dst, 0, 0, 1, 1), -1);

  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, NullDestinationBufferPointer) {
  EdgeAppLibDrawBuffer src{};
  src.width = 2;
  src.height = 2;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.size = 2 * 2 * 3;
  src.address = new uint8_t[src.size];

  // Passing nullptr as destination buffer
  ASSERT_EQ(CropRectangle(&src, nullptr, 0, 0, 1, 1), -1);

  delete[] reinterpret_cast<uint8_t *>(src.address);
}

TEST_F(EdgeAppLibDrawApiTest, NullBufferAddress) {
  // Valid struct but null address
  EdgeAppLibDrawBuffer src{};
  src.width = 2;
  src.height = 2;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.size = 0;
  src.address = nullptr;

  EdgeAppLibDrawBuffer dst{};
  dst.width = 2;
  dst.height = 2;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.size = 2 * 2 * 3;
  dst.address = new uint8_t[dst.size];

  ASSERT_EQ(CropRectangle(&src, &dst, 0, 0, 1, 1), -1);

  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, UnknownFormat) {
  EdgeAppLibDrawBuffer src{};
  src.width = 2;
  src.height = 2;
  src.format = static_cast<EdgeAppLibDrawFormat>(999);  // Invalid format
  src.size = 2 * 2 * 3;
  src.address = new uint8_t[src.size];

  EdgeAppLibDrawBuffer dst{};
  dst.width = 2;
  dst.height = 2;
  dst.format = static_cast<EdgeAppLibDrawFormat>(999);
  dst.size = 2 * 2 * 3;
  dst.address = new uint8_t[dst.size];

  ASSERT_EQ(CropRectangle(&src, &dst, 0, 0, 1, 1), -1);

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, StrideAndSizeMismatch) {
  EdgeAppLibDrawBuffer src{};
  src.width = 2;
  src.height = 2;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;  // Valid format, testing incorrect
                                          // stride and size settings
  src.stride_byte = 2 * 3 + 1;            // Add padding
  src.size = src.stride_byte * src.height + 5;  // Wrong size
  src.address = new uint8_t[src.size];

  EdgeAppLibDrawBuffer dst{};
  dst.width = 2;
  dst.height = 2;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.stride_byte = 2 * 3;  // Correct stride
  dst.size = 2 * 2 * 3;
  dst.address = new uint8_t[dst.size];

  ASSERT_EQ(CropRectangle(&src, &dst, 0, 0, 1, 1), -1);

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, NoStrideByteSetting) {
  EdgeAppLibDrawBuffer src{};
  src.width = 4;
  src.height = 4;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.size = 4 * 4 * 3;
  src.address = new uint8_t[src.size];

  // Fill source with known RGB values
  auto *src_ptr = reinterpret_cast<uint8_t *>(src.address);
  for (uint32_t y = 0; y < src.height; ++y) {
    for (uint32_t x = 0; x < src.width; ++x) {
      size_t idx = (y * src.width + x) * 3;
      src_ptr[idx + 0] = 100;  // R
      src_ptr[idx + 1] = 150;  // G
      src_ptr[idx + 2] = 200;  // B
    }
  }

  EdgeAppLibDrawBuffer dst{};
  dst.width = 2;
  dst.height = 2;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.size = 2 * 2 * 3;
  dst.address = new uint8_t[dst.size];

  // Crop center region: (1,1)-(2,2)
  ASSERT_EQ(CropRectangle(&src, &dst, 1, 1, 2, 2), 0);

  // Validate all cropped pixels have the expected RGB values
  auto *dst_ptr = reinterpret_cast<uint8_t *>(dst.address);
  for (uint32_t y = 0; y < dst.height; ++y) {
    for (uint32_t x = 0; x < dst.width; ++x) {
      size_t idx = (y * dst.width + x) * 3;
      EXPECT_EQ(dst_ptr[idx + 0], 100);
      EXPECT_EQ(dst_ptr[idx + 1], 150);
      EXPECT_EQ(dst_ptr[idx + 2], 200);
    }
  }

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

// ===================== ResizeRectangle tests (Bilinear) =====================

TEST_F(EdgeAppLibDrawApiTest,
       ResizeRectangleBilinear_RGB8_Downscale_2x2_to_1x1) {
  EdgeAppLibDrawBuffer src{};
  src.width = 2;
  src.height = 2;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.stride_byte = 2 * 3;
  src.size = src.stride_byte * src.height;
  src.address = new uint8_t[src.size];

  // Fill a 2x2 image with distinct RGB values per pixel
  // Layout (x,y): (0,0) (1,0)
  //               (0,1) (1,1)
  auto *s = reinterpret_cast<uint8_t *>(src.address);
  // (0,0)
  s[0] = 10;
  s[1] = 20;
  s[2] = 30;
  // (1,0)
  s[3] = 40;
  s[4] = 50;
  s[5] = 60;
  // (0,1)
  s[6] = 70;
  s[7] = 80;
  s[8] = 90;
  // (1,1)
  s[9] = 100;
  s[10] = 110;
  s[11] = 120;

  EdgeAppLibDrawBuffer dst{};
  dst.width = 1;
  dst.height = 1;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.stride_byte = 1 * 3;
  dst.size = dst.stride_byte * dst.height;
  dst.address = new uint8_t[dst.size];

  ASSERT_EQ(ResizeRectangle(&src, &dst), 0);

  auto *d = reinterpret_cast<uint8_t *>(dst.address);
  // Bilinear at center with pixel-center mapping equals the average of 4 pixels
  int exp_r = (10 + 40 + 70 + 100) / 4;
  int exp_g = (20 + 50 + 80 + 110) / 4;
  int exp_b = (30 + 60 + 90 + 120) / 4;

  EXPECT_NEAR(d[0], exp_r, 1);
  EXPECT_NEAR(d[1], exp_g, 1);
  EXPECT_NEAR(d[2], exp_b, 1);

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, ResizeRectangleBilinear_RGB8_Identity_3x3) {
  EdgeAppLibDrawBuffer src{};
  src.width = 3;
  src.height = 3;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.stride_byte = 3 * 3;
  src.size = src.stride_byte * src.height;
  src.address = new uint8_t[src.size];

  // Fill with a simple ramp so we can compare byte-wise
  auto *s = reinterpret_cast<uint8_t *>(src.address);
  for (uint32_t y = 0; y < src.height; ++y) {
    for (uint32_t x = 0; x < src.width; ++x) {
      size_t idx = y * src.stride_byte + x * 3;
      s[idx + 0] = static_cast<uint8_t>(x + y * 3);    // R
      s[idx + 1] = static_cast<uint8_t>(100 + x + y);  // G
      s[idx + 2] = static_cast<uint8_t>(200 - x - y);  // B
    }
  }

  EdgeAppLibDrawBuffer dst{};
  dst.width = 3;
  dst.height = 3;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.stride_byte = 3 * 3;
  dst.size = dst.stride_byte * dst.height;
  dst.address = new uint8_t[dst.size];

  ASSERT_EQ(ResizeRectangle(&src, &dst), 0);

  auto *d = reinterpret_cast<uint8_t *>(dst.address);
  for (uint32_t i = 0; i < dst.size; ++i) {
    EXPECT_EQ(d[i], s[i]);  // Identity ResizeRectangle should match exactly
  }

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, ResizeRectangleBilinear_RGB8_WithPaddingStride) {
  EdgeAppLibDrawBuffer src{};
  src.width = 4;
  src.height = 3;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.stride_byte = 4 * 3 + 4;  // add padding per row
  src.size = src.stride_byte * src.height;
  src.address = new uint8_t[src.size];

  // Initialize all bytes to a known pad, then fill valid pixels
  auto *s = reinterpret_cast<uint8_t *>(src.address);
  memset(s, 7, src.size);
  for (uint32_t y = 0; y < src.height; ++y) {
    for (uint32_t x = 0; x < src.width; ++x) {
      size_t idx = y * src.stride_byte + x * 3;
      s[idx + 0] = static_cast<uint8_t>(10 * x);
      s[idx + 1] = static_cast<uint8_t>(20 * y);
      s[idx + 2] = 200;
    }
  }

  EdgeAppLibDrawBuffer dst{};
  dst.width = 2;
  dst.height = 2;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.stride_byte = 2 * 3;
  dst.size = dst.stride_byte * dst.height;
  dst.address = new uint8_t[dst.size];

  ASSERT_EQ(ResizeRectangle(&src, &dst), 0);

  auto *d = reinterpret_cast<uint8_t *>(dst.address);
  // Spot-check: values should be within valid 0..255 range and not equal to
  // padding
  for (uint32_t i = 0; i < dst.size; ++i) {
    EXPECT_NE(d[i], 7);
  }

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest,
       ResizeRectangleBilinear_RGB8Planar_Upscale_2x2_to_4x4) {
  EdgeAppLibDrawBuffer src{};
  src.width = 2;
  src.height = 2;
  src.format = AITRIOS_DRAW_FORMAT_RGB8_PLANAR;
  src.stride_byte = 2;  // planar: bytes per row per plane
  src.size = src.stride_byte * src.height * 3;
  src.address = new uint8_t[src.size];

  // Planar layout: R-plane (0..wh-1), G-plane (next), B-plane (next)
  auto *base = reinterpret_cast<uint8_t *>(src.address);
  uint8_t *r = base + src.stride_byte * src.height * 0;
  uint8_t *g = base + src.stride_byte * src.height * 1;
  uint8_t *b = base + src.stride_byte * src.height * 2;

  // Fill a simple pattern
  // R plane
  r[0] = 10;
  r[1] = 20;
  r[2] = 30;
  r[3] = 40;
  // G plane
  g[0] = 50;
  g[1] = 60;
  g[2] = 70;
  g[3] = 80;
  // B plane
  b[0] = 90;
  b[1] = 100;
  b[2] = 110;
  b[3] = 120;

  EdgeAppLibDrawBuffer dst{};
  dst.width = 4;
  dst.height = 4;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8_PLANAR;
  dst.stride_byte = 4;  // planar: bytes per row per plane
  dst.size = dst.stride_byte * dst.height * 3;
  dst.address = new uint8_t[dst.size];

  ASSERT_EQ(ResizeRectangle(&src, &dst), 0);

  // Sanity: center pixels should be between min/max of corresponding source
  // plane
  auto *base_d = reinterpret_cast<uint8_t *>(dst.address);
  uint8_t *rd = base_d + dst.stride_byte * dst.height * 0;
  uint8_t *gd = base_d + dst.stride_byte * dst.height * 1;
  uint8_t *bd = base_d + dst.stride_byte * dst.height * 2;

  uint8_t r_min = 10, r_max = 40;
  uint8_t g_min = 50, g_max = 80;
  uint8_t b_min = 90, b_max = 120;

  // Check a few positions
  EXPECT_GE(rd[2 + 2 * dst.stride_byte], r_min);
  EXPECT_LE(rd[2 + 2 * dst.stride_byte], r_max);
  EXPECT_GE(gd[2 + 2 * dst.stride_byte], g_min);
  EXPECT_LE(gd[2 + 2 * dst.stride_byte], g_max);
  EXPECT_GE(bd[2 + 2 * dst.stride_byte], b_min);
  EXPECT_LE(bd[2 + 2 * dst.stride_byte], b_max);

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}

TEST_F(EdgeAppLibDrawApiTest, ResizeRectangleBilinear_FailureCases) {
  // Format mismatch
  EdgeAppLibDrawBuffer src{};
  src.width = 2;
  src.height = 2;
  src.format = AITRIOS_DRAW_FORMAT_RGB8;
  src.stride_byte = 2 * 3;
  src.size = src.stride_byte * src.height;
  src.address = new uint8_t[src.size];

  EdgeAppLibDrawBuffer dst{};
  dst.width = 4;
  dst.height = 4;
  dst.format = AITRIOS_DRAW_FORMAT_RGB8_PLANAR;  // mismatch on purpose
  dst.stride_byte = 4;
  dst.size = dst.stride_byte * dst.height * 3;
  dst.address = new uint8_t[dst.size];

  EXPECT_EQ(ResizeRectangle(&src, &dst), -1);

  // Null source
  EXPECT_EQ(ResizeRectangle(nullptr, &dst), -1);
  // Null destination
  EXPECT_EQ(ResizeRectangle(&src, nullptr), -1);

  // Zero dst size
  dst.format = AITRIOS_DRAW_FORMAT_RGB8;
  dst.width = 0;
  dst.height = 4;
  dst.stride_byte = 0;
  dst.size = 0;
  EXPECT_EQ(ResizeRectangle(&src, &dst), -1);

  delete[] reinterpret_cast<uint8_t *>(src.address);
  delete[] reinterpret_cast<uint8_t *>(dst.address);
}
// ===================== End of ResizeRectangle tests =====================
