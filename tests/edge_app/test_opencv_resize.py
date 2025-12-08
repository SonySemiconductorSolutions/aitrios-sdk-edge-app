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

import requests
import subprocess
import argparse

from PIL import Image
import numpy as np

import os

import threading

def generate_test_cpp(in_img_path: str, in_img_name: str, out_img_path: str, out_img_name: str) -> None:
    test_codes =\
f"#include <iostream>   \n\
#include <stdlib.h>    \n\
#include <unistd.h>    \n\
#include <string>      \n\
#include <fstream>     \n\
                       \n\
#include <opencv2/opencv.hpp>          \n\
#include <opencv2/core/core.hpp>       \n\
#include <opencv2/imgproc/imgproc.hpp> \n\
                                       \n\
int main()                             \n\
{{                                     \n\
 std::cout <<\"OpenCV version:\" << CV_VERSION << std::endl;      \n\
 std::string img_path =\"{in_img_path}/{in_img_name}\";           \n\
 std::string out_path =\"{out_img_path}/{out_img_name}\";         \n\
 cv::Mat image;                                                   \n\
                                                                  \n\
 if (std::filesystem::exists(img_path))                           \n\
     std::cout <<\"Image exist!\" << img_path << std::endl;       \n\
 else {{                                                          \n\
     std::cout <<\"Image not exist!\" << img_path << std::endl;   \n\
     return -1;                                                   \n\
 }}                                                               \n\
                                                                  \n\
 std::ifstream file(img_path, std::ios::binary | std::ios::ate);  \n\
 if (!file.is_open()) {{                                          \n\
     std::cout <<\"Cannot open file!\" << std::endl;              \n\
     return -1;                                                   \n\
 }}                                                               \n\
                                                                  \n\
 std::streamsize file_size = file.tellg();                        \n\
 file.seekg(0, std::ios::beg);                                    \n\
                                                                  \n\
 std::vector<unsigned char> buf(file_size);                       \n\
 if (!file.read(reinterpret_cast<char*>(buf.data()), file_size)){{\n\
     std::cout <<\"Read file failed!\" << img_path << std::endl;  \n\
     return -1;                                                   \n\
 }}                                                               \n\
 file.close();                                                    \n\
                                                                  \n\
 if (buf.empty()) {{                                              \n\
     std::cout <<\"Buffer is NULL!\" << std::endl;                \n\
     return -1;                                                   \n\
 }}                                                               \n\
                                                                  \n\
 int i_width = 512, i_height = 512;                               \n\
 int o_width = 100, o_height = 100;                               \n\
 cv::Size ssize = cv::Size(i_width, i_height);                    \n\
 cv::Size dsize = cv::Size(o_width, o_height);                    \n\
 cv::Mat shrink;                                                  \n\
 image = cv::Mat(ssize.width, ssize.height, CV_8UC3, buf.data()); \n\
 cv::resize(image, shrink, dsize, 0, 0, cv::INTER_LINEAR);        \n\
                                                                  \n\
 std::stringstream str;                                           \n\
 str << out_path;                                                 \n\
 std::ofstream out_file(out_path, std::ios::out | std::ios::binary);\n\
 if (!out_file) {{                                                \n\
     std::cout <<\"Error opening file!\" << out_path <<std::endl; \n\
     return -1;                                                   \n\
 }}                                                               \n\
 out_file.write(reinterpret_cast<char*>(shrink.data),o_width*o_height*3);\
 out_file.close();                                                \n\
 std::cout <<\"Data written successfully!\" << std::endl;         \n\
                                                                  \n\
 return 0;                                                        \n\
}}"

    with open("opencv_resize_sample.cpp", "wb") as test_cpp:
        if isinstance(test_codes, str):
            test_codes = test_codes.encode('utf-8')
        test_cpp.write(test_codes)
    print("Write file done")

def kill_process(proc):
    proc.terminate()
#    proc.kill()

def run_in_background(command):
    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    print(f"Launch process ID: {process.pid}")
    return process

def download_single_file(raw_url, local_filename=None):
    """
    Download single file through raw url

    Args:
        raw_url (str): file url from github
        local_filename (str, optional): local file name. If None, will use file name in url
    """
    try:
        if local_filename is None:
            local_filename = raw_url.split('/')[-1]

        response = requests.get(raw_url, stream=True)
        # Raise exception if failed(404，403)
        response.raise_for_status()

        # Write binary
        with open(local_filename, 'wb') as file:
            # Write to the file in chunks to avoid excessive memory usage for large files.
            for chunk in response.iter_content(chunk_size=8192):
                file.write(chunk)

        print(f"File download successfully: {local_filename}")
        return True

    except requests.exceptions.RequestException as e:
        print(f"File download failed: {e}")
        return False

def download_iwasm() -> None:
    wamr_tag = "2.4.3" # latest 20251110
    iwasm_name = f"iwasm-{wamr_tag}-x86_64-ubuntu-22.04.tar.gz"

    subprocess.getstatusoutput(f'rm -rf {iwasm_name}')

    file_raw_url = f"https://github.com/bytecodealliance/wasm-micro-runtime/releases/download/WAMR-{wamr_tag}/{iwasm_name}"
    print(f"Downloading iwasm from: {file_raw_url}")
    download_single_file(file_raw_url, iwasm_name)
    print(f"Download iwasm done")

    exit_code, output = subprocess.getstatusoutput(f'tar -xzvf {iwasm_name}')
    if exit_code == 0:
        print("Unzip iwasm success:", output)
    else:
        print("Unzip iwasm fail:", output)

    subprocess.getstatusoutput(f'rm -rf {iwasm_name}')

def run_wasm(in_raw_path: str, out_raw_name: str, wasm_path: str) -> tuple[int, int]:
    pwd = os.getcwd()
    iwasm_dir_path = os.path.join(pwd, in_raw_path)
    cmd = f"./iwasm --dir={iwasm_dir_path} {wasm_path}"
    print(f"{cmd}")
    exit_code, output = subprocess.getstatusoutput(cmd)
    if exit_code == 0:
        print("Run iwasm success:", output)
    else:
        print("Run iwasm fail:", output)

    file_size_bytes = os.path.getsize(f"{in_raw_path}/{out_raw_name}")
    print(f"file size: {file_size_bytes} bytes")

    # RGB
    expected_bytes_per_pixel = 3
    expected_width = 100
    expected_height = 100

    expected_size = expected_width * expected_height * expected_bytes_per_pixel

    if file_size_bytes != expected_size:
        print(f"ERROR: Output file size {expected_size} bytes does not match {file_size_bytes} bytes!")
        print("Test failed")
    else:
        print(f"Output file size {expected_size}bytes match. Test passed")

    return file_size_bytes, expected_size


def image_to_raw(img_path: str, in_img_name: str, out_raw_name: str) -> None:

    img = Image.open(f"{img_path}/{in_img_name}").convert("RGB")
    print(f"cvt input: {img_path}/{in_img_name}")

    raw_data = np.array(img, dtype=np.uint8).tobytes()

    with open(f"{img_path}/{out_raw_name}", "wb") as raw_file:
        raw_file.write(raw_data)
    print(f"cvt output: {img_path}/{out_raw_name}")

def show_raw_image(img_path: str, in_raw_name: str, out_raw_name: str) -> None:
    # Make sure ImageMagick is installed in your ubuntu

    cmd1 = ["display", "-size", "512x512", "-depth", "8", f"RGB:{img_path}/{in_raw_name}"]
    cmd2 = ["display", "-size", "100x100", "-depth", "8", f"RGB:{img_path}/{out_raw_name}"]

    process1 = run_in_background(cmd1)
    timer1 = threading.Timer(2.0, kill_process, [process1])
    timer1.start()

    process2 = run_in_background(cmd2)
    timer2 = threading.Timer(2.0, kill_process, [process2])
    timer2.start()

def test_opencv_resize() -> None:
    download_iwasm()
    image_to_raw("tests/edge_app/resources", "pepper.png", "pepper.raw")
    file_size_bytes, expected_size = run_wasm("tests/edge_app/resources", "pepper_resize.raw", "./opencv_resize_sample.wasm")
    # show_raw_image("tests/edge_app/resources", "pepper.raw", "pepper_resize.raw")

    assert file_size_bytes == expected_size
