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

APP_PATH = "./tests/edge_app/build/build/edge_app"
PYTHON_APP_PATH = "./libs/py/example/simple_edge_app.py"
VALGRIND_LOG = "./valgrind.log"
DTDL_LOG = "./state.logs"
INTEGRATION_TEST_LOG = "./integration_test.log"
INTEGRATION_TEST_INTERVAL_SECONDS = 0.5
INTEGRATION_TEST_RETRY_NUM = 20
APITEST_LAST_SCENARIO_ID = 7
ACK_FILE = "./data.ack"
INJECT_FAIL = "./sleep_time"
TIME_LIMIT_SEC = 60.0 # 2.0 # TODO: for some reason Python bindings integration test requires more time for consistent test runs # increased more for running on github actions
