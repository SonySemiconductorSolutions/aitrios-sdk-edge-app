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

#include "log.h"
extern "C" {
#include "sha256.h"
}

#define SHA256_BUFSIZE 4096

bool IsFileHashCorrect(const char *hash, const char *path) {
  if (!hash || strnlen(hash, SHA256_HEX_SIZE) != SHA256_HEX_SIZE - 1) {
    LOG_ERR("Wrong input hash string.");
    return false;
  }
  if (!path) {
    LOG_ERR("Wrong input file path string.");
    return false;
  }

  FILE *file = fopen(path, "rb");
  if (!file) {
    LOG_WARN("Open file %s failure.", path);
    return false;
  }

  char *buffer = (char *)malloc(SHA256_BUFSIZE);
  size_t bytes_read = 0;
  if (!buffer) {
    LOG_ERR("Memory allocation for %d bytes failed.", SHA256_BUFSIZE);
    fclose(file);
    return false;
  }

  char file_hash[SHA256_HEX_SIZE];
  sha256 ctx;
  sha256_init(&ctx);

  while ((bytes_read = fread(buffer, 1, SHA256_BUFSIZE, file)) > 0) {
    sha256_append(&ctx, buffer, bytes_read);
  }
  free(buffer);
  fclose(file);

  sha256_finalize_hex(&ctx, file_hash);
  LOG_INFO("Input hash string: %s", hash);
  LOG_INFO("File hash string: %s", file_hash);
  if (strncasecmp(hash, file_hash, SHA256_HEX_SIZE)) {
    return false;
  } else {
    return true;
  }
}
