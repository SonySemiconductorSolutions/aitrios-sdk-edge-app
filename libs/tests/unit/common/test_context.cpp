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

#include "context.hpp"

TEST(ContextTest, InitializeAndDestroy) {
  Context *context = Context::GetInstance();
  context->Delete();
}

TEST(ContextTest, DoubleInitialize) {
  Context *context = Context::GetInstance();
  ASSERT_EQ(context, Context::GetInstance());
}

TEST(ContextTest, DoubleDelete) {
  Context *context = Context::GetInstance();
  context->Delete();
  context->Delete();
}

TEST(ContextTest, StateManagement) {
  Context *context = Context::GetInstance();
  for (int i = 0; i < STATE_COUNT; ++i) {
    context->SetNextState((STATE)i);
    ASSERT_EQ(context->GetNextState(), (STATE)i);
  }
}

TEST(ContextTest, Notification) {
  Context *context = Context::GetInstance();
  // no notification at the beginning
  ASSERT_FALSE(context->IsPendingNotification());
  context->EnableNotification();
  ASSERT_TRUE(context->IsPendingNotification());
  context->ClearNotification();
  ASSERT_FALSE(context->IsPendingNotification());
}
