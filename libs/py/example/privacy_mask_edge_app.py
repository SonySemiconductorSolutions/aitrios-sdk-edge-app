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

import sys

# TODO:
# These two imports help avoid a deadlock when calling print() in a callback.
# The issue seems related to missing initialization of sys.stdout.
# Further investigation is needed to determine the root cause.
import queue
import threading

import cv2
import numpy as np

from typing import List, Tuple

from edge_app_sdk import EdgeApp
from edge_app_sdk import EdgeAppError
from edge_app_sdk import run_sm
from edge_app_sdk import stream

from utils import CLASS_LABELS, Detections, annotate_boxes, display

# Constants definitions
GET_FRAME_TIMEOUT = 5000
PERSON_CLASS_ID = 0
MOSAIC_SCALE = 0.1
CONFIDENCE_THRESHOLD = 0.5


# Person detection
def detection(frame) -> Tuple[Detections, List[str]]:
    outputs = frame.get_outputs()
    total_detections = int(outputs[3][0])
    outputs[0] = outputs[0].reshape(4, 10).T    # y0, x0, y1, x1

    boxes = outputs[0][:total_detections][:, [1, 0, 3, 2]]  # x0, y0, x1, y1
    boxes = np.clip(boxes, 0, 1)
    detections = Detections(
        boxes = boxes,
        classes = np.array(outputs[1][:total_detections], dtype=np.uint16),
        scores = outputs[2][:total_detections],
    )

    filtered_detections = detections[
        (detections.scores > CONFIDENCE_THRESHOLD) &
        (detections.classes == PERSON_CLASS_ID)
    ]
    labels = [
        f"{CLASS_LABELS[class_id]}: {score:0.2f}"
        for _, score, class_id in filtered_detections
    ]

    return filtered_detections, labels

# Apply mosaic to detected person areas
def apply_privacymask(image: np.ndarray, detections: Detections) -> np.ndarray:
    h, w, _ = image.shape

    for box, _, _ in detections:
        x1, y1, x2, y2 = (box * [w, h, w, h]).astype(int)

        mosaic_w = x2 - x1
        mosaic_h = y2 - y1

        roi = image[y1:y2, x1:x2]

        # Apply mosaic effect
        small_roi = cv2.resize(
            roi,
            (int(mosaic_w * MOSAIC_SCALE), int(mosaic_h * MOSAIC_SCALE)),
            interpolation=cv2.INTER_LINEAR
        )
        image[y1:y2, x1:x2] = cv2.resize(
            small_roi, (mosaic_w, mosaic_h), interpolation = cv2.INTER_NEAREST
        )

    return image


def process_frame(frame) -> None:
    image = frame.get_inputs()

    # Person detection
    detections, labels = detection(frame)

    # Apply mosaic
    image = apply_privacymask(image, detections)

    # Annotate image
    image = annotate_boxes(image, detections, labels)

    # Show result
    display(image)


class PrivacyMaskEdgeApp(EdgeApp):
    def on_iterate(self) -> int:
        print("[Python] on_iterate")
        try:
            s = stream()
            frame = s.get_frame(GET_FRAME_TIMEOUT)
            process_frame(frame)
            s.release_frame(frame)

        except EdgeAppError as e:
            print("[PYTHON] Failed to get frame: {e}")
            return -1

        return 0


def main():
    exit_code = run_sm(PrivacyMaskEdgeApp)
    print(f"[PYTHON] {exit_code=}")
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
