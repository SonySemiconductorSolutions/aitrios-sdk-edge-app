# BSD 2-Clause License
#
# Copyright (c) 2021, Raspberry Pi
# Copyright 2025 Sony Semiconductor Solutions Corp.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This code is based on:
# https://github.com/raspberrypi/picamera2/blob/main/examples/imx500/imx500_classification_demo.py

import argparse
import time
from typing import List

import cv2
import numpy as np
# Modification: Add import DataExport
from edgeapplib.data_export import DataExport
from picamera2 import CompletedRequest
from picamera2 import MappedArray
from picamera2 import Picamera2
from picamera2.devices.imx500 import IMX500
from picamera2.devices.imx500.postprocess import softmax

last_detections = []
LABELS = None


class Classification:
    def __init__(self, idx: int, score: float):
        """Create a Classification object, recording the idx and score."""
        self.idx = idx
        self.score = score


def get_label(request: CompletedRequest, idx: int) -> str:
    """Retrieve the label corresponding to the classification index."""
    global LABELS
    if LABELS is None:
        with open(args.labels) as f:
            LABELS = f.read().splitlines()
        assert len(LABELS) in [
            1000,
            1001,
        ], "Labels file should contain 1000 or 1001 labels."
        output_tensor_size = imx500.get_output_shapes(request.get_metadata())[0][0]
        if output_tensor_size == 1000:
            LABELS = LABELS[1:]  # Ignore the background label if present
    return LABELS[idx]


def parse_and_draw_classification_results(request: CompletedRequest):
    """Analyse and draw the classification results in the output tensor."""
    results = parse_classification_results(request)
    console.send(request, results)
    # Modification: Comment out draw function.
    # draw_classification_results(request, results)


def parse_classification_results(request: CompletedRequest) -> list[Classification]:
    """Parse the output tensor into the classification results above the threshold."""
    global last_detections
    np_outputs = imx500.get_outputs(request.get_metadata())
    if np_outputs is None:
        return last_detections
    np_output = np_outputs[0]
    if args.softmax:
        np_output = softmax(np_output)
    top_indices = np.argpartition(-np_output, 3)[
        :3
    ]  # Get top 3 indices with the highest scores
    top_indices = top_indices[
        np.argsort(-np_output[top_indices])
    ]  # Sort the top 3 indices by their scores
    last_detections = [Classification(index, np_output[index]) for index in top_indices]
    return last_detections


def draw_classification_results(
    request: CompletedRequest, results: list[Classification], stream: str = "main"
):
    """Draw the classification results for this request onto the ISP output."""
    with MappedArray(request, stream) as m:
        if args.preserve_aspect_ratio:
            # drawing roi box
            b = imx500.get_roi_scaled(request)
            cv2.putText(
                m.array,
                "ROI",
                (b.x + 5, b.y + 15),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (255, 0, 0),
                1,
            )
            cv2.rectangle(
                m.array, (b.x, b.y), (b.x + b.width, b.y + b.height), (255, 0, 0, 0)
            )
            text_left, text_top = b.x, b.y + 20
        else:
            text_left, text_top = 0, 0
        # drawing labels (in the ROI box if exist)
        for index, result in enumerate(results):
            label = get_label(request, idx=result.idx)
            text = f"{label}: {result.score:.3f}"
            cv2.putText(
                m.array,
                text,
                (text_left + 5, text_top + 15 + index * 20),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (0, 0, 255),
                1,
            )


def get_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=str, required=True, help="Path of the model")
    parser.add_argument("--fps", type=int, default=30, help="Frames per second")
    parser.add_argument(
        "-s", "--softmax", action="store_true", help="Add post-process softmax"
    )
    parser.add_argument(
        "-r",
        "--preserve-aspect-ratio",
        action="store_true",
        help="preprocess the image with preserve aspect ratio",
    )
    parser.add_argument(
        "--labels",
        type=str,
        default="assets/imagenet_labels.txt",
        help="Path to the labels file",
    )
    # Modification: Add new argument.
    parser.add_argument(
        "--mqtt_host", type=str, required=True, help="Local console MQTT Address"
    )
    parser.add_argument(
        "--mqtt_port", type=int, required=True, help="Local console MQTT Port"
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = get_args()

    # This must be called before instantiation of Picamera2
    imx500 = IMX500(args.model)

    # Modification: Add DataExport function.
    console = DataExport(args.mqtt_host, args.mqtt_port, "classification")

    picam2 = Picamera2()
    config = picam2.create_preview_configuration(
        controls={"FrameRate": args.fps}, buffer_count=28
    )
    picam2.start(config, show_preview=False)
    if args.preserve_aspect_ratio:
        imx500.set_auto_aspect_ratio()
    # Register the callback to parse and draw classification results
    picam2.pre_callback = parse_and_draw_classification_results

    while True:
        time.sleep(0.5)
