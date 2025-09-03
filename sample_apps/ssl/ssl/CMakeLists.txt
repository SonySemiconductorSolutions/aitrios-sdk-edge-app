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

# Define directory paths
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(SAMPLE_APP_DIR ${ROOT_DIR}/sample_apps)
set(LIBS_DIR ${ROOT_DIR}/libs)

# Configure mbedTLS
set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbedTLS programs")
set(ENABLE_TESTING OFF CACHE BOOL "Build mbedTLS tests")

# Check if building for WASM (WASI) or native
if(CMAKE_C_COMPILER_TARGET MATCHES "wasm")
    # Set WASI-specific flags for WAMR compatibility
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WASI_EMULATED_PROCESS_CLOCKS -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_GETPID -D__unix__")

    # Download and configure wamr-wasi-extensions using FetchContent
    set(URL https://github.com/bytecodealliance/wasm-micro-runtime/releases/download/WAMR-2.4.1/wamr-wasi-extensions-2.4.1.zip)

    include(FetchContent)
    FetchContent_Declare(
        wamr-wasi-extensions
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL ${URL}
    )
    FetchContent_MakeAvailable(wamr-wasi-extensions)
    list(APPEND CMAKE_PREFIX_PATH ${wamr-wasi-extensions_SOURCE_DIR})

    set(SSL_WASM_BUILD TRUE)
    message(STATUS "Building SSL components for WASM/WASI environment")
else()
    set(SSL_WASM_BUILD FALSE)
    message(STATUS "Building SSL components for native environment")
endif()

# Add third-party dependencies
add_subdirectory(${SAMPLE_APP_DIR}/ssl/third_party/mbedtls mbedtls)
add_subdirectory(${LIBS_DIR}/third_party/parson parson)

# Add SSL component subdirectories
add_subdirectory(ssl_client)
add_subdirectory(ssl_server)
