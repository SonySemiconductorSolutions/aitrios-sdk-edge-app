# BSD 2-Clause License
#
# Copyright (c) 2021, Raspberry Pi
# Copyright 2024 Sony Semiconductor Solutions Corp.
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
# https://github.com/raspberrypi/picamera2/blob/main/examples/imx500/imx500_object_detection_demo.py

import argparse
import time
from functools import lru_cache

import cv2
import numpy as np
# Modification: Add import DataExport
from edgeapplib.data_export import DataExport
from picamera2 import MappedArray
from picamera2 import Picamera2
from picamera2.devices.imx500 import IMX500
from picamera2.devices.imx500 import postprocess_efficientdet_lite0_detection
from picamera2.devices.imx500 import postprocess_nanodet_detection
from picamera2.devices.imx500 import postprocess_yolov5_detection
from picamera2.devices.imx500 import postprocess_yolov8_detection

last_detections = []


class Detection:
    def __init__(self, coords, category, conf, metadata):
        """Create a Detection object, recording the bounding box, category and confidence."""
        self.category = category
        self.conf = conf
        obj_scaled = imx500.convert_inference_coords(coords, metadata, picam2)
        self.box = (obj_scaled[0], obj_scaled[1], obj_scaled[2], obj_scaled[3])


def parse_and_draw_detections(request):
    """Analyse the detected objects in the output tensor and draw them on the main output image."""
    detections = parse_detections(request.get_metadata())
    console.send(request, detections)
    # Modification: Comment out draw function.
    # draw_detections(request, detections)


def parse_detections(metadata: dict):
    """Parse the output tensor into a number of detected objects, scaled to the ISP out."""
    global last_detections
    bbox_normalization = args.bbox_normalization
    threshold = args.threshold
    iou = args.iou
    max_detections = args.max_detections

    np_outputs = imx500.get_outputs(metadata, add_batch=True)
    input_w, input_h = imx500.get_input_size()
    if np_outputs is None:
        return last_detections
    if args.postprocess == "efficientdet_lite0":
        boxes, scores, classes = postprocess_efficientdet_lite0_detection(
            outputs=np_outputs,
            conf_thres=threshold,
            iou_thres=iou,
            max_out_dets=max_detections,
        )
        from picamera2.devices.imx500.postprocess_yolov5 import scale_boxes

        boxes = scale_boxes(boxes, 1, 1, input_h, input_w, False)

    elif args.postprocess == "yolov5n":
        boxes, scores, classes = postprocess_yolov5_detection(
            outputs=np_outputs,
            conf_thres=threshold,
            iou_thres=iou,
            max_out_dets=max_detections,
        )
        from picamera2.devices.imx500.postprocess_yolov5 import scale_boxes

        boxes = scale_boxes(boxes, 1, 1, input_h, input_w, False)

    elif args.postprocess == "yolov8n":
        boxes, scores, classes = postprocess_yolov8_detection(
            outputs=np_outputs,
            conf=threshold,
            iou_thres=iou,
            max_out_dets=max_detections,
        )[0]
        boxes = boxes / input_h
    elif args.postprocess == "nanodet":
        boxes, scores, classes = postprocess_nanodet_detection(
            outputs=np_outputs[0],
            conf=threshold,
            iou_thres=iou,
            max_out_dets=max_detections,
        )[0]
        from picamera2.devices.imx500.postprocess import scale_boxes

        boxes = scale_boxes(boxes, 1, 1, input_h, input_w, False, False)
    else:
        boxes, scores, classes = np_outputs[0][0], np_outputs[1][0], np_outputs[2][0]
        if bbox_normalization:
            boxes = boxes / input_h

        boxes = np.array_split(boxes, 4, axis=1)
        boxes = zip(*boxes)

    last_detections = [
        Detection(box, category, score, metadata)
        for box, score, category in zip(boxes, scores, classes)
        if score > threshold
    ]
    return last_detections


@lru_cache
def get_labels():
    with open(args.labels) as f:
        labels = f.read().split("\n")

    if args.ignore_dash_labels:
        labels = [l for l in labels if l and l != "-"]
    return labels


def draw_detections(request, detections, stream="main"):
    """Draw the detections for this request onto the ISP output."""
    labels = get_labels()
    with MappedArray(request, stream) as m:
        for detection in detections:
            x, y, w, h = detection.box
            label = f"{labels[int(detection.category)]} ({detection.conf:.2f})"
            cv2.putText(
                m.array,
                label,
                (x + 5, y + 15),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (0, 0, 255),
                1,
            )
            cv2.rectangle(m.array, (x, y), (x + w, y + h), (0, 0, 255, 0))
        if args.preserve_aspect_ratio:
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


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=str, required=True, help="Path of the model")
    parser.add_argument("--fps", type=int, default=30, help="Frames per second")
    parser.add_argument(
        "--bbox-normalization", action="store_true", help="Normalize bbox"
    )
    parser.add_argument(
        "--threshold", type=float, default=0.55, help="Detection threshold"
    )
    parser.add_argument("--iou", type=float, default=0.65, help="Set iou threshold")
    parser.add_argument(
        "--max-detections", type=int, default=10, help="Set max detections"
    )
    parser.add_argument(
        "--ignore-dash-labels", action="store_true", help="Remove '-' labels "
    )
    parser.add_argument(
        "--postprocess",
        choices=["yolov8n", "yolov5n", "nanodet", "efficientdet_lite0"],
        default=None,
        help="Run post process of type",
    )
    parser.add_argument(
        "-r",
        "--preserve-aspect-ratio",
        action="store_true",
        help="preprocess the image with  preserve aspect ratio",
    )
    parser.add_argument(
        "--labels",
        type=str,
        default="assets/coco_labels.txt",
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
    console = DataExport(args.mqtt_host, args.mqtt_port, "detection")

    picam2 = Picamera2()
    config = picam2.create_preview_configuration(
        controls={"FrameRate": args.fps}, buffer_count=28
    )
    picam2.start(config, show_preview=False)
    if args.preserve_aspect_ratio:
        imx500.set_auto_aspect_ratio()
    picam2.pre_callback = parse_and_draw_detections
    while True:
        time.sleep(0.5)
