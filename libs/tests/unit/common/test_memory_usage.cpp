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

#include <thread>
#include <vector>

#include "log.h"
#include "memory_usage.h"

extern "C" void memory_usage(size_t used, size_t fordblks, size_t keepcost);

TEST(MemoryUsage, InitialState) {
  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, 0);
  EXPECT_EQ(metrics.free_bytes, 0);
  EXPECT_FLOAT_EQ(metrics.fragmentation_rate, -1.0f);
}

TEST(MemoryUsage, UpdateAndRead) {
  memory_usage(1024, 2048, 512);

  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, 1024);
  EXPECT_EQ(metrics.free_bytes, 2048);
  EXPECT_FLOAT_EQ(metrics.fragmentation_rate, 0.75f);
}

TEST(MemoryUsage, ZeroValues) {
  memory_usage(0, 0, 0);

  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, 0);
  EXPECT_EQ(metrics.free_bytes, 0);
  EXPECT_FLOAT_EQ(metrics.fragmentation_rate, -1.0f);
}

TEST(MemoryUsage, OnlyUsedIsZero) {
  memory_usage(0, 1024, 512);

  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, 0);
  EXPECT_EQ(metrics.free_bytes, 1024);
  EXPECT_FLOAT_EQ(metrics.fragmentation_rate, -1.0f);
}

TEST(MemoryUsage, OnlyFreeIsZero) {
  memory_usage(1024, 0, 512);

  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, 1024);
  EXPECT_EQ(metrics.free_bytes, 0);
  EXPECT_FLOAT_EQ(metrics.fragmentation_rate, -1.0f);
}

TEST(MemoryUsage, MaxFragmentation) {
  memory_usage(1024, 2048, 0);

  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, 1024);
  EXPECT_EQ(metrics.free_bytes, 2048);
  EXPECT_FLOAT_EQ(metrics.fragmentation_rate, 1.0f);
}

TEST(MemoryUsage, NoFragmentation) {
  memory_usage(1024, 2048, 2048);

  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, 1024);
  EXPECT_EQ(metrics.free_bytes, 2048);
  EXPECT_FLOAT_EQ(metrics.fragmentation_rate, 0.0f);
}

TEST(MemoryUsage, NullPointerHandling) {
  memory_usage(100, 200, 50);
  get_memory_metrics(nullptr);
}

TEST(MemoryUsage, MultipleUpdates) {
  memory_usage(100, 200, 50);

  MemoryMetrics metrics1;
  get_memory_metrics(&metrics1);
  EXPECT_EQ(metrics1.used_bytes, 100);

  memory_usage(500, 1000, 250);

  MemoryMetrics metrics2;
  get_memory_metrics(&metrics2);
  EXPECT_EQ(metrics2.used_bytes, 500);
  EXPECT_EQ(metrics2.free_bytes, 1000);
  EXPECT_FLOAT_EQ(metrics2.fragmentation_rate, 0.75f);
}

TEST(MemoryUsage, ConcurrentReads) {
  memory_usage(1000, 2000, 500);

  const int num_threads = 10;
  const int reads_per_thread = 100;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([reads_per_thread]() {
      for (int j = 0; j < reads_per_thread; ++j) {
        MemoryMetrics metrics;
        get_memory_metrics(&metrics);

        EXPECT_EQ(metrics.used_bytes, 1000);
        EXPECT_EQ(metrics.free_bytes, 2000);
        EXPECT_FLOAT_EQ(metrics.fragmentation_rate, 0.75f);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }
}

TEST(MemoryUsage, LargeValues) {
  size_t large_used = SIZE_MAX / 2;
  size_t large_free = SIZE_MAX / 3;
  size_t large_keep = SIZE_MAX / 4;

  memory_usage(large_used, large_free, large_keep);

  MemoryMetrics metrics;
  get_memory_metrics(&metrics);

  EXPECT_EQ(metrics.used_bytes, large_used);
  EXPECT_EQ(metrics.free_bytes, large_free);

  float expected_frag = 1.0f - ((float)large_keep / (float)large_free);
  EXPECT_NEAR(metrics.fragmentation_rate, expected_frag, 0.01f);
}
