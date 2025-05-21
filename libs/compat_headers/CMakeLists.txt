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

# The recent version of Edge Application SDK changed a way for applications
# to include its headers.
#
# Old:
#
#  #include "sm.h"
#
# New:
#
#  #include <edgeapp/sm.h>
#
# To ease the transition, this interface library tries to allow
# applications written for the earlier versions of Edge Application SDK
# as it is by adding extra -isystem compiler option.
#
# You can enable this by adding edge-app-lib-compat-headers to
# the target_link_libraries of your application.
#
# eg.
#
#   target_link_libraries(yourapp
#     edge-app-lib
#     edge-app-lib-compat-headers
#   )

add_library(edge-app-lib-compat-headers INTERFACE)
