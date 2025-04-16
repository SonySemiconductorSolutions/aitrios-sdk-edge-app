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
import numpy as np
from dataclasses import dataclass
from typing import Iterator, List, Tuple

# Class labels of the objects to detect
CLASS_LABELS = [
    "person",
    "bicycle",
    "car"
]

@dataclass
class Detections():
    boxes: np.ndarray
    classes: np.ndarray
    scores: np.ndarray

    def __getitem__(self, index) -> "Detections":
        return Detections(
            boxes=self.boxes[index],
            scores=self.scores[index],
            classes=self.classes[index]
        )

    def __len__(self) -> int:
        return len(self.classes)

    def __iter__(self) -> Iterator[Tuple[np.ndarray, float, int]]:
        for i in range(len(self)):
            yield (
                self.boxes[i],
                self.scores[i],
                self.classes[i]
            )

def annotate_boxes(image: np.ndarray, detections: Detections, labels: List[str]) -> np.ndarray:
    h, w, _ = image.shape

    for i, (box, _, _) in enumerate(detections):
        x1, y1, x2, y2 = (box * [w, h, w, h]).astype(int)

        cv2.rectangle(image, (x1, y1), (x2, y2), color=(0, 255, 0), thickness=1)
        set_label(image, x1, y1, (0, 255, 0), labels[i])

    return image

def set_label(image, x: int, y: int, color: Tuple[int, int, int], label:str) -> None:
    font = cv2.FONT_HERSHEY_SIMPLEX
    text_width, text_height = cv2.getTextSize(
        text=label,
        fontFace=font,
        fontScale=0.5,
        thickness=1,
    )[0]

    text_padding = 10
    text_x = x + text_padding
    text_y = y - text_padding

    text_background_x1 = x
    text_background_y1 = y - 2 * text_padding - text_height

    text_background_x2 = x + 2 * text_padding + text_width
    text_background_y2 = y

    # Draw background rectangle
    cv2.rectangle(
        img=image,
        pt1=(text_background_x1, text_background_y1),
        pt2=(text_background_x2, text_background_y2),
        color=color,
        thickness=cv2.FILLED,
    )

    # Draw text
    cv2.putText(
        img=image,
        text=label,
        org=(text_x, text_y),
        fontFace=font,
        fontScale=0.5,
        color=(255, 255, 255),
        thickness=1,
        lineType=cv2.LINE_AA,
    )

def display(image) -> None:
    cv2.imshow("Edge App", cv2.cvtColor(image, cv2.COLOR_BGR2RGB))
    cv2.waitKey(1)
