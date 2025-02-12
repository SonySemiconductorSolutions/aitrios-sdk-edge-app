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
import cv2
from picamera2 import MappedArray


def encode_picam_image(request, ext=".jpg"):
    # Access the image data from the request
    with MappedArray(request, "main") as m:
        # Convert the image to JPEG format(BGR to RGB)
        image_rgb = cv2.cvtColor(m.array, cv2.COLOR_BGR2RGB)

    success, encoded_image = cv2.imencode(ext, image_rgb)
    if success:
        return encoded_image.tobytes()

    print("Failed to encode image")
