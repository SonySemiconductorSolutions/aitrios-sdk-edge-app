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

FROM ubuntu:24.04

ARG EXTRA_CFLAGS

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
       --no-install-recommends \
       make \
       wget \
       ca-certificates \
       cmake \
       git \
       clang \
       llvm \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN cd /opt \
    && wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-22/wasi-sdk-22.0-linux.tar.gz \
    && tar zxvf wasi-sdk-22.0-linux.tar.gz \
    && ln -s wasi-sdk-22.0 wasi-sdk \
    && rm wasi-sdk-22.0-linux.tar.gz

RUN test ${EXTRA_CFLAGS} = '-g' && git clone https://github.com/WebAssembly/wasi-libc.git -b wasi-sdk-22 && \
    cd wasi-libc && \
    make EXTRA_CFLAGS=${EXTRA_CFLAGS} THREAD_MODEL=posix SYSROOT=/opt/wasi-sdk/share/wasi-sysroot install || \
    echo "skip building with ${EXTRA_CFLAGS}"

RUN cd /opt \
    && wget https://github.com/WebAssembly/binaryen/releases/download/version_117/binaryen-version_117-x86_64-linux.tar.gz \
    && tar zxvf binaryen-version_117-x86_64-linux.tar.gz \
    && ln -s binaryen-version_117 binaryen \
    && rm binaryen-version_117-x86_64-linux.tar.gz
