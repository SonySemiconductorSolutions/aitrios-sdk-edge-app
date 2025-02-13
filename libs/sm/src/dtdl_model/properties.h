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

#ifndef DTDL_MODEL_PROPERTIES_H
#define DTDL_MODEL_PROPERTIES_H

#define TOPIC "edge_app"

#define TOLERANCE 10e-08

typedef enum {
  CODE_OK = 0,
  CODE_CANCELLED,
  CODE_UNKNOWN,
  CODE_INVALID_ARGUMENT,
  CODE_DEADLINE_EXCEEDED,
  CODE_NOT_FOUND,
  CODE_ALREADY_EXISTS,
  CODE_PERMISSION_DENIED,
  CODE_RESOURCE_EXHAUSTED,
  CODE_FAILED_PRECONDITION,
  CODE_ABORTED,
  CODE_OUT_OF_RANGE,
  CODE_UNIMPLEMENTED,
  CODE_INTERNAL,
  CODE_UNAVAILABLE,
  CODE_DATA_LOSS,
  CODE_UNAUTHENTICATED,
} CODE;

typedef enum {
  METHOD_MQTT = 0,
  METHOD_BLOB_STORAGE,
  METHOD_HTTP_STORAGE,
} METHOD;

typedef enum { METHOD_BMP = 0, METHOD_JPEG } METHOD_FORMAT;

typedef enum {
  LEVEL_CRITICAL = 0,
  LEVEL_ERROR,
  LEVEL_WARN,
  LEVEL_INFO,
  LEVEL_DEBUG,
  LEVEL_TRACE
} LOG_LEVEL_DTDL;

#endif /* DTDL_MODEL_PROPERTIES_H */
