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
import inspect
import json
import numpy as np

from edge_app_sdk import (
    EdgeApp,
    EdgeAppError,
    SensorFrame,
    run_sm,
    stream,
    send_image,
    send_inference,
    send_state,
    set_edge_app_lib_network,
    get_configure_error_json,
    DataProcessorResultCode,
    ResponseCode,
)

# Constants
GET_FRAME_TIMEOUT = 5000
BBOX_ORDER_SIZE = 32
CLS_SCORE_SIZE = 32

# Default values for detection parameters
DEFAULT_MAX_DETECTIONS = 2
DEFAULT_THRESHOLD = 0.5
DEFAULT_INPUT_TENSOR_WIDTH = 300
DEFAULT_INPUT_TENSOR_HEIGHT = 300

@dataclass
class DataProcessorCustomParam:
    max_detections: int
    threshold: float
    input_width: int
    input_height: int
    bbox_order: str
    bbox_normalized: bool
    class_score_order: str

def log(msg: str) -> None:
    frame = inspect.currentframe().f_back
    if frame is not None:
        func_name = frame.f_code.co_name
        line_no = frame.f_lineno
        print(f"[EdgeApp][Python][{func_name}:{line_no}] {msg}", flush=True)

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

class ConfigurationManager:
    def __init__(self, model_name: str):
        self.model_name = model_name
        self.detection_param = DataProcessorCustomParam(
            max_detections=DEFAULT_MAX_DETECTIONS,
            threshold=DEFAULT_THRESHOLD,
            input_width=DEFAULT_INPUT_TENSOR_WIDTH,
            input_height=DEFAULT_INPUT_TENSOR_HEIGHT,
            bbox_order="yxyx",
            bbox_normalized=True,
            class_score_order="cls_score"
        )
        self.area: Dict[str, Any] = {}
        self.send_area_counts = False
        self.metadata_format = 1

    def validate_model_config(self, config: Dict[str, Any]) -> Tuple[bool, str, str]:
        if "ai_models" not in config or self.model_name not in config["ai_models"]:
            return False, "Error accessing AI model parameters in JSON object", config.get("res_info", {}).get("res_id", "")

        model_config = config["ai_models"][self.model_name]
        if "parameters" not in model_config:
            return False, "Error accessing AI model parameters in JSON object", config.get("res_info", {}).get("res_id", "")

        return True, "", config.get("res_info", {}).get("res_id", "")

    def validate_parameters(self, parameters: Dict[str, Any]) -> Tuple[bool, str]:
        # Validate max_detections
        max_detections = parameters.get("max_detections", self.detection_param.max_detections)
        if not isinstance(max_detections, int) or max_detections <= 0:
            return False, "Invalid max_detections parameter"

        # Validate threshold
        threshold = parameters.get("threshold", self.detection_param.threshold)
        if not isinstance(threshold, (int, float)) or threshold < 0 or threshold > 1:
            return False, "Invalid threshold parameter"

        # Validate input dimensions
        input_width = parameters.get("input_width", self.detection_param.input_width)
        input_height = parameters.get("input_height", self.detection_param.input_height)
        if not isinstance(input_width, int) or input_width <= 0 or \
           not isinstance(input_height, int) or input_height <= 0:
            return False, "Invalid input dimensions"

        return True, ""

    def update_detection_parameters(self, parameters: Dict[str, Any]) -> None:
        self.detection_param = DataProcessorCustomParam(
            max_detections=parameters.get("max_detections", self.detection_param.max_detections),
            threshold=parameters.get("threshold", self.detection_param.threshold),
            input_width=parameters.get("input_width", self.detection_param.input_width),
            input_height=parameters.get("input_height", self.detection_param.input_height),
            bbox_order=parameters.get("bbox_order", self.detection_param.bbox_order),
            bbox_normalized=parameters.get("bbox_normalized", self.detection_param.bbox_normalized),
            class_score_order=parameters.get("class_score_order", self.detection_param.class_score_order)
        )

    def update_area_settings(self, config: Dict[str, Any]) -> None:
        if "area" in config:
            area_config = config["area"]
            if "coordinates" in area_config:
                coords = area_config["coordinates"]
                self.area = {
                    "coordinates": {
                        "left": coords.get("left", 0),
                        "top": coords.get("top", 0),
                        "right": coords.get("right", 0),
                        "bottom": coords.get("bottom", 0)
                    },
                    "overlap": area_config.get("overlap", 0),
                    "class_ids": area_config.get("class_id", []),
                    "num_of_class": len(area_config.get("class_id", []))
                }
                self.send_area_counts = True
            else:
                self.send_area_counts = False
                self.area = {}
        else:
            self.send_area_counts = False
            self.area = {}

    def update_metadata_settings(self, config: Dict[str, Any]) -> None:
        if "metadata_settings" in config:
            self.metadata_format = config["metadata_settings"].get("format", 1)

    def process_configuration(self, config_str: str) -> Tuple[int, str]:
        try:
            config = json.loads(config_str)
            if not config_str or not config_str.strip():
                log("Configuration string is empty")
                return (
                    DataProcessorResultCode.InvalidParameter,
                    "Error parsing custom settings JSON"
                )

            # Validate model configuration
            success, error_msg, res_id = self.validate_model_config(config)
            if not success:
                log(f"Model config validation failed: {error_msg}")
                error_json = get_configure_error_json(
                    ResponseCode.InvalidArgument.value,
                    error_msg,
                    res_id
                )
                return DataProcessorResultCode.InvalidParameter, error_json

            model_config = config["ai_models"][self.model_name]
            parameters = model_config["parameters"]

            # Validate parameters
            success, error_msg = self.validate_parameters(parameters)
            if not success:
                log(f"Parameter validation failed: {error_msg}")
                error_json = get_configure_error_json(
                    ResponseCode.InvalidArgument.value,
                    error_msg,
                    config.get("res_info", {}).get("res_id", "")
                )
                return DataProcessorResultCode.InvalidParameter, error_json

            # Update all settings
            self.update_detection_parameters(parameters)
            self.update_area_settings(config)
            self.update_metadata_settings(config)

            # Set up network configuration
            s = stream()
            if set_edge_app_lib_network(s, model_config) != 0:
                error_msg = "Failed to set network configuration"
                log(f"{error_msg}")
                error_json = get_configure_error_json(
                    ResponseCode.InvalidArgument.value,
                    error_msg,
                    config.get("res_info", {}).get("res_id", "")
                )
                return DataProcessorResultCode.InvalidParameter, error_json

            return DataProcessorResultCode.Success, config_str

        except json.JSONDecodeError as e:
            error_msg = "Error parsing custom settings JSON"
            log(f"JSON parse error: {error_msg}: {e}")
            error_json = get_configure_error_json(
                ResponseCode.InvalidArgument.value,
                error_msg,
                ""
            )
            return DataProcessorResultCode.InvalidParameter, error_json
        except Exception as e:
            log(f"Unexpected error in process_configuration: {e}")
            error_json = get_configure_error_json(
                ResponseCode.InvalidArgument.value,
                str(e),
                ""
            )
            return DataProcessorResultCode.InvalidParameter, error_json

class DetectionProcessor:
    def __init__(self, config_manager: ConfigurationManager):
        self.config_manager = config_manager

    def extract_detection(self, frame: SensorFrame) -> Detections:
        np_outputs = frame.get_outputs()
        total_detections = int(np_outputs[3][0])
        num_boxes = np_outputs[0].size // 4
        boxes_all = np_outputs[0].reshape(4, num_boxes).T    # y0, x0, y1, x1
        boxes = boxes_all[:total_detections][:, [1, 0, 3, 2]]  # x0, y0, x1, y1
        boxes = np.clip(boxes, 0, 1)
        detections = Detections(
            boxes = boxes,
            classes = np.array(np_outputs[1][:total_detections], dtype=np.uint16),
            scores = np_outputs[2][:total_detections],
        )

        return detections[(detections.scores > self.config_manager.detection_param.threshold)]

    def parse_detections(self, image: np.ndarray, detections: Detections) -> List[Dict[str, Any]]:
        inference_results = []
        h, w, _ = image.shape
        for bbox, score, class_id in detections:
            x1, y1, x2, y2 = (bbox * [w, h, w, h]).astype(int)
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
        inference_results = self.parse_detections(image, detections)

        send_inference(inference_results, timestamp, filename)

class DetectionEdgeApp(EdgeApp):
    """Edge app implementation for object detection."""

    model_name = "detection"
    state_topic = ""

    def __init__(self):
        super().__init__()
        self.config_manager = ConfigurationManager(self.model_name)
        self.detection_processor = DetectionProcessor(self.config_manager)

    def on_configure(self, topic: str, config_str: str) -> int:
        log("on_configure")
        if not config_str or not config_str.strip():
            log("Configuration string is empty")
            return -1

        self.state_topic = topic
        result_code, out_config_json = self.config_manager.process_configuration(config_str)

        if result_code != DataProcessorResultCode.Success:
            send_state(topic, out_config_json)
            return 0 if result_code == DataProcessorResultCode.InvalidParameter else -1

        send_state(topic, config_str)
        return 0

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
    log("Running state machine...")
    exit_code = run_sm(DetectionEdgeApp)
    log(f"{exit_code=}")
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
