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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "memory_manager.hpp"

TEST(MemoryManager, UseMaxAllocations) {
  for (int i = 0; i < 10; ++i) {
    __setMaxAllocations(i);
    for (int j = 0; j < 10; ++j) {
      void *buf = xmalloc(10);
      printf("%d %d\n", i, j);
      if (i == 0 || i <= j)
        ASSERT_TRUE(buf == NULL);
      else
        ASSERT_TRUE(buf != NULL);
      free(buf);
    }
  }
}

TEST(MemoryManager, NoLimit) {
  __setMaxAllocations(-1);
  for (int j = 0; j < 10; ++j) {
    void *buf = xmalloc(10);
    ASSERT_TRUE(buf != NULL);
    free(buf);
  }
}
