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

#include "map.hpp"

#include <pthread.h>

static MapElem map_vec[MAX_FUTURES_QUEUE] = {{nullptr, nullptr}};

static pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

int map_set(void *key, void *value) {
  pthread_mutex_lock(&map_mutex);
  bool is_set = false;
  for (int i = 0; i < MAX_FUTURES_QUEUE; ++i) {
    if (map_vec[i].key == nullptr) {
      map_vec[i].key = key;
      map_vec[i].value = value;
      is_set = true;
      break;
    }
  }
  pthread_mutex_unlock(&map_mutex);
  return is_set ? 0 : -1;
}

void *map_remained() {
  pthread_mutex_lock(&map_mutex);
  void *res = nullptr;
  for (int i = 0; res == nullptr and i < MAX_FUTURES_QUEUE; ++i) {
    if (map_vec[i].key != nullptr) {
      res = map_vec[i].key;
    }
  }
  pthread_mutex_unlock(&map_mutex);
  return res;
}

void *map_pop(void *key) {
  pthread_mutex_lock(&map_mutex);
  void *res = nullptr;
  for (int i = 0; res == nullptr and i < MAX_FUTURES_QUEUE; ++i) {
    if (map_vec[i].key == key) {
      res = map_vec[i].value;
      map_vec[i] = (MapElem){.key = nullptr, .value = nullptr};
    }
  }
  pthread_mutex_unlock(&map_mutex);
  return res;
}

bool map_is_empty() {
  pthread_mutex_lock(&map_mutex);
  bool element_found = false;
  for (int i = 0; not element_found and i < MAX_FUTURES_QUEUE; ++i)
    element_found = map_vec[i].key != nullptr;
  pthread_mutex_unlock(&map_mutex);
  return not element_found;
}

void map_clear() {
  pthread_mutex_lock(&map_mutex);
  for (int i = 0; i < MAX_FUTURES_QUEUE; ++i)
    map_vec[i] = (MapElem){.key = nullptr, .value = nullptr};
  pthread_mutex_unlock(&map_mutex);
}
