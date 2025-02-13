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
#include "mock_process_format.hpp"

/* 0 = no called, 1 = called */
static int EdgeAppLibSendDataSyncMetaCalled = 0;

static EdgeAppLibSendDataResult SendDataSyncMetaSuccess =
    EdgeAppLibSendDataResultSuccess;

void setSendDataSyncMetaFail(EdgeAppLibSendDataResult result) {
  SendDataSyncMetaSuccess = result;
}

void resetSendDataSyncMetaSuccess() {
  SendDataSyncMetaSuccess = EdgeAppLibSendDataResultSuccess;
}

int wasEdgeAppLibSendDataSyncMetaCalled() {
  return EdgeAppLibSendDataSyncMetaCalled;
}

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif

EdgeAppLibSendDataResult SendDataSyncMeta(void *data, int datalen,
                                          EdgeAppLibSendDataType datatype,
                                          uint64_t timestamp, int timeout_ms) {
  EdgeAppLibSendDataSyncMetaCalled = 1;
  if (SendDataSyncMetaSuccess != EdgeAppLibSendDataResultSuccess)
    return SendDataSyncMetaSuccess;
  return EdgeAppLibSendDataResultSuccess;
}

#ifdef __cplusplus
}
#endif

}  // namespace EdgeAppLib
