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

#include <gtest/gtest.h>

#include "map.hpp"

class MapTest : public ::testing::Test {
 public:
  void TearDown() override { map_clear(); }
};

TEST_F(MapTest, SetAndPop) {
  EXPECT_TRUE(map_is_empty());
  int a, b;
  int x, y;
  EXPECT_TRUE(map_pop(&x) == nullptr);
  EXPECT_TRUE(map_pop(nullptr) == nullptr);
  map_set(&a, &x);
  map_set(&b, &y);
  EXPECT_TRUE(map_pop(&a) == &x);
  EXPECT_FALSE(map_is_empty());
  EXPECT_TRUE(map_pop(&b) == &y);
  EXPECT_TRUE(map_is_empty());
}

TEST_F(MapTest, FillQueue) {
  map_clear();
  EXPECT_TRUE(map_is_empty());
  for (int i = 0; i < MAX_FUTURES_QUEUE; ++i) {
    EXPECT_TRUE(map_set((void *)i + 1, (void *)2) == 0);
  }
  EXPECT_TRUE(map_set((void *)1, (void *)2) == -1);
  map_clear();
  EXPECT_TRUE(map_is_empty());
}

TEST_F(MapTest, CancelMap) {
  map_clear();
  for (int i = 0; i < MAX_FUTURES_QUEUE; ++i) {
    EXPECT_TRUE(map_set((void *)i + 1, (void *)2) == 0);
  }
  EXPECT_FALSE(map_is_empty());
  while (!map_is_empty()) {
    void *key = map_remained();
    EXPECT_TRUE(map_pop(key) == (void *)2);
  }
  EXPECT_TRUE(map_is_empty());
}

TEST_F(MapTest, Approve_Overwrap) {
  map_clear();
  map_set((void *)1, (void *)2);
  EXPECT_TRUE(map_set((void *)1, (void *)2) == 0);
  EXPECT_TRUE(map_set((void *)2, (void *)2) == 0);
  EXPECT_TRUE(map_set((void *)2, (void *)2) == 0);
  map_clear();
  EXPECT_TRUE(map_is_empty());
}
