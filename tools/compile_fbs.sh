#!/bin/bash
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

IMAGE_NAME='app_build_env:1.0.0';

fbs_file_abspath=$(realpath $1)
if [ ! -f "$fbs_file_abspath" ]; then
    echo "Schema file not found: $schema_file_abspath"
fi
fbs_folder=$(dirname $fbs_file_abspath)
fbs_file_name=$(basename -- $fbs_file_abspath)

output_dir_abspath=$(realpath $2)
if [ ! -d "$output_dir_abspath" ]; then
    mkdir -p "$output_dir_abspath"
    echo "Output folder created: $output_dir_abspath"
else
    echo "Output folder already exists: $output_dir_abspath"
fi

if [ ! "$(docker image ls -q "$IMAGE_NAME")" ]; then
    cd "$(dirname "$0")"
    docker build .. -f $PWD/../Dockerfile -t $IMAGE_NAME --network host
fi

docker run --rm \
    -v $fbs_folder/:/root/schema_folder/ \
    -v $output_dir_abspath/:/root/output_folder/ \
    $IMAGE_NAME \
    /bin/sh -c \
    "flatc --cpp -o /root/output_folder/ /root/schema_folder/$fbs_file_name"
