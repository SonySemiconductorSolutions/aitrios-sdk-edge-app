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

/**
 * @file map.hpp
 * @details This file contains the declarations of the map functions. All
 * functions in the API are thread-safe.
 */

#ifndef AITRIOS_DATA_EXPORT_MYMAP_HPP
#define AITRIOS_DATA_EXPORT_MYMAP_HPP

#define MAX_FUTURES_QUEUE 100

typedef struct {
  struct EdgeAppLibDataExportFuture *future;
  void *state;
} FutureState;

typedef struct {
  void *key;
  void *value;
} MapElem;

void map_clear();

/**
 * @brief Store the value `value` with the given key `key` in the map.
 *
 * @note Linear cost.
 *
 * @param key The key associated with the value. Must not be null.
 * @param value The value to be stored. Must not be null.
 * @return 0 if the operation succeeds, -1 if the map is full.
 */
int map_set(void *key, void *value);

/**
 * @brief Retrieve and remove the item associated with the given key from the
 * map.
 *
 * @note Linear cost.
 *
 * @param key The key of the item to retrieve and remove.
 * @return A pointer to the value associated with the key, or NULL if the key is
 * not found.
 */
void *map_pop(void *key);

/**
 * @brief Retrieve the first item in the map.
 *
 * @note Linear cost.
 *
 * @return A pointer to the key of the first item in the map, or NULL if the map
 * is empty.
 */
void *map_remained();

bool map_is_empty();

#endif /* AITRIOS_DATA_EXPORT_MYMAP_HPP */
