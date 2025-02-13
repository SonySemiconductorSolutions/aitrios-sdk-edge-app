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

#include "context.hpp"

#include <stdio.h>

Context *Context::context_ = nullptr;

Context *Context::GetInstance() {
  if (context_ == nullptr) {
    context_ = new Context();
  }
  return context_;
}

void Context::Delete() {
  if (context_ != nullptr) {
    delete context_; /* LCOV_EXCL_LINE: check if null in if-statement */
  }
  context_ = nullptr;
}

STATE Context::GetNextState() const { return _next_state; }

void Context::SetNextState(STATE next_state) { _next_state = next_state; }

bool Context::IsPendingNotification() { return is_pending_notification; }

void Context::ClearNotification() { is_pending_notification = false; }

void Context::EnableNotification() { is_pending_notification = true; }
