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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

#define ACK_FILE "./data.ack"

void check_ack_file(const void *state, size_t statelen) {
  struct stat st;
  const int timeout_s = 5;  // 5 seconds timeout
  struct timespec start, current;
  clock_gettime(CLOCK_MONOTONIC, &start);
  while (stat(ACK_FILE, &st) == 0) {
    usleep(100 * 1000);  // 100ms
    clock_gettime(CLOCK_MONOTONIC, &current);
    if ((current.tv_sec - start.tv_sec) > timeout_s) {
      LOG_ERR("Timeout waiting for ACK_FILE to be removed");
      abort();
    }
  }
  // store states in logs
  FILE *f = fopen("state.logs", "a");
  fwrite(state, statelen, 1, f);
  char newline = '\n';
  fwrite(&newline, 1, 1, f);
  fclose(f);
  // create ack file
  FILE *ack = fopen(ACK_FILE, "w");
  if (ack) fclose(ack);
}
