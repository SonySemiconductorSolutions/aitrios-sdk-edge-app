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

#include "testing_utils.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

float **StringToFloatArrayForIT(const char *inputString,
                                uint32_t *num_array_elements,
                                uint32_t **out_lengths) {
  const char *p = inputString;
  float **result = NULL;
  uint32_t *lengths = NULL;
  uint32_t count = 0;

  while ((p = strchr(p, '[')) != NULL) {
    if (*(p + 1) == '[') {
      p++;  // skip outer [[
      continue;
    }

    const char *start = p + 1;
    const char *end = strchr(start, ']');
    if (!end) break;

    size_t len = end - start;
    char *line = (char *)malloc(len + 1);
    strncpy(line, start, len);
    line[len] = '\0';

    // Parse this line into a float array
    uint32_t cap = 32, len_count = 0;
    float *array = (float *)malloc(cap * sizeof(float));
    char *tok = strtok(line, ",");
    while (tok) {
      if (len_count >= cap) {
        cap *= 2;
        array = (float *)realloc(array, cap * sizeof(float));
      }
      array[len_count++] = strtof(tok, NULL);
      tok = strtok(NULL, ",");
    }

    result = (float **)realloc(result, (count + 1) * sizeof(float *));
    lengths = (uint32_t *)realloc(lengths, (count + 1) * sizeof(uint32_t));
    result[count] = array;
    lengths[count] = len_count;
    count++;

    free(line);
    p = end + 1;
  }

  *num_array_elements = count;
  *out_lengths = lengths;
  return result;
}
