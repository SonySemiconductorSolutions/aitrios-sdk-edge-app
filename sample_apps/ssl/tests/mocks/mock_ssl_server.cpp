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

#include "mock_ssl_server.h"

// Mock control variables
static bool mock_start_ssl_server_fail = false;
static bool mock_stop_ssl_server_fail = false;

// Mock control functions
void setMockStartSSLServerFail(int fail) { mock_start_ssl_server_fail = fail; }

void setMockStopSSLServerFail(int fail) { mock_stop_ssl_server_fail = fail; }

void resetAllMockSSLServerMocks() {
  mock_start_ssl_server_fail = false;
  mock_stop_ssl_server_fail = false;
}

// Mock implementations for SSL server functions

int start_ssl_server(void) {
  // Mock start with configurable success/failure
  if (mock_start_ssl_server_fail) {
    return -1;  // Mock failure
  }
  // Mock successful start
  return 0;
}

int stop_ssl_server(void) {
  // Mock stop with configurable success/failure
  if (mock_stop_ssl_server_fail) {
    return -1;  // Mock failure
  }
  // Mock successful stop
  return 0;
}
