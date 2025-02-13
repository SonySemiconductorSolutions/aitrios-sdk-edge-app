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
import json
from unittest.mock import ANY
from unittest.mock import MagicMock

import paho.mqtt.client as mqtt
import pytest
from edgeapplib.devices.device_message import Device_Message


@pytest.fixture
def make_device_message():
    client: Device_Message = MagicMock(spec=mqtt.Client)
    device_message = Device_Message(client)
    yield device_message, client


def test_send_keep_alive(make_device_message):
    device_message, client = make_device_message

    # Call the method being tested
    device_message.send_keep_alive()

    # Verify
    client.publish.assert_called_once()


def test_send_command_response(make_device_message):
    device_message, client = make_device_message

    # Call the method being tested
    request_id = "10001"
    device_message.send_command_response(request_id)

    # Verify
    response_topic = f"v1/devices/me/rpc/response/{request_id}"
    payload = json.dumps(
        {
            "direct-command-response": {
                "status": "ok",
                "reqid": request_id,
                "response": {"Result": "Accepted"},
            }
        }
    )
    client.publish.assert_called_once_with(response_topic, payload=payload)


def test_send_streaming_response(make_device_message):
    device_message, client = make_device_message

    # Call the method being tested
    streaming = True
    device_message.send_streaming_response(streaming)

    # Verify
    response_topic = "v1/devices/me/attributes"
    payload = ANY
    client.publish.assert_called_once_with(response_topic, payload=payload)
