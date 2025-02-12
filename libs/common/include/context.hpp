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

#ifndef CONTEXT_HPP
#define CONTEXT_HPP

typedef enum {
  STATE_CREATING = 0,
  STATE_IDLE = 1,
  STATE_RUNNING = 2,
  STATE_DESTROYING = 3,
  STATE_EXITING = 4,
  STATE_COOLINGDOWN = 5,
  STATE_APPLYING = 6,
  STATE_COUNT = 7
} STATE;

class Context {
 public:
  Context(Context &other) = delete;
  void operator=(const Context &) = delete;

  static Context *GetInstance();
  static void Delete();
  STATE GetNextState() const;
  void SetNextState(STATE next_state);
  bool IsPendingNotification();
  void ClearNotification();
  void EnableNotification();

 protected:
  Context() = default;
  virtual ~Context() = default;
  static Context *context_;
  STATE _next_state = STATE_EXITING;

 private:
  bool is_pending_notification = false;
};

#endif /* CONTEXT_HPP */
