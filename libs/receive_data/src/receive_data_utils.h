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
#ifndef AITRIOS_RECEIVE_DATA_UTILS_H
#define AITRIOS_RECEIVE_DATA_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH_LEN 256
#define OPEN_DIR_FAILED -1
#define REMOVE_FILE_ATTEMPT 1
#define REMOVE_FILE_FAILED 9999

char *GetSuffixFromUrl(const char *url);
void ReleaseSuffixString(char *suffix);
bool IsFileHashCorrect(const char *hash, const char *path);
int RemoveOutdatedFile(const char *dir, const char *filename);

#ifdef __cplusplus
}
#endif

#endif
