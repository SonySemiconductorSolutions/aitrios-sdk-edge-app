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

FROM ubuntu:20.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
       make \
       libxml2 \
       wget \
       cmake \
       build-essential \
       libc6-dev-i386 gcc-multilib g++-multilib \
       libxml2-dev libedit-dev git build-essential cmake \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN cd /opt \
    && wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-22/wasi-sdk-22.0-linux.tar.gz \
    && tar zxvf wasi-sdk-22.0-linux.tar.gz \
    && ln -s wasi-sdk-22.0 wasi-sdk \
    && rm wasi-sdk-22.0-linux.tar.gz

RUN cd /opt \
    && wget https://github.com/WebAssembly/binaryen/releases/download/version_117/binaryen-version_117-x86_64-linux.tar.gz \
    && tar zxvf binaryen-version_117-x86_64-linux.tar.gz \
    && ln -s binaryen-version_117 binaryen \
    && rm binaryen-version_117-x86_64-linux.tar.gz
