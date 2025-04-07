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

#include "draw/mock_draw.hpp"

#include "draw.h"
static int32_t EdgeAppLibDrawRectangleReturn = 0;
static int EdgeAppLibDrawRectangleCalled = 0;

int32_t DrawRectangle(struct EdgeAppLibDrawBuffer *buffer, uint32_t left,
                      uint32_t top, uint32_t right, uint32_t bottom,
                      struct EdgeAppLibColor color) {
  EdgeAppLibDrawRectangleCalled = 1;
  return EdgeAppLibDrawRectangleReturn;
}
int wasEdgeAppLibDrawRectangleCalled() { return EdgeAppLibDrawRectangleCalled; }
void resetEdgeAppLibDrawRectangle() { EdgeAppLibDrawRectangleCalled = 0; }
