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
import argparse
import numpy as np
from edge_app_sdk import (
    EdgeApp,
    EdgeAppError,
    stream,
    send_input_tensor,
    send_metadata,
    run_sm,
)


GET_FRAME_TIMEOUT = 5000
IN_INTEGRATION_TEST = False


def log(msg: str):
    if not IN_INTEGRATION_TEST:
        print(msg, flush=True)


class SimpleEdgeApp(EdgeApp):
    """Implements the event functions of our Edge App."""

    def on_create(self) -> int:
        log("[PYTHON] on_create")
        return 0

    def on_configure(self, topic: str, config_str: str) -> int:
        log("[PYTHON] on_configure")
        return 0

    def on_iterate(self) -> int:
        log("[PYTHON] on_iterate")
        try:
            s = stream()
            frame = s.get_frame(GET_FRAME_TIMEOUT)

            log(f"[PYTHON] send_input_tensor({frame=})")
            send_input_tensor(frame)
            log(f"[PYTHON] send_metadata({frame=})")
            send_metadata(frame)

            try:
                # AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT
                channel = frame.get_channel(0)
                log(f"[PYTHON] {channel=}")
                log(f"[PYTHON] {channel.raw_data=}")
                log(f"[PYTHON] {type(channel.raw_data)=}")
                log(f"[PYTHON] {channel.data=}")
                log(f"[PYTHON] {type(channel.data)=}")

                max_value = np.max(channel.data)
                log(f"[PYTHON] {max_value=}")
            except EdgeAppError:
                pass  # channel might not be available and throw an error, it's ok to continue though

            s.release_frame(frame)
        except EdgeAppError as e:
            log(f"[PYTHON] ERROR : {e}")
            return -1

        return 0

    def on_stop(self) -> int:
        log("[PYTHON] on_stop")
        return 0

    def on_start(self) -> int:
        log("[PYTHON] on_start")

        if not IN_INTEGRATION_TEST:
            s = stream()
            try:
                log(f"[PYTHON]  {s.image_crop=}")
                crop = s.image_crop
                crop.left = 5
                crop.top = 10
                crop.width = 20
                crop.height = 20
                s.image_crop = crop
                log(f"[PYTHON]  {s.image_crop=}")
            except EdgeAppError:
                log("[PYTHON] image_crop property not supported")

        return 0

    def on_destroy(self) -> int:
        log("[PYTHON] on_destroy")
        return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-k", "--stream-key", required=False)
    parser.add_argument("--integration-test", action="store_true")
    args = parser.parse_args()
    stream_key = args.stream_key or None

    global IN_INTEGRATION_TEST
    IN_INTEGRATION_TEST = args.integration_test

    log("[PYTHON] Running state machine...")
    exit_code = run_sm(SimpleEdgeApp, stream_key)
    log(f"[PYTHON] {exit_code=}")
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
