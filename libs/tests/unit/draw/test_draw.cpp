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
