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

#include "data_export/mock_data_export.hpp"

#include <stdlib.h>

#include "data_export.h"
#include "data_export_private.h"

/* 0 = no called, 1 = called */
static int EdgeAppLibDataExportInitializeStatus = 0;
static EdgeAppLibDataExportResult EdgeAppLibDataExportInitializeReturn =
    EdgeAppLibDataExportResultSuccess;
static EdgeAppLibDataExportResult EdgeAppLibDataExportUnInitializeReturn =
    EdgeAppLibDataExportResultSuccess;
static int EdgeAppLibDataExportUnInitializeStatus = 0;
static int EdgeAppLibDataExportAwaitCalled = 0;
static int EdgeAppLibDataExportSendStateCalled = 0;
static int EdgeAppLibDataExportCleanupCalled = 0;
static int EdgeAppLibDataExportSendDataCalled = 0;
static int EdgeAppLibDataExportCancelOperationCalled = 0;
static bool EdgeAppLibDataExportIsEnabledReturn = true;

namespace EdgeAppLib {
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
EdgeAppLibDataExportResult DataExportInitialize(Context *context,
                                                void *evp_client) {
  EdgeAppLibDataExportInitializeStatus = 1;
  return EdgeAppLibDataExportInitializeReturn;
}
EdgeAppLibDataExportResult DataExportUnInitialize() {
  EdgeAppLibDataExportUnInitializeStatus = 1;
  return EdgeAppLibDataExportUnInitializeReturn;
}
EdgeAppLibDataExportFuture *DataExportSendData(
    char *portname, EdgeAppLibDataExportDataType datatype, void *data,
    int datalen, uint64_t timestamp, uint32_t current, uint32_t division) {
  EdgeAppLibDataExportSendDataCalled = 1;
  EdgeAppLibDataExportFuture *future =
      (EdgeAppLibDataExportFuture *)malloc(sizeof(EdgeAppLibDataExportFuture));
  if (datatype == EdgeAppLibDataExportRaw) {
    /* Input Tensor memory would be controlled by libs */
    free(data);
  }

  return future;
}
EdgeAppLibDataExportResult DataExportAwait(EdgeAppLibDataExportFuture *future,
                                           int timeout_ms) {
  EdgeAppLibDataExportAwaitCalled = 1;
  return EdgeAppLibDataExportResultSuccess;
}
EdgeAppLibDataExportResult DataExportSendState(const char *topic, void *state,
                                               int statelen) {
  free(state);
  EdgeAppLibDataExportSendStateCalled = 1;
  return EdgeAppLibDataExportResultSuccess;
}
EdgeAppLibDataExportResult DataExportCleanup(
    EdgeAppLibDataExportFuture *future) {
  EdgeAppLibDataExportCleanupCalled = 1;
  free(future);
  return EdgeAppLibDataExportResultSuccess;
}
EdgeAppLibDataExportResult DataExportCancelOperation(void) {
  EdgeAppLibDataExportCancelOperationCalled = 1;
  return EdgeAppLibDataExportResultSuccess;
}
bool DataExportIsEnabled(EdgeAppLibDataExportDataType) {
  return EdgeAppLibDataExportIsEnabledReturn;
}
bool DataExportHasPendingOperations() {
  if (EdgeAppLibDataExportCancelOperationCalled) {
    return true;
  } else {
    return false;
  }
}
#ifdef __cplusplus
}
#endif /* __cplusplus */
}  // namespace EdgeAppLib

int wasEdgeAppLibDataExportInitializeCalled() {
  return EdgeAppLibDataExportInitializeStatus;
}
void resetEdgeAppLibDataExportInitialize() {
  EdgeAppLibDataExportInitializeStatus = 0;
  EdgeAppLibDataExportInitializeReturn = EdgeAppLibDataExportResultSuccess;
}
void setEdgeAppLibDataExportInitializeError() {
  EdgeAppLibDataExportInitializeReturn = EdgeAppLibDataExportResultFailure;
}

int wasEdgeAppLibDataExportUnInitializeCalled() {
  return EdgeAppLibDataExportUnInitializeStatus;
}
void resetEdgeAppLibDataExportUnInitialize() {
  EdgeAppLibDataExportUnInitializeStatus = 0;
  EdgeAppLibDataExportUnInitializeReturn = EdgeAppLibDataExportResultSuccess;
}
void setEdgeAppLibDataExportUnInitializeError() {
  EdgeAppLibDataExportUnInitializeReturn = EdgeAppLibDataExportResultFailure;
}

int wasEdgeAppLibDataExportSendDataCalled() {
  return EdgeAppLibDataExportSendDataCalled;
}
void resetEdgeAppLibDataExportSendDataCalled() {
  EdgeAppLibDataExportSendDataCalled = 0;
}

int wasEdgeAppLibDataExportAwaitCalled() {
  return EdgeAppLibDataExportAwaitCalled;
}
void resetEdgeAppLibDataExportAwaitCalled() {
  EdgeAppLibDataExportAwaitCalled = 0;
}

int wasEdgeAppLibDataExportSendStateCalled() {
  return EdgeAppLibDataExportSendStateCalled;
}
void resetEdgeAppLibDataExportSendStateCalled() {
  EdgeAppLibDataExportSendStateCalled = 0;
}

int wasEdgeAppLibDataExportCleanupCalled() {
  return EdgeAppLibDataExportCleanupCalled;
}
void resetEdgeAppLibDataExportCleanupCalled() {
  EdgeAppLibDataExportCleanupCalled = 0;
}

int wasEdgeAppLibDataExportCancelOperationCalled() {
  return EdgeAppLibDataExportCancelOperationCalled;
}
void resetEdgeAppLibDataExportCancelOperationCalled() {
  EdgeAppLibDataExportCancelOperationCalled = 0;
}

void resetEdgeAppLibDataExportIsEnabled() {
  EdgeAppLibDataExportIsEnabledReturn = true;
}
void setEdgeAppLibDataExportIsEnabledDisabled() {
  EdgeAppLibDataExportIsEnabledReturn = false;
}
