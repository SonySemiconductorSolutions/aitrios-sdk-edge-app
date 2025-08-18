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

#include "log.h"

extern "C" {
void memory_usage(size_t used, size_t fordblks, size_t keepcost) {
  if (used == 0 || fordblks == 0) {
    LOG_DBG("Memory used: %zu bytes, fragmentation_rate N/A", used);
    return;
  }
  float frag_rate = 1.0f - ((float)keepcost / (float)fordblks);
  LOG_DBG("Memory used: %zu bytes, fragmentation_rate %.2f%%", used,
          frag_rate * 100.0f);
}
}
