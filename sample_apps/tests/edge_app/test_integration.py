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
import time
import check_pq_settings
from utils import manage_edge_app
from utils import print_dtdl_state_log
from utils import print_valgrind_summary
from utils import send_data
from constants import APITEST_LAST_SCENARIO_ID, DTDL_LOG, INTEGRATION_TEST_INTERVAL_SECONDS, INTEGRATION_TEST_RETRY_NUM, INTEGRATION_TEST_LOG, APP_PATH, VALGRIND_LOG
from state_checker import DTDLStateChecker

class State:
    IDLE = 1
    RUNNING = 2

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

def change_passthrough_custom_settings(data: dict) -> None:
    pass

def validate_passthrough_custom_settings(data: dict) -> None:
    pass

def change_draw_custom_settings(data: dict) -> None:
    pass

def validate_draw_custom_settings(data: dict) -> None:
    pass

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

def change_classification_custom_settings(data: dict) -> None:
    data["req_info"]["req_id"] = "classification_custom_settings"
    data["custom_settings"]["ai_models"]["classification"]["parameters"]["max_predictions"] = 5
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_classification_custom_settings(data: dict) -> None:
    assert data["res_info"]["res_id"] == "classification_custom_settings"
    assert data["custom_settings"]["ai_models"]["classification"]["parameters"]["max_predictions"] == 5

def change_detection_custom_settings(data: dict) -> None:
    data["req_info"]["req_id"] = "detection_custom_settings"
    data["custom_settings"]["ai_models"]["detection"]["parameters"]["max_detections"] = 50
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_detection_custom_settings(data: dict) -> None:
    assert data["res_info"]["res_id"] == "detection_custom_settings"
    assert data["custom_settings"]["ai_models"]["detection"]["parameters"]["max_detections"] == 50

def change_segmentation_custom_settings(data: dict) -> None:
    data["req_info"]["req_id"] = "segmentation_custom_settings"
    data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_width"] = 4
    data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_height"] = 4
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_segmentation_custom_settings(data: dict) -> None:
    assert data["res_info"]["res_id"] == "segmentation_custom_settings"
    assert data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_width"] == 4
    assert data["custom_settings"]["ai_models"]["segmentation"]["parameters"]["input_height"] == 4

def change_apitest_custom_settings(data: dict) -> None:
    pass

def validate_apitest_custom_settings(data: dict) -> None:
    pass

def change_switch_dnn_custom_settings(data: dict) -> None:
    data["req_info"]["req_id"] = "switch_dnn_custom_settings"
    data["common_settings"]["number_of_inference_per_message"] = 2
    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)
    pass

def validate_switch_dnn_custom_settings(data: dict) -> None:
    assert data["res_info"]["res_id"] == "switch_dnn_custom_settings"
    assert data["common_settings"]["number_of_inference_per_message"] == 2
    pass

def change_process_state(data: dict, state: int, req_id: str = None) -> None:
    if req_id:
        data["req_info"]["req_id"] = req_id
    else:
        data["req_info"]["req_id"] = "change_process_state" + str(state)
    data["common_settings"]["process_state"] = state

    # Limitation: currently data_export only supports mode 0 or 2.
    data["common_settings"]["port_settings"]["input_tensor"]["method"] = 0
    data["common_settings"]["port_settings"]["metadata"]["method"] = 0

    send_data(data)
    time.sleep(INTEGRATION_TEST_INTERVAL_SECONDS)

def validate_process_state(data: dict, state: int) -> None:
    assert data["res_info"]["res_id"] == "change_process_state" + str(state)
    assert data["common_settings"]["process_state"] == state

def get_and_validate_process_state(checker: DTDLStateChecker, state: int, res_id: str) -> None:
    for i in range(INTEGRATION_TEST_RETRY_NUM):
        data = checker.get_state()
        if data["res_info"]["res_id"] == res_id and data["common_settings"]["process_state"] == state:
            # Pass
            break

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

    # Limitation: currently data_export only supports mode 0 or 2.
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
    "segmentation": change_segmentation_custom_settings,
    "apitest": change_apitest_custom_settings,
    "switch_dnn": change_switch_dnn_custom_settings,
}

VALIDATE_CUSTOM_SETTINGS_PER_APP = {
    "passthrough": validate_passthrough_custom_settings,
    "draw": validate_draw_custom_settings,
    "classification": validate_classification_custom_settings,
    "detection": validate_detection_custom_settings,
    "segmentation": validate_segmentation_custom_settings,
    "apitest": validate_apitest_custom_settings,
    "switch_dnn": validate_switch_dnn_custom_settings,
}

@pytest.fixture(scope="session")
def app(pytestconfig) -> str:
    app_name = pytestconfig.getoption("app")
    assert app_name in CUSTOM_SETTINGS_PER_APP
    return app_name

def test_valgrind(app: str):
    if os.path.exists(DTDL_LOG):
        os.remove(DTDL_LOG)
    if os.path.exists(INTEGRATION_TEST_LOG):
        os.remove(INTEGRATION_TEST_LOG)
    file_path = f"sample_apps/{app}/configuration/configuration.json"
    with manage_edge_app(app_path=APP_PATH, file_path=file_path) as (data, process):
        checker = DTDLStateChecker()

        # verify initial state
        state_dict = checker.get_state()
        assert state_dict["res_info"]["res_id"]

        if app != "apitest":
            change_pq_settings(data)
            state_dict = checker.get_state()
            validate_pq_settings(state_dict)

            change_number_iterations(data, 3)
            state_dict = checker.get_state()
            validate_number_iterations(state_dict, 3)

            CUSTOM_SETTINGS_PER_APP[app](data)
            if app != "passthrough" and app != "draw" and app != "apitest":
                state_dict = checker.get_state()
                VALIDATE_CUSTOM_SETTINGS_PER_APP[app](state_dict)

            change_process_state(data, State.IDLE)
            state_dict = checker.get_state()
            validate_process_state(state_dict, State.IDLE)

            req_id = "change_process_state_numofinf"
            change_process_state(data, State.RUNNING, req_id)
            # number_of_iterations is 3.
            # So after iterate 3 times completed, state changes from running to idle.
            get_and_validate_process_state(checker, State.IDLE, req_id)

            change_number_iterations(data, 0)
            state_dict = checker.get_state()
            validate_number_iterations(state_dict, 0)

            change_process_state(data, State.RUNNING)
            state_dict = checker.get_state()
            validate_process_state(state_dict, State.RUNNING)

            change_process_state(data, State.IDLE)
            state_dict = checker.get_state()
            validate_process_state(state_dict, State.IDLE)

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
    print_valgrind_summary(VALGRIND_LOG)

    # To check log file manually, not remove.
    # os.remove(DTDL_LOG)
    # os.remove(INTEGRATION_TEST_LOG)

    assert process.returncode == 0
