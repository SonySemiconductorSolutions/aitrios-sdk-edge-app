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
import os
import pytest
import random
import string
import time
import check_pq_settings
from utils import manage_edge_app
from utils import manage_python_edge_app
from utils import print_dtdl_state_log
from utils import print_valgrind_summary
from utils import send_data
from constants import APITEST_LAST_SCENARIO_ID, DTDL_LOG, INTEGRATION_TEST_INTERVAL_SECONDS, INTEGRATION_TEST_RETRY_NUM, INTEGRATION_TEST_LOG, APP_PATH, PYTHON_APP_PATH, VALGRIND_LOG
from state_checker import DTDLStateChecker

# buffer size depends on evp specification
EVP_MQTT_SEND_BUFF_SIZE = 131072

class State:
    IDLE = 1
    RUNNING = 2

class Method:
    MQTT = 0
    BLOB = 1
    HTTP = 2

class Codec:
    RAW = 0
    JPG = 1
    BMP = 2

class MetadataFormat:
    BASE64 = 0
    JSON = 1

def change_pq_settings(data: dict) -> None:
    data["req_info"]["req_id"] = "change_pq_settings"
    data["common_settings"]["pq_settings"]["digital_zoom"] = 0.125
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_pq_settings(data: dict) -> None:
    assert data["res_info"]["res_id"] == "change_pq_settings"
    assert data["common_settings"]["pq_settings"]["digital_zoom"] == 0.125

def change_number_iterations(data: dict, numofinf: int) -> None:
    data["req_info"]["req_id"] = f"change_number_iterations{numofinf}"
    data["common_settings"]["inference_settings"]["number_of_iterations"] = numofinf
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_number_iterations(data: dict, numofinf: int) -> None:
    assert data["res_info"]["res_id"] == f"change_number_iterations{numofinf}"
    assert data["common_settings"]["inference_settings"]["number_of_iterations"] == numofinf

def change_passthrough_custom_settings(data: dict, id_suffix: str) -> None:
    data["req_info"]["req_id"] = f"passthrough_custom_settings{id_suffix}"
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_passthrough_custom_settings(data: dict, id_suffix: str) -> None:
    assert data["res_info"]["res_id"] == f"passthrough_custom_settings{id_suffix}"
    assert data["custom_settings"]["res_info"]["res_id"] == f"passthrough_custom_settings{id_suffix}"

def change_draw_custom_settings(data: dict, id_suffix: str) -> None:
    data["req_info"]["req_id"] = f"draw_custom_settings{id_suffix}"
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_draw_custom_settings(data: dict, id_suffix: str) -> None:
    assert data["res_info"]["res_id"] == f"draw_custom_settings{id_suffix}"
    assert data["custom_settings"]["res_info"]["res_id"] == f"draw_custom_settings{id_suffix}"

def change_pq_settings_error(data: dict, app: str) -> None:
    data["req_info"]["req_id"] = "change_pq_settings_error"
    data["common_settings"]["pq_settings"]["frame_rate"]["denom"] = 0  # error value.
    if app == "detection":
        data["custom_settings"]["ai_models"][app]["parameters"]["max_detections"] = 9
    elif app == "classification":
        data["custom_settings"]["ai_models"][app]["parameters"]["max_predictions"] = 4
    elif app == "segmentation":
        data["custom_settings"]["ai_models"][app]["parameters"]["input_width"] = 100
    send_data(data)

def validate_pq_settings_error(data: dict, app: str) -> None:
    assert data["res_info"]["res_id"] == "change_pq_settings_error"
    assert data["res_info"]["code"] == 3
    assert data["common_settings"]["pq_settings"]["frame_rate"]["denom"] == 100
    if app == "detection":
        assert data["custom_settings"]["ai_models"][app]["parameters"]["max_detections"] == 50
    elif app == "classification":
        assert data["custom_settings"]["ai_models"][app]["parameters"]["max_predictions"] == 5
    elif app == "segmentation":
        assert data["custom_settings"]["ai_models"][app]["parameters"]["input_width"] == 4

def change_classification_custom_settings(data: dict, id_suffix: str) -> None:
    data["req_info"]["req_id"] = f"classification_custom_settings{id_suffix}"
    data["custom_settings"]["ai_models"]["classification"]["parameters"]["max_predictions"] = 5
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_classification_custom_settings(data: dict, id_suffix: str) -> None:
    assert data["res_info"]["res_id"] == f"classification_custom_settings{id_suffix}"
    assert data["custom_settings"]["res_info"]["res_id"] == f"classification_custom_settings{id_suffix}"
    assert data["custom_settings"]["ai_models"]["classification"]["parameters"]["max_predictions"] == 5

def change_detection_custom_settings(data: dict, id_suffix: str) -> None:
    data["req_info"]["req_id"] = f"detection_custom_settings{id_suffix}"
    data["custom_settings"]["ai_models"]["detection"]["parameters"]["max_detections"] = 50
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_detection_custom_settings(data: dict, id_suffix: str) -> None:
    assert data["res_info"]["res_id"] == f"detection_custom_settings{id_suffix}"
    assert data["custom_settings"]["res_info"]["res_id"] == f"detection_custom_settings{id_suffix}"
    assert data["custom_settings"]["ai_models"]["detection"]["parameters"]["max_detections"] == 50

def change_posenet_custom_settings(data: dict, id_suffix: str) -> None:
    data["req_info"]["req_id"] = f"posenet_custom_settings{id_suffix}"
    data["custom_settings"]["ai_models"]["posenet"]["parameters"]["max_pose_detections"] = 50
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_posenet_custom_settings(data: dict, id_suffix: str) -> None:
    assert data["res_info"]["res_id"] == f"posenet_custom_settings{id_suffix}"
    assert data["custom_settings"]["ai_models"]["posenet"]["parameters"]["max_pose_detections"] == 50

def change_segmentation_custom_settings(data: dict, id_suffix: str) -> None:
    data["req_info"]["req_id"] = f"segmentation_custom_settings{id_suffix}"
    data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_width"] = 4
    data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_height"] = 4
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_segmentation_custom_settings(data: dict, id_suffix: str) -> None:
    assert data["res_info"]["res_id"] == f"segmentation_custom_settings{id_suffix}"
    assert data["custom_settings"]["res_info"]["res_id"] == f"segmentation_custom_settings{id_suffix}"
    assert data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_width"] == 4
    assert data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_height"] == 4

def change_apitest_custom_settings(data: dict, id_suffix: str) -> None:
    pass

def validate_apitest_custom_settings(data: dict, id_suffix: str) -> None:
    pass

def change_switch_dnn_custom_settings(data: dict, id_suffix: str) -> None:
    data["req_info"]["req_id"] = f"switch_dnn_custom_settings{id_suffix}"
    data["common_settings"]["number_of_inference_per_message"] = 2
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_switch_dnn_custom_settings(data: dict, id_suffix: str) -> None:
    assert data["res_info"]["res_id"] == f"switch_dnn_custom_settings{id_suffix}"
    assert data["custom_settings"]["res_info"]["res_id"] == f"switch_dnn_custom_settings{id_suffix}"
    assert data["common_settings"]["number_of_inference_per_message"] == 2

def change_custom_settings_metadata_format(data: dict, metadata_format: int) -> None:
    data["req_info"]["req_id"] = f"custom_settings_metadata_format{metadata_format}"
    data["custom_settings"]["metadata_settings"]["format"] = metadata_format
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_custom_settings_metadata_format(data: dict, metadata_format: int) -> None:
    assert data["res_info"]["res_id"] == f"custom_settings_metadata_format{metadata_format}"
    assert data["custom_settings"]["res_info"]["res_id"] == f"custom_settings_metadata_format{metadata_format}"
    assert data["custom_settings"]["metadata_settings"]["format"] == metadata_format

def change_process_state(data: dict, state: int, method: int, req_id: str = None) -> None:
    if req_id:
        data["req_info"]["req_id"] = req_id
    else:
        data["req_info"]["req_id"] = "change_process_state" + str(state)
    data["common_settings"]["process_state"] = state

    data["common_settings"]["port_settings"]["input_tensor"]["method"] = method
    data["common_settings"]["port_settings"]["metadata"]["method"] = method

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def change_codec_settings(data: dict, format: int) -> None:
    data["req_info"]["req_id"] = f"change_codec_settings{format}"
    data["common_settings"]["codec_settings"]["format"] = format
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_codec_settings(data: dict, format: int) -> None:
    assert data["res_info"]["res_id"] == f"change_codec_settings{format}"
    assert data["common_settings"]["codec_settings"]["format"] == format

def get_and_validate_process_state(checker: DTDLStateChecker, state: int, method: int, res_id: str = None) -> dict:
    if res_id == None:
        res_id = "change_process_state" + str(state)
    for i in range(INTEGRATION_TEST_RETRY_NUM):
        data = checker.get_state()
        if data["res_info"]["res_id"] == res_id and data["common_settings"]["process_state"] == state:
            assert data["common_settings"]["port_settings"]["input_tensor"]["method"] == method
            assert data["common_settings"]["port_settings"]["metadata"]["method"] == method
            # Pass
            return data

        if i == INTEGRATION_TEST_RETRY_NUM - 1:
            # Failed. So raise assertion.
            assert data["res_info"]["res_id"] == res_id
            assert data["common_settings"]["process_state"] == state

        time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_code(data: dict, code: int) -> None:
    assert data["res_info"]["code"] == code

def run_apitest_scenario(data: dict, scenario_id: int) -> None:
    data["req_info"]["req_id"] = "apitest_" + str(scenario_id)
    data["common_settings"]["process_state"] = 1

    data["common_settings"]["port_settings"]["input_tensor"]["method"] = 0
    data["common_settings"]["port_settings"]["metadata"]["method"] = 0

    data["custom_settings"]["apitest"]["scenario_id"] = scenario_id
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_apitest_scenario(data: dict, scenario_id: int) -> None:
    assert data["res_info"]["res_id"] == "apitest_" + str(scenario_id)
    assert data["common_settings"]["process_state"] == 1
    assert data["custom_settings"]["res_info"]["res_id"] == ""
    assert data["custom_settings"]["res_info"]["code"] == 0
    assert data["custom_settings"]["res_info"]["detail_msg"] == "apitest, " + str(scenario_id) + ", result, 0"

CUSTOM_SETTINGS_PER_APP = {
    "passthrough": change_passthrough_custom_settings,
    "draw": change_draw_custom_settings,
    "classification": change_classification_custom_settings,
    "detection": change_detection_custom_settings,
    "posenet": change_posenet_custom_settings,
    "segmentation": change_segmentation_custom_settings,
    "apitest": change_apitest_custom_settings,
    "switch_dnn": change_switch_dnn_custom_settings,
}

VALIDATE_CUSTOM_SETTINGS_PER_APP = {
    "passthrough": validate_passthrough_custom_settings,
    "draw": validate_draw_custom_settings,
    "classification": validate_classification_custom_settings,
    "detection": validate_detection_custom_settings,
    "posenet": validate_posenet_custom_settings,
    "segmentation": validate_segmentation_custom_settings,
    "apitest": validate_apitest_custom_settings,
    "switch_dnn": validate_switch_dnn_custom_settings,
}

def generate_random_string(length):
    letters = string.ascii_letters
    random_string = ''.join(random.choice(letters) for _ in range(length))
    return random_string

def do_test_configuration_json_max_size(checker: DTDLStateChecker, app: str, data: dict):

    # preserve original value
    original_value = data["common_settings"]["port_settings"]["metadata"]["path"]

    data["req_info"]["req_id"] = "configuration_max_size"
    data["common_settings"]["port_settings"]["metadata"]["path"] = ""

    json_str = json.dumps(data)
    json_str_len = len(json_str)

    max_len = EVP_MQTT_SEND_BUFF_SIZE - 1

    if max_len > json_str_len:
        # generate and set dummy value for being configuration json size max.
        dummy_value = generate_random_string(max_len - json_str_len)
        data["common_settings"]["port_settings"]["metadata"]["path"] = dummy_value

        json_str = json.dumps(data)
        json_str_len = len(json_str.encode())
        assert json_str_len == max_len
        print(f"[do_test_configuration_json_max_size] json_str_len {json_str_len}")

        send_data(data)
        time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

        state_dict = checker.get_state()

        assert state_dict["res_info"]["res_id"] == "configuration_max_size"
        assert state_dict["res_info"]["code"] == 0
        assert state_dict["res_info"]["detail_msg"] == ""
        assert state_dict["common_settings"]["port_settings"]["metadata"]["path"] == dummy_value

    # restore original value.
    data["common_settings"]["port_settings"]["metadata"]["path"] = original_value

@pytest.fixture(scope="session")
def app(pytestconfig) -> str:
    app_name = pytestconfig.getoption("app")
    assert app_name in CUSTOM_SETTINGS_PER_APP
    return app_name

@pytest.fixture(scope="session")
def python_bindings(pytestconfig) -> bool:
    return pytestconfig.getoption("python_bindings")

def test_valgrind(app: str, python_bindings: bool):
    if os.path.exists(DTDL_LOG):
        os.remove(DTDL_LOG)
    if os.path.exists(INTEGRATION_TEST_LOG):
        os.remove(INTEGRATION_TEST_LOG)
    if app != "apitest":
        file_path = f"sample_apps/{app}/configuration/configuration.json"
    else:
        file_path = f"tests/{app}/configuration/configuration.json"

    manage = manage_python_edge_app if python_bindings else manage_edge_app
    app_path = PYTHON_APP_PATH if python_bindings else APP_PATH
    with manage(app_path=app_path, file_path=file_path) as (data, process):
        checker = DTDLStateChecker()

        # verify initial state
        state_dict = checker.get_state()
        assert state_dict["res_info"]["res_id"]

        if app != "apitest":
            change_pq_settings(data)
            state_dict = checker.get_state()
            validate_pq_settings(state_dict)

            change_number_iterations(data, 2)
            state_dict = checker.get_state()
            validate_number_iterations(state_dict, 2)

            codec = Codec.RAW
            change_codec_settings(data, codec)
            state_dict = checker.get_state()
            validate_codec_settings(state_dict, codec)

            codec = Codec.JPG
            change_codec_settings(data, codec)
            state_dict = checker.get_state()
            validate_codec_settings(state_dict, codec)

            CUSTOM_SETTINGS_PER_APP[app](data, "1")
            state_dict = checker.get_state()
            VALIDATE_CUSTOM_SETTINGS_PER_APP[app](state_dict, "1")

            # check the same custom_settings also processed
            CUSTOM_SETTINGS_PER_APP[app](data, "2")
            state_dict = checker.get_state()
            VALIDATE_CUSTOM_SETTINGS_PER_APP[app](state_dict, "2")

            # check configuration json max size
            do_test_configuration_json_max_size(checker, app, data)

            state = State.IDLE
            method = Method.MQTT
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            state = State.RUNNING
            method = Method.MQTT
            req_id = "change_process_state_numofinf"
            change_process_state(data, state, method, req_id)
            # number_of_iterations is 2.
            # So after iterate 2 times completed, state changes from running to idle.
            state_dict = get_and_validate_process_state(checker, State.IDLE, method, req_id)
            data["common_settings"]["process_state"] = 1

            change_number_iterations(data, 0)
            state_dict = checker.get_state()
            validate_number_iterations(state_dict, 0)

            # inference with port_settings method MQTT
            state = State.RUNNING
            method = Method.MQTT
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            state = State.IDLE
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            # inference with port_settings method BLOB
            state = State.RUNNING
            method = Method.BLOB
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            state = State.IDLE
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            # inference with port_settings method HTTP
            state = State.RUNNING
            method = Method.HTTP
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            state = State.IDLE
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            # inference with codec_settings format RAW
            codec = Codec.RAW
            change_codec_settings(data, codec)
            state_dict = checker.get_state()
            validate_codec_settings(state_dict, codec)

            state = State.RUNNING
            method = Method.HTTP
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            state = State.IDLE
            change_process_state(data, state, method)
            state_dict = get_and_validate_process_state(checker, state, method)

            codec = Codec.JPG
            change_codec_settings(data, codec)
            state_dict = checker.get_state()
            validate_codec_settings(state_dict, codec)

            # inference with metadata format JSON
            if app == "detection" or app == "classification":
                metadata_format = MetadataFormat.JSON
                change_custom_settings_metadata_format(data, metadata_format)
                state_dict = checker.get_state()
                validate_custom_settings_metadata_format(state_dict, metadata_format)

                state = State.RUNNING
                method = Method.HTTP
                change_process_state(data, state, method)
                state_dict = get_and_validate_process_state(checker, state, method)

                state = State.IDLE
                change_process_state(data, state, method)
                state_dict = get_and_validate_process_state(checker, state, method)

                metadata_format = MetadataFormat.BASE64
                change_custom_settings_metadata_format(data, metadata_format)
                state_dict = checker.get_state()
                validate_custom_settings_metadata_format(state_dict, metadata_format)

            # prepare
            change_pq_settings(data)
            state_dict = checker.get_state()
            validate_pq_settings(state_dict)

            # This data should be ignored because the req_id is the same as the previous one.
            change_pq_settings(data)

            # This checks error behavior when SensorSetProperty returns error.
            change_pq_settings_error(data, app)
            state_dict = checker.get_state()
            validate_pq_settings_error(state_dict, app)
        else:
            for scenario_id in range(1, APITEST_LAST_SCENARIO_ID + 1):
                run_apitest_scenario(data, scenario_id)
                state_dict = checker.get_state()
                validate_apitest_scenario(state_dict, scenario_id)

            check_pq_settings.check_image_cropping(checker, file_path)

            check_pq_settings.camera_image_flip(checker, file_path)

            check_pq_settings.image_rotation(checker, file_path)

            check_pq_settings.check_auto_exposure_metering(checker, file_path)

    print_dtdl_state_log(DTDL_LOG)
    if not python_bindings:  # we are not currently running with valgrind when testing Python bindings
        print_valgrind_summary(VALGRIND_LOG)

    # To check log file manually, not remove.
    # os.remove(DTDL_LOG)
    # os.remove(INTEGRATION_TEST_LOG)

    assert process.returncode == 0
