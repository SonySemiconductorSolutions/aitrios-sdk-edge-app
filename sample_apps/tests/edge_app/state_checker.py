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
import jsondiff
from constants import DTDL_LOG
from constants import TIME_LIMIT_SEC


class DTDLStateChecker:

    prev = {}
    last_line_count = 0

    def __init__(self):
        last_line_dict, self.last_line_count = self.get_DTDL_LOG_last_update_info()
        self.prev = last_line_dict
        print(f"DTDL {self.last_line_count - 1}:")
        print(f"{json.dumps(last_line_dict)}\n")
        assert not last_line_dict["res_info"]["res_id"]

    def get_state(self) -> dict:
        # File update check with time limit loop
        self.wait_time_for_response(self.last_line_count)
        last_line_dict, self.last_line_count = self.get_DTDL_LOG_last_update_info()
        print(f"DTDL {self.last_line_count - 1}:")
        act = last_line_dict
        print(f"{jsondiff.diff(self.prev, act)}\n")
        self.prev = act
        return act

    def get_DTDL_LOG_last_update_info(self):
        # Get and return the last string and number of rows from DTDL_LOG
        with open(DTDL_LOG)as f:
            file_read = f.readlines()
            self.last_line = file_read[-1]
        last_line_dict = json.loads(self.last_line)
        last_line_count = len(file_read)
        return last_line_dict, last_line_count

    def wait_time_for_response(self, last_line_count: int):
        start = time.time()
        elapsed_time = 0.0
        while elapsed_time < TIME_LIMIT_SEC:
            elapsed_time = time.time() - start
            if last_line_count != len(open(DTDL_LOG).readlines()):
                break
            time.sleep(0.2)
        # time limit check
        assert elapsed_time < TIME_LIMIT_SEC
