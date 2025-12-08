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

FROM ubuntu:22.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
       --no-install-recommends \
       make \
       wget \
       ca-certificates \
       cmake \
       libjpeg-dev \
       libblas-dev \
       liblapack-dev \
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
    && wget -q https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-24/wasi-sdk-24.0-${WASI_ARCH}-linux.tar.gz \
    && tar zxf wasi-sdk-24.0-${WASI_ARCH}-linux.tar.gz \
    && ln -s wasi-sdk-24.0-${WASI_ARCH}-linux wasi-sdk \
    && rm wasi-sdk-24.0-${WASI_ARCH}-linux.tar.gz

# Detect architecture and download appropriate binaryen
RUN cd /opt \
    && ARCH=$(uname -m) \
    && wget -q https://github.com/WebAssembly/binaryen/releases/download/version_117/binaryen-version_117-${ARCH}-linux.tar.gz \
    && tar zxf binaryen-version_117-${ARCH}-linux.tar.gz \
    && ln -s binaryen-version_117 binaryen \
    && rm binaryen-version_117-${ARCH}-linux.tar.gz

# Install SensCord libcamera package
RUN cd /opt \
    && wget -q https://midokura.github.io/debian/evp-archive-keyring_jammy_amd64.deb \
    && dpkg -i ./evp-archive-keyring_jammy_amd64.deb \
    && apt update \
    && apt install senscord-libcamera \
    && rm evp-archive-keyring_jammy_amd64.deb

# Add custom wasi-libmalloc to support memory profile
COPY ./tools/virtual-machine/memory_profile/wasi-libmalloc /opt/wasi-libmalloc
RUN cd /opt/wasi-libmalloc  \
    && CC=/opt/wasi-sdk/bin/clang make libc

# Install opencv
RUN cd /opt \
    && wget -q https://github.com/opencv/opencv/archive/refs/tags/4.6.0.tar.gz \
    && tar zxf 4.6.0.tar.gz \
    && rm 4.6.0.tar.gz \
    && cd opencv-4.6.0 \
    && mkdir -p build_wasm && cd build_wasm \
    && cmake .. \
      -DWITH_TBB=OFF \
      -DWITH_OPENMP=OFF \
      -DBUILD_opencv_core_PARALLEL=OFF \
      -DWITH_PTHREADS_PF=OFF \
      -DHAVE_PARALLEL_OPENMP=0 \
      -DHAVE_PARALLEL_TBB=0 \
      -DHAVE_PARALLEL_PTHREADS=0 \
      -DHAVE_PARALLEL=0 \
      -DOPENCV_EXTRA_CXX_FLAGS="-DOPENCV_DISABLE_THREAD_SUPPORT" \
      -DWITH_PROTOBUF=OFF \
      -DBUILD_PROTOBUF=ON \
      -DWITH_ADE=OFF \
      -DCMAKE_TOOLCHAIN_FILE="/opt/wasi-sdk/share/cmake/wasi-sdk-pthread.cmake" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-D_WASI_EMULATED_PROCESS_CLOCKS" \
      -DCMAKE_CXX_FLAGS="-D_WASI_EMULATED_PROCESS_CLOCKS" \
      -DCMAKE_INSTALL_PREFIX=/opt/opencv-4.6.0-install/install-wasi \
      -DBUILD_SHARED_LIBS=OFF \
      -DBUILD_LIST=core,imgproc,imgcodecs,videoio,video,dnn \
      -DBUILD_TESTS=OFF \
      -DBUILD_PERF_TESTS=OFF \
      -DBUILD_opencv_apps=OFF \
      -DWITH_PNG=OFF \
      -DWITH_JPEG=OFF \
      -DWITH_WEBP=OFF \
      -DWITH_TIFF=OFF \
      -DWITH_OPENEXR=OFF \
      -DHAVE_OPENCV_DNN=ON \
      -DHAVE_OPENCV_VIDEOIO=ON \
      -DBUILD_opencv_objdetect=OFF \
      -DWITH_QUIRC=OFF \
      -DWITH_GDCM=OFF \
    && make VERBOSE=1 install \
    && cd /opt \
    && rm -rf /opt/opencv-4.6.0

ENV OpenCV_DIR=/opt/opencv-4.6.0-install/install-wasi/lib/cmake/opencv4/
