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
import time

import check_auto_exposure_metering as ae_metering
from utils import send_data
from state_checker import DTDLStateChecker
from constants import INTEGRATION_TEST_INTERVAL_SECONDS


def change_crop_normal(data: dict) -> None:
    print(f"DTDL image_cropping {1}:")
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "image_cropping"
    data["common_settings"]["pq_settings"]["image_cropping"]["width"] = 128
    data["common_settings"]["pq_settings"]["image_cropping"]["height"] = 96
    data["common_settings"]["pq_settings"]["image_cropping"]["left"] = 1901
    data["common_settings"]["pq_settings"]["image_cropping"]["top"] = 1424
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_crop_normal(data: dict) -> None:
    assert data["res_info"]["code"] == 0
    assert data["res_info"]["res_id"] == "image_cropping"
    assert data["res_info"]["detail_msg"] == ""
    assert data["common_settings"]["pq_settings"]["image_cropping"]["width"] == 128
    assert data["common_settings"]["pq_settings"]["image_cropping"]["height"] == 96
    assert data["common_settings"]["pq_settings"]["image_cropping"]["left"] == 1901
    assert data["common_settings"]["pq_settings"]["image_cropping"]["top"] == 1424


def change_crop_left_error(data: dict) -> None:
    print(f"DTDL image_cropping left {1}:")
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "image_cropping left lessmin -1"
    # error value.
    data["common_settings"]["pq_settings"]["image_cropping"]["left"] = -1

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_crop_left_error(data: dict) -> None:
    assert data["res_info"]["code"] == 3
    assert data["res_info"]["res_id"] == "image_cropping left lessmin -1"
    assert data["res_info"]["detail_msg"] == "left not >= 0.000000"


def change_crop_top_error(data: dict) -> None:
    print(f"DTDL image_cropping top {1}:")
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "image_cropping top lessmin -1"
    # error value.
    data["common_settings"]["pq_settings"]["image_cropping"]["top"] = -1

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_crop_top_error(data: dict) -> None:
    assert data["res_info"]["code"] == 3
    assert data["res_info"]["res_id"] == "image_cropping top lessmin -1"
    assert data["res_info"]["detail_msg"] == "top not >= 0.000000"


def change_crop_width_error(data: dict) -> None:
    print(f"DTDL image_cropping width {1}:")
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "image_cropping width lessmin -1"
    # error value.
    data["common_settings"]["pq_settings"]["image_cropping"]["width"] = -1

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_crop_width_error(data: dict) -> None:
    assert data["res_info"]["code"] == 3
    assert data["res_info"]["res_id"] == "image_cropping width lessmin -1"
    assert data["res_info"]["detail_msg"] == "width not >= 0.000000"


def change_crop_height_error(data: dict) -> None:
    print(f"DTDL image_cropping height {1}:")
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "image_cropping height lessmin -1"
    # error value.
    data["common_settings"]["pq_settings"]["image_cropping"]["height"] = -1

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_crop_height_error(data: dict) -> None:
    assert data["res_info"]["code"] == 3
    assert data["res_info"]["res_id"] == "image_cropping height lessmin -1"
    assert data["res_info"]["detail_msg"] == "height not >= 0.000000"


def change_flip_normal(data: dict) -> None:
    print(f"DTDL camera_image_flip {1}:")
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "camera_image_flip"
    data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_horizontal"] = 1
    data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_vertical"] = 1

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def change_rotation_normal(data: dict) -> None:
    print(f"DTDL image_rotation {1}:")
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "image_rotation"
    data["common_settings"]["pq_settings"]["image_rotation"] = 3

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_flip_normal(data: dict) -> None:
    assert data["res_info"]["code"] == 0
    assert data["res_info"]["res_id"] == "camera_image_flip"
    assert data["res_info"]["detail_msg"] == ""
    assert data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_horizontal"] == 1
    assert data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_vertical"] == 1


def change_flip_horizontal_error(data: dict, error_type: str) -> None:
    data["common_settings"]["process_state"] = 1
    if error_type == "lessmin":
        print(f"DTDL camera_image_flip flip_horizontal {1}:")
        data["req_info"]["req_id"] = "camera_image_flip flip_horizontal lessmin -1"
        # error value.
        data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_horizontal"] = -1
    elif error_type == "overmax":
        print(f"DTDL camera_image_flip flip_horizontal {2}:")
        data["req_info"]["req_id"] = "camera_image_flip flip_horizontal overmax 2"
        # error value.
        data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_horizontal"] = 2

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_rotation_normal(data: dict) -> None:
    assert data["res_info"]["code"] == 0
    assert data["res_info"]["res_id"] == "image_rotation"
    assert data["res_info"]["detail_msg"] == ""
    assert data["common_settings"]["pq_settings"]["image_rotation"] == 3


def change_rotation_error(data: dict, error_type: str) -> None:
    data["common_settings"]["process_state"] = 1
    if error_type == "lessmin":
        print(f"DTDL image_rotation {1}:")
        data["req_info"]["req_id"] = "image_rotation lessmin -1"
        # error value.
        data["common_settings"]["pq_settings"]["image_rotation"] = -1
    elif error_type == "overmax":
        print(f"DTDL image_rotation {2}:")
        data["req_info"]["req_id"] = "image_rotation overmax 4"
        # error value.
        data["common_settings"]["pq_settings"]["image_rotation"] = 4

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_flip_horizontal_error(data: dict, error_type: str) -> None:
    assert data["res_info"]["code"] == 3
    if error_type == "lessmin":
        assert data["res_info"]["res_id"] == "camera_image_flip flip_horizontal lessmin -1"
        assert data["res_info"]["detail_msg"] == "flip_horizontal not >= 0.000000"
    elif error_type == "overmax":
        assert data["res_info"]["res_id"] == "camera_image_flip flip_horizontal overmax 2"
        assert data["res_info"]["detail_msg"] == "flip_horizontal not <= 1.000000"


def change_flip_vertical_error(data: dict, error_type: str) -> None:
    data["common_settings"]["process_state"] = 1
    if error_type == "lessmin":
        print(f"DTDL camera_image_flip flip_vertical {1}:")
        data["req_info"]["req_id"] = "camera_image_flip flip_vertical lessmin -1"
        # error value.
        data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_vertical"] = -1
    elif error_type == "overmax":
        print(f"DTDL camera_image_flip flip_vertical {2}:")
        data["req_info"]["req_id"] = "camera_image_flip flip_vertical overmax 2"
        # error value.
        data["common_settings"]["pq_settings"]["camera_image_flip"]["flip_vertical"] = 2

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)


def validate_flip_vertical_error(data: dict, error_type: str) -> None:
    assert data["res_info"]["code"] == 3
    if error_type == "lessmin":
        assert data["res_info"]["res_id"] == "camera_image_flip flip_vertical lessmin -1"
        assert data["res_info"]["detail_msg"] == "flip_vertical not >= 0.000000"
    elif error_type == "overmax":
        assert data["res_info"]["res_id"] == "camera_image_flip flip_vertical overmax 2"
        assert data["res_info"]["detail_msg"] == "flip_vertical not <= 1.000000"

def validate_rotation_error(data: dict, error_type: str) -> None:
    assert data["res_info"]["code"] == 3
    if error_type == "lessmin":
        assert data["res_info"]["res_id"] == "image_rotation lessmin -1"
        assert data["res_info"]["detail_msg"] == "image_rotation not >= 0.000000"
    elif error_type == "overmax":
        assert data["res_info"]["res_id"] == "image_rotation overmax 4"
        assert data["res_info"]["detail_msg"] == "image_rotation not <= 3.000000"


def get_base_config(file_path) -> dict:
    with open(file_path, "r") as f:
        data = json.load(f)["edge_app"]
    return data


def check_image_cropping(checker: DTDLStateChecker, file_path: str) -> None:

    config = get_base_config(file_path)
    change_crop_normal(config)
    state_dict = checker.get_state()
    validate_crop_normal(state_dict)

    config = get_base_config(file_path)
    change_crop_left_error(config)
    state_dict = checker.get_state()
    validate_crop_left_error(state_dict)

    config = get_base_config(file_path)
    change_crop_top_error(config)
    state_dict = checker.get_state()
    validate_crop_top_error(state_dict)

    config = get_base_config(file_path)
    change_crop_width_error(config)
    state_dict = checker.get_state()
    validate_crop_width_error(state_dict)

    config = get_base_config(file_path)
    change_crop_height_error(config)
    state_dict = checker.get_state()
    validate_crop_height_error(state_dict)


def camera_image_flip(checker: DTDLStateChecker, file_path: str) -> None:

    config = get_base_config(file_path)
    change_flip_normal(config)
    state_dict = checker.get_state()
    validate_flip_normal(state_dict)

    config = get_base_config(file_path)
    change_flip_horizontal_error(config, "lessmin")
    state_dict = checker.get_state()
    validate_flip_horizontal_error(state_dict, "lessmin")

    config = get_base_config(file_path)
    change_flip_horizontal_error(config, "overmax")
    state_dict = checker.get_state()
    validate_flip_horizontal_error(state_dict, "overmax")

    config = get_base_config(file_path)
    change_flip_vertical_error(config, "lessmin")
    state_dict = checker.get_state()
    validate_flip_vertical_error(state_dict, "lessmin")

    config = get_base_config(file_path)
    change_flip_vertical_error(config, "overmax")
    state_dict = checker.get_state()
    validate_flip_vertical_error(state_dict, "overmax")


def image_rotation(checker: DTDLStateChecker, file_path: str) -> None:

    config = get_base_config(file_path)
    change_rotation_normal(config)
    state_dict = checker.get_state()
    validate_rotation_normal(state_dict)

    config = get_base_config(file_path)
    change_rotation_error(config, "lessmin")
    state_dict = checker.get_state()
    validate_rotation_error(state_dict, "lessmin")

    config = get_base_config(file_path)
    change_rotation_error(config, "overmax")
    state_dict = checker.get_state()
    validate_rotation_error(state_dict, "overmax")


def check_auto_exposure_metering(checker: DTDLStateChecker, file_path: str) -> None:

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_success(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_success(state_dict)

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_mode_full(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_mode_full(state_dict)

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_mode_overrange(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_mode_overrange(state_dict)

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_mode_underrange(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_mode_underrange(state_dict)

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_top_overrange(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_top_overrange(state_dict)

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_top_underrange(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_top_underrange(state_dict)

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_left_overrange(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_left_overrange(state_dict)

    config = get_base_config(file_path)
    ae_metering.change_auto_exposure_metering_left_underrange(config)
    state_dict = checker.get_state()
    ae_metering.validate_auto_exposure_metering_left_underrange(state_dict)

def change_gamma_mode_error(data: dict) -> None:
    data["common_settings"]["process_state"] = 1
    data["req_info"]["req_id"] = "gamma mode out of scope"
    # error value.
    data["common_settings"]["pq_settings"]["gamma_mode"] = 2

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)
