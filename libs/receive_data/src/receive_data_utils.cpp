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

#include "receive_data_utils.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
extern "C" {
#include "sha256.h"
}

#define SHA256_BUFSIZE 4096

char *GetSuffixFromUrl(const char *url, int len) {
  const char *p = url + len;
  const char *q = nullptr;
  while (p != url) {
    p--;
    if (*p == '?') {
      q = q ? q : p;
    }
    if (*p == '/') {
      return nullptr;
    }
    if (*p == '.') {
      break;
    }
  }
  if (p == url) {
    return nullptr;
  }
  if (q == nullptr) {
    q = url + len;
  }
  char *suffix = (char *)malloc(q - p + 1);
  snprintf(suffix, q - p + 1, "%s", p);
  LOG_INFO("Suffix of file to download from %s is %s.", url, suffix);
  return suffix;
}

void ReleaseSuffixString(char *suffix) { free(suffix); }

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

static bool IsRealFilename(const char *filename, const char *real_filename) {
  if (strncmp(filename, real_filename, strlen(real_filename))) {
    return false;
  }
  if (strlen(filename) == strlen(real_filename)) {
    return true;
  } else if (strlen(filename) > strlen(real_filename)) {
    if (*(filename + strlen(real_filename)) == '.') {
      return true;
    }
  }
  return false;
}

void RemoveOutdatedFile(const char *dir, const char *filename) {
  DIR *dir_p = opendir(dir);
  if (!dir_p) {
    LOG_ERR("Open directory failed.");
    return;
  }

  struct dirent *entry;
  char filepath[MAX_PATH_LEN];
  while ((entry = readdir(dir_p)) != NULL) {
    if (entry->d_type == DT_REG && IsRealFilename(entry->d_name, filename)) {
      snprintf(filepath, MAX_PATH_LEN, "%s/%s", dir, entry->d_name);
      LOG_INFO("Remove file: %s", filepath);
      if (unlink(filepath)) {
        LOG_ERR("Remove file failed.");
      }
    }
  }

  closedir(dir_p);
}
