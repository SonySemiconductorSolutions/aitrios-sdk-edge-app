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

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
       --no-install-recommends \
       make \
       wget \
       ca-certificates \
       cmake \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Detect architecture and download appropriate wasi-sdk
RUN cd /opt \
    && ARCH=$(uname -m) \
    && if [ "$ARCH" = "x86_64" ]; then \
         WASI_ARCH="x86_64"; \
       elif [ "$ARCH" = "aarch64" ]; then \
         WASI_ARCH="arm64"; \
       else \
         echo "Unsupported architecture: $ARCH" && exit 1; \
       fi \
    && wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-24/wasi-sdk-24.0-${WASI_ARCH}-linux.tar.gz \
    && tar zxvf wasi-sdk-24.0-${WASI_ARCH}-linux.tar.gz \
    && ln -s wasi-sdk-24.0-${WASI_ARCH}-linux wasi-sdk \
    && rm wasi-sdk-24.0-${WASI_ARCH}-linux.tar.gz

# Detect architecture and download appropriate binaryen
RUN cd /opt \
    && ARCH=$(uname -m) \
    && wget https://github.com/WebAssembly/binaryen/releases/download/version_117/binaryen-version_117-${ARCH}-linux.tar.gz \
    && tar zxvf binaryen-version_117-${ARCH}-linux.tar.gz \
    && ln -s binaryen-version_117 binaryen \
    && rm binaryen-version_117-${ARCH}-linux.tar.gz
