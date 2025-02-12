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
import subprocess

import paho.mqtt.client as mqtt


class Device_Message:
    def __init__(self, client: mqtt.Client) -> None:
        self.client = client

    def send_keep_alive(self) -> None:
        # Get uname -a output
        uname_output = subprocess.check_output("uname -a", shell=True).decode().strip()
        uname_parts = uname_output.split()

        # Parse uname -a output into the system info dictionary
        utsname = {
            "sysname": uname_parts[0],
            "nodename": uname_parts[1],
            "release": uname_parts[2],
            "version": " ".join(uname_parts[3:5]),  # version might be in two parts
            "machine": uname_parts[-1],
        }

        payload = {
            "systemInfo": {"utsname": utsname, "protocolVersion": "EVP1"},
            "deploymentStatus": '{"instances":{},"modules":{}}',
        }

        # Convert payload to JSON string
        payload_str = json.dumps(payload)

        # Publish to the specific topic upon connection
        self.client.publish("v1/devices/me/attributes", payload=payload_str, qos=1)

    def send_command_response(self, request_id: str) -> None:
        # Prepare the response topic
        response_topic = f"v1/devices/me/rpc/response/{request_id}"

        # Prepare the payload with the correct formatting
        payload = json.dumps(
            {
                "moduleInstance": "backdoor-EA_Main",
                "status": 0,
                "response": {"Result": "Accepted"}
            }
        )

        # Publish the response
        self.client.publish(response_topic, payload=payload)
        print(f"Published to {response_topic}: {payload}")

    def send_streaming_response(self, streaming: bool) -> None:
        topic = "v1/devices/me/attributes"

        sensor_state = "Streaming" if streaming else "Standby"

        payload = f'{{"Hardware": {{"Sensor": "IMX500", "SensorId": "00000000000000000000000000000000", "KG": "65535", "ApplicationProcessor": "", "LedOn": true}}, "Version": {{"SensorFwVersion": "910701", "SensorLoaderVersion": "020301", "DnnModelVersion": [], "ApLoaderVersion": "", "ApFwVersion": "D80001"}}, "Status": {{"Sensor": "{sensor_state}", "ApplicationProcessor": "StreamingImage"}}, "OTA": {{"SensorFwLastUpdatedDate": "", "SensorLoaderLastUpdatedDate": "", "DnnModelLastUpdatedDate": [], "ApFwLastUpdatedDate": "", "UpdateProgress": 100, "UpdateStatus": "Done"}}, "Permission": {{"FactoryReset": true}}}}'

        # Encode the JSON string to base64
        encoded_payload = base64.b64encode(payload.encode("utf-8")).decode("utf-8")

        # Publish the base64-encoded payload to the MQTT topic
        self.client.publish(
            topic,
            payload=f'{{"state/backdoor-EA_Main/placeholder": "{encoded_payload}"}}',
        )
        print(f"Published to {topic}: {encoded_payload}")
