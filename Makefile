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

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

MODULE_NAME = edge_app

ROOTDIR = /rearch
BINDIR = $(ROOTDIR)/bin
BUILDDIR = $(ROOTDIR)/build
TARGET=$(BINDIR)/$(MODULE_NAME).wasm
WASM_OPT = /opt/binaryen/bin/wasm-opt

IMAGE_NAME = app_build_env:2.0.0
IMAGE = $(shell docker image ls -q $(IMAGE_NAME))

ifeq ($(DEBUG_AOT), 1)
EXTRA_CFLAGS += "-g"
endif

.PHONY: all
all: docker_build
	docker run --rm \
	  -v $(MAKEFILE_DIR):$(ROOTDIR) \
	  $(IMAGE_NAME) \
	  	/bin/sh -c "\
		mkdir -p $(BUILDDIR) && \
		cd $(BUILDDIR) && \
		cmake -DCMAKE_TOOLCHAIN_FILE=/opt/wasi-sdk/share/cmake/wasi-sdk-pthread.cmake $(CMAKE_FLAGS) .. && make && \
		if [ \"$(DEBUG_AOT)\" = \"1\" ]; then \
			echo 'DEBUG_AOT is enabled: Copying $(BUILDDIR)/$(MODULE_NAME) to $(TARGET)'; \
			cp $(BUILDDIR)/$(MODULE_NAME) $(TARGET); \
		else \
			echo 'DEBUG_AOT is disabled: Running WASM_OPT on $(BUILDDIR)/$(MODULE_NAME)'; \
			$(WASM_OPT) -Oz -o $(TARGET) $(BUILDDIR)/$(MODULE_NAME); \
		fi"




.PHONY: clean
clean:
	docker run --rm \
	  -v $(MAKEFILE_DIR):$(ROOTDIR) \
	  $(IMAGE_NAME) \
		/bin/sh -c \
		"rm -rf $(TARGET)"

.PHONY: cleanall
cleanall: clean
	docker run --rm \
	  -v $(MAKEFILE_DIR):$(ROOTDIR) \
	  $(IMAGE_NAME) \
		/bin/sh -c "\
		rm -rf $(BUILDDIR)"
	docker rmi $(IMAGE_NAME)

.PHONY: docker_build
docker_build:
ifeq ($(IMAGE),)
	docker build . --build-arg EXTRA_CFLAGS=$(EXTRA_CFLAGS) -f $(MAKEFILE_DIR)Dockerfile -t $(IMAGE_NAME)
endif
