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

#include "memory_manager.hpp"

static int s_max_allocs = -1;
static int s_current_allocs = 0;

void *xmalloc(size_t __size) {
  if (s_max_allocs == -1 || s_current_allocs++ < s_max_allocs)
    return malloc(__size);
  return NULL;
}

void __setMaxAllocations(int max_allocs) {
  s_current_allocs = 0;
  s_max_allocs = max_allocs;
}
