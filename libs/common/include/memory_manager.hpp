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

#ifndef MEMORY_MANAGER
#define MEMORY_MANAGER

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void *xmalloc(size_t __size);

void __setMaxAllocations(int max_allocs);

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This should be kept sync with
 * https://github.com/SonySemiconductorSolutions/aitrios-sdk-device-edge-software-framework/blob/develop/src/esf/memory_manager/include/memory_manager.h
 */
typedef enum {
  kEsfMemoryManagerResultSuccess = 0,
  kEsfMemoryManagerResultParamError,
  kEsfMemoryManagerResultAllocationError,
  kEsfMemoryManagerResultMapError,
  kEsfMemoryManagerResultFileIoError,
  kEsfMemoryManagerResultNotSupport,
  kEsfMemoryManagerResultOperationError,
  kEsfMemoryManagerResultOtherError
} EsfMemoryManagerResult;

typedef uint32_t EsfMemoryManagerHandle;

#if defined(__wasm__)
__attribute__((import_module("env")))
__attribute__((import_name("EsfMemoryManagerPread")))
#endif
EsfMemoryManagerResult
EsfMemoryManagerPread(EsfMemoryManagerHandle handle, void *buf, size_t sz,
                      uint64_t offset, size_t *rsz);

/**
 * @struct MemoryRef
 * @brief Represents a reference to either Host or Wasm memory.
 *
 * This structure is used to abstract a reference to memory resources,
 * which can either be a pointer to a Wasm Linear memory region (`void *p`)
 * or a handle to a Host memory manager (`EsfMemoryManagerHandle`).
 *
 * @var type
 *     Specifies the type of memory being referenced.
 *     Example values could be:
 *       - 0: Wasm Linear memory
 *       - 1: Host memory
 *
 * @var u
 *     A union that holds the actual memory reference:
 *       - `p`: A pointer to Wasm Linear memory
 *       - `esf_handle`: A handle to the Host memory.
 *
 * This abstraction allows the same structure to manage different types of
 * memory transparently, making it easier to handle memory resources in
 * environments where both Host and Wasm memory coexist.
 */
struct MemoryRef {
  int type;
  union {
    void *p;
    EsfMemoryManagerHandle esf_handle;
  } u;
};

#define MEMORY_MANAGER_MAP_TYPE 0
#define MEMORY_MANAGER_FILE_TYPE 1

#if defined(__cplusplus)
}
#endif

#endif /* MEMORY_MANAGER */
