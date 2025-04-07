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

import time
from constants import INTEGRATION_TEST_INTERVAL_SECONDS
from utils import send_data


def change_auto_exposure_metering_success(data: dict) -> None:
    print(f"DTDL auto exposure metering - success")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_success"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = 1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = 160
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = 120
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 360
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_success(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_success"
    pq_settings = data["common_settings"]["pq_settings"]
    assert pq_settings["auto_exposure_metering"]["metering_mode"] == 1
    assert pq_settings["auto_exposure_metering"]["top"] == 160
    assert pq_settings["auto_exposure_metering"]["left"] == 120
    assert pq_settings["auto_exposure_metering"]["bottom"] == 480
    assert pq_settings["auto_exposure_metering"]["right"] == 360


def change_auto_exposure_metering_mode_full(data: dict) -> None:
    print(f"DTDL auto exposure metering - mode to full")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_mode_full"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = 0
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = 160
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = 120
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 10
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 20
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_mode_full(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_mode_full"
    pq_settings = data["common_settings"]["pq_settings"]
    assert pq_settings["auto_exposure_metering"]["metering_mode"] == 0
    assert pq_settings["auto_exposure_metering"]["top"] == 160
    assert pq_settings["auto_exposure_metering"]["left"] == 120
    assert pq_settings["auto_exposure_metering"]["bottom"] == 10
    assert pq_settings["auto_exposure_metering"]["right"] == 20


def change_auto_exposure_metering_mode_overrange(data: dict) -> None:
    print(f"DTDL auto exposure metering - mode over range")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_mode_overrange"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = 2
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = 160
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = 120
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 360
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_mode_overrange(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_mode_overrange"
    assert data["res_info"]["code"] == 3    # CODE_INVALID_ARGUMENT
    assert data["res_info"]["detail_msg"] == "metering_mode not <= 1.000000"


def change_auto_exposure_metering_mode_underrange(data: dict) -> None:
    print(f"DTDL auto exposure metering - mode underrange")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_mode_underrange"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = -1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = 160
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = 120
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 360
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_mode_underrange(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_mode_underrange"
    assert data["res_info"]["code"] == 3    # CODE_INVALID_ARGUMENT
    assert data["res_info"]["detail_msg"] == "metering_mode not >= 0.000000"


def change_auto_exposure_metering_top_overrange(data: dict) -> None:
    print(f"DTDL auto exposure metering - top overrange")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_top_overrange"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = 1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = 120
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 360
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_top_overrange(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_top_overrange"
    assert data["res_info"]["code"] == 3    # CODE_INVALID_ARGUMENT
    assert data["res_info"]["detail_msg"] == (
        "top not top < bottom"
    )


def change_auto_exposure_metering_top_underrange(data: dict) -> None:
    print(f"DTDL auto exposure metering - top underrange")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_top_underrange"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = 1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = -1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = 120
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 360
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_top_underrange(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_top_underrange"
    assert data["res_info"]["code"] == 3    # CODE_INVALID_ARGUMENT
    assert data["res_info"]["detail_msg"] == (
        "top not >= 0.000000"
    )


def change_auto_exposure_metering_left_overrange(data: dict) -> None:
    print(f"DTDL auto exposure metering - left overrange")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_left_overrange"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = 1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = 160
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = 360
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 360
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_left_overrange(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_left_overrange"
    assert data["res_info"]["code"] == 3    # CODE_INVALID_ARGUMENT
    assert data["res_info"]["detail_msg"] == (
        "left not left < right"
    )


def change_auto_exposure_metering_left_underrange(data: dict) -> None:
    print(f"DTDL auto exposure metering - left underrange")
    data["req_info"]["req_id"] = "change_auto_exposure_metering_left_underrange"
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["metering_mode"] = 1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["top"] = 160
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["left"] = -1
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["bottom"] = 480
    data["common_settings"]["pq_settings"]["auto_exposure_metering"]["right"] = 360
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_auto_exposure_metering_left_underrange(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_auto_exposure_metering_left_underrange"
    assert data["res_info"]["code"] == 3    # CODE_INVALID_ARGUMENT
    assert data["res_info"]["detail_msg"] == (
        "left not >= 0.000000"
    )
