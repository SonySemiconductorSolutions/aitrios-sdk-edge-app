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

#include "bindings.hpp"
#include "data_export_types.h"
#include "send_data_types.h"
#include "sm_types.h"

void bind_enums(pybind11::module_ &m) {
  pybind11::enum_<EdgeAppLibDataExportResult>(m, "DataExportResult")
      .value("SUCCESS", EdgeAppLibDataExportResultSuccess)
      .value("ERROR", EdgeAppLibDataExportResultFailure)
      .value("Timeout", EdgeAppLibDataExportResultTimeout)
      .value("InvalidParam", EdgeAppLibDataExportResultInvalidParam)
      .value("DataTooLarge", EdgeAppLibDataExportResultDataTooLarge)
      .value("Denied", EdgeAppLibDataExportResultDenied)
      .value("Enqueued", EdgeAppLibDataExportResultEnqueued)
      .value("Uninitialized", EdgeAppLibDataExportResultUninitialized)
      .export_values();

  pybind11::enum_<EdgeAppLibSendDataType>(m, "SendDataType")
      .value("Base64", EdgeAppLibSendDataBase64)
      .value("Json", EdgeAppLibSendDataJson)
      .export_values();

  pybind11::enum_<ResponseCode>(m, "ResponseCode")
      .value("Ok", ResponseCodeOk)
      .value("Cancelled", ResponseCodeCancelled)
      .value("Unknown", ResponseCodeUnknown)
      .value("InvalidArgument", ResponseCodeInvalidArgument)
      .value("DeadlineExceeded", ResponseCodeDeadlineExceeded)
      .value("NotFound", ResponseCodeNotFound)
      .value("AlreadyExists", ResponseCodeAlreadyExists)
      .value("PermissionDenied", ResponseCodePermissionDenied)
      .value("ResourceExhausted", ResponseCodeResourceExhausted)
      .value("FailedPrecondition", ResponseCodeFailedPrecondition)
      .value("Aborted", ResponseCodeAborted)
      .value("OutOfRange", ResponseCodeOutOfRange)
      .value("Unimplemented", ResponseCodeUnimplemented)
      .value("Internal", ResponseCodeInternal)
      .value("Unavailable", ResponseCodeUnavaiable)
      .value("DataLoss", ResponseCodeDataLoss)
      .value("Unauthenticated", ResponseCodeUnauthenticated)
      .export_values();
}
