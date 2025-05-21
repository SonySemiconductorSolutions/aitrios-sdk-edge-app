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
from dataclasses import dataclass
from datetime import datetime
from typing import Iterator, Tuple, List, Dict, Any
import numpy as np

from edge_app_sdk import (
    EdgeApp,
    EdgeAppError,
    SensorFrame,
    run_sm,
    stream,
    send_image,
    send_inference,
)

# Constants
GET_FRAME_TIMEOUT = 5000
BBOX_ORDER_SIZE = 32
CLS_SCORE_SIZE = 32
CONFIDENCE_THRESHOLD = 0.5

def log(msg: str) -> None:
    print(msg, flush=True)

@dataclass(frozen=True)
class Detections:
    boxes: np.ndarray
    classes: np.ndarray
    scores: np.ndarray

    def __len__(self) -> int:
        return len(self.classes)

    def __getitem__(self, index) -> "Detections":
        return Detections(
            boxes=self.boxes[index],
            scores=self.scores[index],
            classes=self.classes[index]
        )

    def __iter__(self) -> Iterator[Tuple[np.ndarray, float, int]]:
        for i in range(len(self)):
            yield (
                self.boxes[i],
                float(self.scores[i]),
                int(self.classes[i]),
            )

    def __repr__(self) -> str:
        return (f"Detections(boxes={self.boxes}, "
                f"classes={self.classes}, scores={self.scores})")

class DetectionProcessor:
    def extract_detection(self, frame: SensorFrame) -> Detections:
        np_outputs = frame.get_outputs()
        total_detections = int(np_outputs[3][0])
        num_boxes = np_outputs[0].size // 4
        boxes_all = np_outputs[0].reshape(4, num_boxes).T
        boxes = boxes_all[:total_detections]  # x0, y0, x1, y1
        detections = Detections(
            boxes = boxes,
            classes = np.array(np_outputs[2][:total_detections], dtype=np.uint16),
            scores = np_outputs[1][:total_detections],
        )

        return detections[(detections.scores > CONFIDENCE_THRESHOLD)]

    def parse_detections(self, detections: Detections) -> List[Dict[str, Any]]:
        inference_results = []
        for bbox, score, class_id in detections:
            x1, y1, x2, y2 = bbox
            inference_results.append(
                {
                    "class_id": class_id,
                    "score": round(score, 3),
                    "bounding_box": {
                        "left": int(x1),
                        "top": int(y1),
                        "right": int(x2),
                        "bottom": int(y2)
                    }
                }
            )
        log(f"inference: {inference_results}")
        return inference_results

    def process_frame(self, frame: SensorFrame) -> None:
        image = frame.get_inputs()
        if image is None:
            log("No image data found in the frame")
            return

        now = datetime.now()
        timestamp = now.strftime('%Y%m%d%H%M%S%f')[:-3]
        filename = timestamp

        send_image(image, timestamp, filename)

        detections = self.extract_detection(frame)
        inference_results = self.parse_detections(detections)

        send_inference(inference_results, timestamp, filename)

class DetectionEdgeApp(EdgeApp):
    """Edge app implementation for object detection."""

    model_name = "detection"
    state_topic = ""

    def __init__(self):
        super().__init__()
        self.detection_processor = DetectionProcessor()

    def on_iterate(self) -> int:
        log("on_iterate")
        try:
            s = stream()
            frame = s.get_frame(GET_FRAME_TIMEOUT)
            try:
                self.detection_processor.process_frame(frame)
            except Exception as e:
                log(f"process_frame error: {e}")
            finally:
                s.release_frame(frame)
        except EdgeAppError as e:
            log(f"ERROR : {e}")
            return -1

        return 0

def main() -> None:
    exit_code = run_sm(DetectionEdgeApp)
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
