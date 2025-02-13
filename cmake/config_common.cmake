# Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Identify wasi-sdk version using clang
# https://github.com/WebAssembly/wasi-sdk/issues/372
set(WASI_SDK_RECVER "18.1.2")

if(NOT DEFINED WASI_SDK_DIR)
  set(WASI_SDK_DIR        "/opt/wasi-sdk")
endif()

execute_process(
    COMMAND ${WASI_SDK_DIR}/bin/clang --version
    COMMAND awk "/version/{print \$3}"
    OUTPUT_VARIABLE WASI_SDK_VER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT ${WASI_SDK_VER} STREQUAL ${WASI_SDK_RECVER})
  message(WARNING "Use recommended version of wasi-sdk: 22")
endif()

set(CMAKE_C_COMPILER      "${WASI_SDK_DIR}/bin/clang")
set(CMAKE_CXX_COMPILER    "${WASI_SDK_DIR}/bin/clang++")
set(CMAKE_LINKER          "${WASI_SDK_DIR}/bin/wasm-ld")
set(CMAKE_AR              "${WASI_SDK_DIR}/bin/llvm-ar")
set(CMAKE_NM              "${WASI_SDK_DIR}/bin/llvm-nm")
set(CMAKE_OBJDUMP         "${WASI_SDK_DIR}/bin/llvm-dwarfdump")
set(CMAKE_RANLIB          "${WASI_SDK_DIR}/bin/llvm-ranlib")

set(MAX_MEMORY 10485760)
set(INITIAL_MEMORY 2097152)

if (DEFINED MAX_MEMORY)
  math(EXPR MAX_MEMORY_DIVISIBLE_BY_65536 "${MAX_MEMORY} % 65536")
  if(NOT MAX_MEMORY_DIVISIBLE_BY_65536 EQUAL 0)
    message(FATAL_ERROR "MAX_MEMORY value (${MAX_MEMORY}) is not divisible by 65536")
  endif()
endif()

set(ADDITIONAL_FLAGS "")
if (DEFINED STACK_SIZE)
    math(EXPR STACK_SIZE_DIVISIBLE_BY_16 "${STACK_SIZE} % 16")
    if(STACK_SIZE_DIVISIBLE_BY_16 EQUAL 0)
        set(ADDITIONAL_FLAGS "${ADDITIONAL_FLAGS} -z stack-size=${STACK_SIZE}")
    else()
        message(FATAL_ERROR "STACK_SIZE value (${STACK_SIZE}) is not divisible by 16")
    endif()
endif()
if (DEFINED INITIAL_MEMORY)
    math(EXPR INITIAL_MEMORY_DIVISIBLE_BY_65536 "${INITIAL_MEMORY} % 65536")
    if(INITIAL_MEMORY_DIVISIBLE_BY_65536 EQUAL 0)
        set(ADDITIONAL_FLAGS "${ADDITIONAL_FLAGS} -Wl,--initial-memory=${INITIAL_MEMORY}")
    else()
        message(FATAL_ERROR "INITIAL_MEMORY value (${INITIAL_MEMORY}) is not divisible by 65536")
    endif()
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Oz -flto -Xclang -fmerge-functions --target=wasm32-wasi-threads")

# -fno-exceptions
#   For our target (wasm), C++ exceptions are not available in general.
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-rtti")

# -Wl,--export=__main_argc_argv
#   A workaround for wasi-libc bug.
#   cf. https://github.com/WebAssembly/wasi-libc/issues/485
#
# -Wl,--export=malloc,--export=free
#   To give WAMR an access to libc heap.
#
# -Wl,--export=__heap_base,--export=__data_end
#   To help WAMR to detect the C stack.
#
# -Wl,--import-memory,--export-memory
#   wasi requires modules to export memory.
#   wasi-threads requires modules to import memory.
#
# -Wl,--export-table
#   wasi requires modules to export the function table.
#   it's necessary especially when we use callback-based host APIs
#   like EVP_setConfigurationCallback.
#   although the current WAMR implementation doesn't
#   look at the export properly, it's better to avoid
#   relying on the bug.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
  --target=wasm32-wasi-threads \
  -Wl,--allow-undefined \
  -Wl,--export=malloc,--export=free \
  -Wl,--export=__heap_base,--export=__data_end \
  -Wl,--export=__main_argc_argv \
  -Wl,--import-memory,--export-memory \
  -Wl,--export-table \
  -Wl,--max-memory=${MAX_MEMORY} \
  ${ADDITIONAL_FLAGS} \
")

if (NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
