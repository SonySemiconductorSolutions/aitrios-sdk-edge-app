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
from datetime import datetime
import json
from urllib.request import urlopen
from threading import Thread

from wedge.client import Client
from wedge.state import State
from wedge.command import CommandResponseStatus
from wedge.blob import BlobMemoryReader, BlobOperationPut
from wedge.exceptions import ShouldExit

from edgeapplib.encoding.fbs_encoder import FBSEncoder
from edgeapplib.models import Detection, Classification


model_types = {
    "detection": Detection,
    "classification": Classification,
}


class BlobBufferReader(BlobMemoryReader):

    def __init__(self, client, buf):
        super().__init__(client, len(buf))
        self.buf = buf

    def handle(self, size):
        return self.buf[self.pos : self.pos + size]


class EdgeApp:

    def __init__(
        self,
        type="detection",
        client: Client = None,
        state: State = None,
    ):
        self.client = client or Client()
        self.encoder = FBSEncoder(type)
        self.state = state or State(self.client)
        self.type = model_types[type]

        self.client.commands.register(
            self.handle_start, "StartUploadInferenceData"
        )
        self.client.commands.register(
            self.handle_stop, "StopUploadInferenceData"
        )

        self.upload_image_url = None
        self.upload_inference_url = None
        self.detections = None
        self.thread = Thread(target=self.run)
        self.data = {
            "Hardware": {
                "Sensor": "IMX500",
                "SensorId": "00000000000000000000000000000000",
                "KG": "65535",
                "ApplicationProcessor": "",
                "LedOn": True,
            },
            "Version": {
                "SensorFwVersion": "910701",
                "SensorLoaderVersion": "020301",
                "DnnModelVersion": [],
                "ApLoaderVersion": "",
                "ApFwVersion": "D80001",
            },
            "Status": {
                "Sensor": "Standby",
                "ApplicationProcessor": "StreamingImage",
            },
            "OTA": {
                "SensorFwLastUpdatedDate": "",
                "SensorLoaderLastUpdatedDate": "",
                "DnnModelLastUpdatedDate": [],
                "ApFwLastUpdatedDate": "",
                "UpdateProgress": 100,
                "UpdateStatus": "Done",
            },
            "Permission": {"FactoryReset": True},
        }

        self._streaming = False
        self._results = None
        self._is_running = True

        self.thread.start()

    @property
    def is_running(self):
        return self._is_running

    @property
    def device(self):
        return self._device

    @property
    def streaming(self):
        return self._streaming

    @streaming.setter
    def streaming(self, value):
        self._streaming = value
        state = "Streaming" if value else "Standby"
        self.data["Status"]["Sensor"] = state
        self.update_state()

    def run(self):
        self._is_running = True
        try:
            while self._is_running:
                self.client.run(1000)
                print(">>> Alive <<<")
        except ShouldExit:
            print(">>> Exiting <<<")
        finally:
            self._is_running = False

    def handle_start(self, handler, reqid, name, params):
        print("Command start upload received")
        params = json.loads(params)

        self.upload_image_url = (
            "{StorageName}/{StorageSubDirectoryPath}".format(**params)
        )
        self.upload_inference_url = (
            "{StorageNameIR}/{StorageSubDirectoryPathIR}".format(**params)
        )
        print(f"StorageName: {self.upload_image_url}")
        print(f"StorageNameIR: {self.upload_inference_url}")
        handler.respond(
            reqid,
            json.dumps({"Result": "Accepted"}),
            CommandResponseStatus.OK,
            lambda *x: print("Response submitted:", x),
        )
        self.streaming = True

    def handle_stop(self, handler, reqid, name, params):
        handler.respond(
            reqid,
            json.dumps({"Result": "Accepted"}),
            CommandResponseStatus.OK,
            lambda *x: print("Response submitted:", x),
        )
        self.streaming = False

    def update_state(self):
        payload = json.dumps(self.data)
        self.state.send("placeholder", payload)

    def upload_blob(self, url, data):
        stream = BlobBufferReader(self.client, data)
        # Note:
        # This will change in the near future with better API
        # This works with wedge SDK because the upload is being executed in
        # another task than the wedge Client is running.
        req = BlobOperationPut(self.client, url, stream).request()
        with urlopen(req) as response:
            if response.status != 200:
                print(
                    f"Failed to Metadata image. Status code: {response.status_code}"
                )

    def send_image(self, data, timestamp) -> None:
        self.upload_blob(f"{self.upload_image_url}/{timestamp}.jpg", data)

    def send_inference_results(
        self,
        inference_results: list,
        timestamp: str,
    ) -> None:
        base_structure = {"Inferences": [{"T": timestamp, "O": []}]}
        flatbuffers = self.encoder.encode(inference_results)
        base_structure["Inferences"][0]["O"] = base64.b64encode(
            flatbuffers
        ).decode("utf-8")

        data = json.dumps(base_structure).encode("utf-8")
        self.upload_blob(f"{self.upload_inference_url}/{timestamp}.txt", data)

    def process_stream(self, data, results):
        if not self.streaming:
            return
        now = datetime.now()
        ts = f"{now.strftime('%Y%m%d%H%M%S')}{now.strftime('%f')[:3]}"
        self.send_image(data, ts)

        if not results:
            return

        res = self.type.convert_all(results)
        self.send_inference_results(res, ts)
