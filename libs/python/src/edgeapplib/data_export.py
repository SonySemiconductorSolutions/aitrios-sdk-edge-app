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
import base64
import json
import threading
import time
from datetime import datetime
from typing import Any
from typing import Union

import cv2
import paho.mqtt.client as mqtt
import requests
from edgeapplib.devices.device_message import Device_Message
from edgeapplib.encoding.fbs_encoder import FBSEncoder
from picamera2 import CompletedRequest
from picamera2 import MappedArray


class Classification:
    def __init__(self, idx: int, score: float) -> None:
        """Create a Classification object, recording the idx and score."""
        self.idx = idx
        self.score = score


class Detection:
    def __init__(self, coords, category, conf, metadata, imx500, picam2) -> None:
        """Create a Detection object, recording the bounding box, category and confidence."""
        self.category = category
        self.conf = conf
        obj_scaled = imx500.convert_inference_coords(coords, metadata, picam2)
        self.box = (obj_scaled.x, obj_scaled.y, obj_scaled.width, obj_scaled.height)


class DataExport:
    def __init__(self, broker_host: str, broker_port: int, task: str) -> None:
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.topic = "v1/devices/me/rpc/request/#"
        self.upload_url = ""
        self.upload_IR_url = ""
        self.client = mqtt.Client()
        self.Device = Device_Message(self.client)
        self.state = "stopped"  # Initialize the internal state
        self.task = task
        self.fbs_encoder = FBSEncoder(task)

        # Set private methods for connection and message handling
        self.client.on_connect = self.__on_connect
        self.client.on_message = self.__on_message

        # Connect to the MQTT broker
        self.client.connect(self.broker_host, self.broker_port, 60)
        self.send_message_periodically()

        # Start the loop in a non-blocking way
        self.client.loop_start()

    def __on_connect(
        self, client: mqtt.Client, userdata: Any, flags: dict[str, int], rc: int
    ) -> None:
        print(f"Connected to MQTT Broker with result code {rc}")

        # Subscribe to the topic
        client.subscribe(self.topic)

    def __on_message(
        self, client: mqtt.Client, userdata: Any, msg: mqtt.MQTTMessage
    ) -> None:
        print(f"Message received: Topic={msg.topic}, Payload={msg.payload.decode()}")

        topic_parts = msg.topic.split("/")
        if len(topic_parts) == 6 and topic_parts[4] == "request":
            request_id = topic_parts[5]
            self.Device.send_command_response(request_id)

        # Parse the received message payload
        try:
            payload = json.loads(msg.payload.decode())
            method = payload.get("method")
            params = payload.get("params", {})
            module_method = params.get("moduleMethod")

            # Check if the moduleMethod is StartUploadInferenceData
            if (
                method == "ModuleMethodCall"
                and module_method == "StartUploadInferenceData"
            ):
                # Update the internal state to running
                self.state = "running"
                print("State updated to 'running'")
                self.Device.send_streaming_response(True)
                self.upload_url = (
                    payload["params"]["params"]["StorageName"]
                    + "/"
                    + payload["params"]["params"]["StorageSubDirectoryPath"]
                )
                self.upload_IR_url = (
                    payload["params"]["params"]["StorageNameIR"]
                    + "/"
                    + payload["params"]["params"]["StorageSubDirectoryPathIR"]
                )
                print(f"StorageNameIR: {self.upload_url}")
            # Check if the moduleMethod is StartUploadInferenceData
            if (
                method == "ModuleMethodCall"
                and module_method == "StopUploadInferenceData"
            ):
                # Update the internal state to running
                self.state = "stopped"
                print("State updated to 'stopped'")
                self.Device.send_streaming_response(False)
                self.upload_url = ""
                self.upload_IR_url = ""

        except json.JSONDecodeError:
            print("Failed to decode the JSON message")

    def send(
        self,
        request: CompletedRequest,
        results: Union[list[Classification], list[Detection]],
    ) -> None:
        if self.state == "running":
            try:
                # Generate a file name based on the current date and time
                timestamp = (
                    datetime.now().strftime("%Y%m%d%H%M%S")
                    + datetime.now().strftime("%f")[:3]
                )
                filename = f"{timestamp}"

                # Send image via HTTP POST
                self.send_image(request, filename)

                # Generate inference results
                inference_results = self.generate_inference_data(results)

                # Send inference via HTTP POST
                self.send_inference_results(inference_results, timestamp, filename)

            except Exception as e:
                print(f"An error occurred while sending data: {e}")

    def send_image(self, request: CompletedRequest, filename: str) -> None:
        try:
            # Access the image data from the request
            with MappedArray(request, "main") as m:
                # Convert the image to JPEG format(BGR to RGB)
                image_rgb = cv2.cvtColor(m.array, cv2.COLOR_BGR2RGB)
                success, encoded_image = cv2.imencode(".jpg", image_rgb)
                if not success:
                    print("Failed to encode image to JPEG format")
                    return

                # Convert the encoded image to bytes once
                image_data = encoded_image.tobytes()

                # Send the image via HTTP POST
                response = requests.post(
                    f"{self.upload_url}/{filename}.jpg", data=image_data
                )
                if response.status_code == 200:
                    print("Image successfully uploaded.")
                else:
                    print(
                        f"Failed to upload image. Status code: {response.status_code}"
                    )
        except Exception as e:
            print(f"An error occurred while sending image: {e}")

    def generate_inference_data(self, results: CompletedRequest) -> list:
        inference_results = []
        if self.task == "classification":
            print("Processing as Classification")
            for obj in results:
                # label = get_label(None, idx=obj.idx)
                inference_results.append(
                    {
                        "class_id": int(obj.idx),
                        "score": round(float(obj.score), 3),
                    }
                )
        elif self.task == "detection":
            print("Processing as Detection")
            for detection in results:
                x, y, w, h = detection.box
                inference_results.append(
                    {
                        "object_id": int(detection.category),
                        "score": round(float(detection.conf), 3),
                        "bbox": {
                            "left": x,
                            "top": y,
                            "right": x + w,
                            "bottom": y + h,
                        },
                    }
                )
        return inference_results

    def send_inference_results(
        self, inference_results: list, timestamp: str, filename: str
    ) -> None:
        base_structure = {"DeviceID":"Aid-XXXXXXXX-0000-2000-9002-000000000000","ModelID":"0311030000000100","Image": True,"Inferences": [{"T": timestamp, "O": []}]}
        try:
            flatbuffers = self.fbs_encoder.encode(inference_results)
        except Exception as e:
            print(f"An error occurred during encoding: {e}")
            exit(1)
        base_structure["Inferences"][0]["O"] = base64.b64encode(flatbuffers).decode(
            "utf-8"
        )
        response = requests.post(
            f"{self.upload_IR_url}/{filename}.txt",
            data=json.dumps(base_structure),
        )
        if response.status_code == 200:
            print("Metadata successfully uploaded.")
        else:
            print(f"Failed to Metadata image. Status code: {response.status_code}")

    def send_message_periodically(self) -> None:
        self.Device.send_keep_alive()
        self._send_message_periodically()

    def _send_message_periodically(self) -> None:
        threading.Timer(180, self.send_message_periodically).start()


if __name__ == "__main__":
    broker_host = "localhost"
    broker_port = 1883
    task = "classification"

    console = DataExport(broker_host, broker_port, task)

    try:
        print("Press Ctrl+C to exit")
        while True:
            time.sleep(1)  # Sleep to reduce CPU usage
    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        console.client.loop_stop()  # Stop the MQTT loop
        console.client.disconnect()  # Cleanly disconnect from the broker
