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
#include <stdint.h>

#include <string>

#include "log.h"

float *StringToFloatArray(const char *inputString,
                          uint32_t *num_array_elements) {
  size_t count = 0;
  const char *p = inputString;

  while (*p) {
    if (*p == ',') {
      count++;
    }
    p++;
  }
  if (count == 0) {
    LOG_DBG("No values in output tensor");
    return NULL;
  }

  *num_array_elements = count + 1;

  float *floatArray = (float *)malloc((count + 1) * sizeof(float));

  size_t i = 0;
  const char *start = inputString;
  char *end;

  while (*start) {
    if (*start == '[' || *start == ',' || *start == ' ') {
      start++;
      continue;
    }

    if (*start == ']' || *start == '\0') {
      break;
    }

    floatArray[i++] = strtof(start, &end);
    start = end;
  }

  return floatArray;
}
