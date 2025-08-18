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
