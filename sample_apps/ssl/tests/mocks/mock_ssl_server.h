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

#ifndef MOCK_SSL_SERVER_H
#define MOCK_SSL_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

// Mock control functions for SSL server functions
void setMockStartSSLServerFail(int fail);
void setMockStopSSLServerFail(int fail);
void resetAllMockSSLServerMocks();

// Mock implementations for SSL server functions
int start_ssl_server(void);
int stop_ssl_server(void);

#ifdef __cplusplus
}
#endif

#endif  // MOCK_SSL_SERVER_H
