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
from datetime import datetime
from unittest.mock import call
from unittest.mock import MagicMock
from unittest.mock import Mock
from unittest.mock import patch

import numpy as np
import paho.mqtt.client as mqtt
import pytest
from edgeapplib.data_export import DataExport
from edgeapplib.data_export import Classification
from edgeapplib.data_export import Detection
from picamera2 import CompletedRequest


@pytest.fixture
def data_export():
    with (
        patch("edgeapplib.data_export.mqtt.Client") as mock_client,
        patch("threading.Timer"),
    ):
        mqtt_host = "192.168.12.34"
        mqtt_port = 1883
        task = "classification"
        data_export = DataExport(mqtt_host, mqtt_port, task)
        yield data_export, mock_client


def test_data_export():
    with (
        patch("edgeapplib.data_export.mqtt.Client") as mock_client,
        patch(
            "edgeapplib.data_export.DataExport.send_message_periodically"
        ) as mock_send_message_periodically,
        patch("threading.Timer"),
    ):
        mqtt_host = "192.168.12.34"
        mqtt_port = 1883
        task = "classification"
        data_export = DataExport(mqtt_host, mqtt_port, task)
        assert data_export.broker_host == mqtt_host
        assert data_export.broker_port == mqtt_port
        assert data_export.task == task
        mock_client.return_value.connect.assert_called_once_with(
            mqtt_host, mqtt_port, 60
        )
        mock_send_message_periodically.assert_called_once()


def test_on_connect(data_export):
    data_export, mock_client = data_export
    client = mock_client.return_value

    # Call the method being tested
    client.on_connect(client, None, {"flags": dict()}, 0)

    # Verify that the method calls the mock client's method
    client.subscribe.assert_called_once_with(data_export.topic)


def test_on_message_send_command_response(data_export):
    # Set up the mock client to return a successful message
    data_export, mock_client = data_export
    client = mock_client.return_value
    data_export.Device = Mock()

    reqID = 10001
    topic = f"v1/devices/me/attributes/request/{reqID}"
    msg = mqtt.MQTTMessage(topic=topic.encode())

    # Call the method being tested
    client.on_message(client, None, msg)

    # Verify that the method calls the mock Device's method
    data_export.Device.send_command_response.assert_called_once_with(str(reqID))


def test_on_message_startupload(data_export):
    # Set up the mock client to return a successful message
    data_export, mock_client = data_export
    client = mock_client.return_value
    data_export.Device = Mock()

    payload = {
        "method": "ModuleMethodCall",
        "params": {
            "moduleMethod": "StartUploadInferenceData",
            "moduleInstance": "backdoor-EA_Main",
            "params": {
                "Mode": 1,
                "UploadMethod": "HttpStorage",
                "StorageName": "http://192.168.11.24:37901",
                "StorageSubDirectoryPath": "images",
                "UploadMethodIR": "HttpStorage",
                "StorageNameIR": "http://192.168.11.24:37901",
                "UploadInterval": 30,
                "StorageSubDirectoryPathIR": "inferences",
                "CropHOffset": 0,
                "CropVOffset": 0,
                "CropHSize": 4056,
                "CropVSize": 3040,
            },
        },
    }
    msg = mqtt.MQTTMessage()
    msg.payload = json.dumps(payload).encode()

    # Call the method being tested
    client.on_message(client, None, msg)

    # Verify that the method calls the mock Device's method
    data_export.Device.send_streaming_response.assert_called_once_with(True)


def test_on_message_stopupload(data_export):
    # Set up the mock client to return a successful message
    data_export, mock_client = data_export
    client = mock_client.return_value
    data_export.Device = Mock()

    payload = {
        "method": "ModuleMethodCall",
        "params": {
            "moduleMethod": "StopUploadInferenceData",
            "moduleInstance": "backdoor-EA_Main",
            "params": {},
        },
    }
    msg = mqtt.MQTTMessage()
    msg.payload = json.dumps(payload).encode()

    # Call the method being tested
    client.on_message(client, None, msg)

    # Verify that the method calls the mock Device's method
    data_export.Device.send_streaming_response.assert_called_once_with(False)


def test_on_message_exception(data_export):
    # Set up the mock client to return a successful message
    data_export, mock_client = data_export
    client = mock_client.return_value
    data_export.Device = Mock()

    with (
        patch("json.loads", side_effect=json.JSONDecodeError("error", "test", 0)),
        patch("builtins.print") as mock_print,
    ):
        payload = {
            "method": "ModuleMethodCall",
            "params": {
                "moduleMethod": "StopUploadInferenceData",
                "moduleInstance": "backdoor-EA_Main",
                "params": {},
            },
        }
        msg = mqtt.MQTTMessage()
        msg.payload = json.dumps(payload).encode()

        # Call the method being tested
        client.on_message(client, None, msg)

        # Verify
        calls = [
            call(
                f"Message received: Topic={msg.topic}, Payload={msg.payload.decode()}"
            ),
            call("Failed to decode the JSON message"),
        ]
        mock_print.assert_has_calls(calls)


def test_on_message_invalid_method(data_export):
    # Set up the mock client to return a successful message
    data_export, mock_client = data_export
    client = mock_client.return_value
    data_export.Device = Mock()

    payload = {"method": "InvalidMethod", "params": {}}
    msg = mqtt.MQTTMessage()
    msg.payload = json.dumps(payload).encode()

    # Call the method being tested
    client.on_message(client, None, msg)

    # Verify that the method calls the mock Device's method
    data_export.Device.send_streaming_response.assert_not_called()


def test_send(data_export):
    data_export, _ = data_export

    with (
        patch(
            "edgeapplib.data_export.DataExport.send_image"
        ) as mock_send_image,
        patch(
            "edgeapplib.data_export.DataExport.generate_inference_data"
        ) as mock_generate_inference_data,
        patch(
            "edgeapplib.data_export.DataExport.send_inference_results"
        ) as mock_send_inference_results,
    ):
        data_export.state = "running"
        request = MagicMock(spec=CompletedRequest)
        results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]

        # Call the method being tested
        data_export.send(request, results)

        # Verify
        mock_send_image.assert_called_once()
        mock_generate_inference_data.assert_called_once()
        mock_send_inference_results.assert_called_once()


def test_send_not_running(data_export):
    data_export, _ = data_export

    with (
        patch(
            "edgeapplib.data_export.DataExport.send_image"
        ) as mock_send_image,
        patch(
            "edgeapplib.data_export.DataExport.generate_inference_data"
        ) as mock_generate_inference_data,
        patch(
            "edgeapplib.data_export.DataExport.send_inference_results"
        ) as mock_send_inference_results,
    ):
        data_export.state = "stopped"
        request = MagicMock(spec=CompletedRequest)
        results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]

        # Call the method being tested
        data_export.send(request, results)

        # Verify
        mock_send_image.assert_not_called()
        mock_generate_inference_data.assert_not_called()
        mock_send_inference_results.assert_not_called()


def test_send_exception(data_export):
    data_export, _ = data_export
    error_msg = "error"

    with (
        patch(
            "edgeapplib.data_export.DataExport.send_image",
            side_effect=Exception(error_msg),
        ) as mock_send_image,
        patch(
            "edgeapplib.data_export.DataExport.generate_inference_data"
        ) as mock_generate_inference_data,
        patch(
            "edgeapplib.data_export.DataExport.send_inference_results"
        ) as mock_send_inference_results,
        patch("builtins.print") as mock_print,
    ):
        data_export.state = "running"
        request = MagicMock(spec=CompletedRequest)
        results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]

        # Call the method being tested
        data_export.send(request, results)

        # Verify
        mock_send_image.assert_called_once()
        mock_generate_inference_data.assert_not_called()
        mock_send_inference_results.assert_not_called()
        mock_print.assert_called_once_with(
            f"An error occurred while sending data: {error_msg}"
        )


def test_send_image(data_export):
    data_export, _ = data_export
    data_export.Device = Mock()
    data_export.state = "running"

    with (
        patch("edgeapplib.data_export.requests") as mock_requests,
        patch("edgeapplib.data_export.MappedArray") as mock_mapped_array,
        patch("builtins.print") as mock_print,
    ):
        dummy_bgr = np.array(
            [[[255, 0, 0], [0, 255, 0]], [[0, 0, 255], [255, 255, 0]]], dtype=np.uint8
        )

        mock_m = mock_mapped_array.return_value.__enter__.return_value
        mock_m.array = dummy_bgr

        request = MagicMock(spec=CompletedRequest)
        results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]

        response = mock_requests.post.return_value
        response.status_code = 200

        # Call the method being tested
        data_export.send_image(request, results)

        # Verify
        mock_requests.post.assert_called_once()
        mock_print.assert_called_once_with("Image successfully uploaded.")


def test_send_image_encode_error(data_export):
    data_export, _ = data_export
    data_export.Device = Mock()
    data_export.state = "running"
    error_msg = "error"

    with (
        patch("edgeapplib.data_export.requests") as mock_requests,
        patch("edgeapplib.data_export.MappedArray") as mock_mapped_array,
        patch(
            "edgeapplib.data_export.cv2.imencode", return_value=(False, None)
        ),
        patch("builtins.print") as mock_print,
    ):
        dummy_bgr = np.array(
            [[[255, 0, 0], [0, 255, 0]], [[0, 0, 255], [255, 255, 0]]], dtype=np.uint8
        )

        mock_m = mock_mapped_array.return_value.__enter__.return_value
        mock_m.array = dummy_bgr

        request = MagicMock(spec=CompletedRequest)
        results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]

        response = mock_requests.post.return_value
        response.status_code = 200

        # Call the method being tested
        data_export.send_image(request, results)

        # Verify
        mock_requests.post.assert_not_called()
        mock_print.assert_called_once_with("Failed to encode image to JPEG format")


def test_send_image_error(data_export):
    data_export, _ = data_export
    data_export.Device = Mock()
    data_export.state = "running"

    with (
        patch("edgeapplib.data_export.requests") as mock_requests,
        patch("edgeapplib.data_export.MappedArray") as mock_mapped_array,
        patch("builtins.print") as mock_print,
    ):
        dummy_bgr = np.array(
            [[[255, 0, 0], [0, 255, 0]], [[0, 0, 255], [255, 255, 0]]], dtype=np.uint8
        )

        mock_m = mock_mapped_array.return_value.__enter__.return_value
        mock_m.array = dummy_bgr

        request = MagicMock(spec=CompletedRequest)
        results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]

        status_code = 500
        response = mock_requests.post.return_value
        response.status_code = status_code

        # Call the method being tested
        data_export.send_image(request, results)

        # Verify
        mock_requests.post.assert_called_once()
        mock_print.assert_called_once_with(
            f"Failed to upload image. Status code: {status_code}"
        )


def test_send_image_exception(data_export):
    data_export, _ = data_export
    data_export.Device = Mock()
    data_export.state = "running"
    error_msg = "error"

    with (
        patch("edgeapplib.data_export.requests") as mock_requests,
        patch("edgeapplib.data_export.MappedArray") as mock_mapped_array,
        patch(
            "edgeapplib.data_export.cv2.cvtColor",
            side_effect=Exception(error_msg),
        ),
        patch("builtins.print") as mock_print,
    ):
        dummy_bgr = np.array(
            [[[255, 0, 0], [0, 255, 0]], [[0, 0, 255], [255, 255, 0]]], dtype=np.uint8
        )

        mock_m = mock_mapped_array.return_value.__enter__.return_value
        mock_m.array = dummy_bgr

        request = MagicMock(spec=CompletedRequest)
        results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]

        response = mock_requests.post.return_value
        response.status_code = 200

        # Call the method being tested
        data_export.send_image(request, results)

        # Verify
        mock_requests.post.assert_not_called()
        mock_print.assert_called_once_with(
            f"An error occurred while sending image: {error_msg}"
        )


def test_generate_inference_data_classification(data_export):
    data_export, _ = data_export
    data_export.task = "classification"
    results = [Classification(idx=1, score=0.5), Classification(idx=2, score=0.8)]
    inference_results = data_export.generate_inference_data(results)

    assert inference_results[0]["class_id"] == 1
    assert inference_results[0]["score"] == 0.5
    assert inference_results[1]["class_id"] == 2
    assert inference_results[1]["score"] == 0.8


def test_generate_inference_data_detection(data_export):
    data_export, _ = data_export
    data_export.task = "detection"

    obj_scaled = MagicMock()
    obj_scaled.x = 0
    obj_scaled.y = 0
    obj_scaled.width = 10
    obj_scaled.height = 20
    imx500 = MagicMock()
    picam2 = MagicMock()
    imx500.convert_inference_coords.return_value = obj_scaled
    results = [
        Detection(
            coords=(0, 0, 10, 20),
            category=1,
            conf=0.5,
            metadata=None,
            imx500=imx500,
            picam2=picam2,
        )
    ]
    inference_results = data_export.generate_inference_data(results)

    assert inference_results[0]["object_id"] == 1
    assert inference_results[0]["score"] == 0.5
    assert inference_results[0]["bbox"]["left"] == 0
    assert inference_results[0]["bbox"]["top"] == 0
    assert inference_results[0]["bbox"]["right"] == 10
    assert inference_results[0]["bbox"]["bottom"] == 20


def test_generate_inference_data_none(data_export):
    data_export, _ = data_export
    data_export.task = "other"
    imx500 = MagicMock()
    picam2 = MagicMock()
    results = [
        Detection(
            coords=(0, 0, 10, 20),
            category=1,
            conf=0.5,
            metadata=None,
            imx500=imx500,
            picam2=picam2,
        )
    ]
    inference_results = data_export.generate_inference_data(results)

    assert inference_results == []


def test_send_inference_results(data_export):
    data_export, _ = data_export
    data_export.Device = Mock()

    with (
        patch("edgeapplib.data_export.requests") as mock_requests,
        patch("builtins.print") as mock_print,
    ):
        timestamp = (
            datetime.now().strftime("%Y%m%d%H%M%S") + datetime.now().strftime("%f")[:3]
        )
        filename = f"{timestamp}"
        inference_results = [
            {"class_id": 1, "score": 0.5},
            {"class_id": 2, "score": 0.8},
        ]

        response = mock_requests.post.return_value
        response.status_code = 200

        # Call the method being tested
        data_export.send_inference_results(inference_results, timestamp, filename)

        # Verify
        mock_print.assert_called_once_with("Metadata successfully uploaded.")


def test_send_inference_results_exception(data_export):
    data_export, _ = data_export
    data_export.Device = Mock()
    error_msg = "error"

    with (
        patch("edgeapplib.data_export.requests") as mock_requests,
        patch(
            "edgeapplib.data_export.FBSEncoder.encode",
            side_effect=Exception(error_msg),
        ),
        patch("builtins.print") as mock_print,
        pytest.raises(SystemExit),
    ):
        timestamp = (
            datetime.now().strftime("%Y%m%d%H%M%S") + datetime.now().strftime("%f")[:3]
        )
        filename = f"{timestamp}"
        inference_results = [
            {"class_id": 1, "score": 0.5},
            {"class_id": 2, "score": 0.8},
        ]

        response = mock_requests.post.return_value
        response.status_code = 200

        # Call the method being tested
        data_export.send_inference_results(inference_results, timestamp, filename)

        # Verify
        mock_print.assert_called_once_with(
            f"An error occurred during encoding: {error_msg}"
        )


def test_send_inference_results_fail_post(data_export):
    data_export, _ = data_export
    data_export.Device = Mock()

    with (
        patch("edgeapplib.data_export.requests") as mock_requests,
        patch("builtins.print") as mock_print,
    ):
        timestamp = (
            datetime.now().strftime("%Y%m%d%H%M%S") + datetime.now().strftime("%f")[:3]
        )
        filename = f"{timestamp}"
        inference_results = [
            {"class_id": 1, "score": 0.5},
            {"class_id": 2, "score": 0.8},
        ]

        status_code = 500
        response = mock_requests.post.return_value
        response.status_code = status_code

        # Call the method being tested
        data_export.send_inference_results(inference_results, timestamp, filename)

        # Verify
        mock_print.assert_called_once_with(
            f"Failed to Metadata image. Status code: {status_code}"
        )


def test_main():
    def mock_sleep(seconds):
        raise KeyboardInterrupt

    with (
        patch("edgeapplib.data_export.time.sleep", mock_sleep),
        patch("paho.mqtt.client") as mock_client,
        patch("threading.Timer"),
    ):
        import sys
        if "edgeapplib.data_export" in sys.modules:
            del sys.modules["edgeapplib.data_export"]

        import runpy
        runpy.run_module("edgeapplib.data_export", run_name="__main__")

        mock_client.Client.return_value.connect.assert_called_once()
        mock_client.Client.return_value.loop_start.assert_called_once()
        mock_client.Client.return_value.disconnect.assert_called_once()
