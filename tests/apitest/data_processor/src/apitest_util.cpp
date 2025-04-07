/****************************************************************************
 * Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#include "apitest_util.h"

#include "apitest_sensor.h"

static bool s_need_to_run_current_api_test_scenario = false;
static int32_t s_api_test_scenario_id = 0;

bool NeedToRunCurrentApiTestScenario() {
  return s_need_to_run_current_api_test_scenario;
}

int32_t CurrentApiTestScenarioId() { return s_api_test_scenario_id; }

void SetCurrentApiTestScenarioId(int32_t scenario_id) {
  if (s_api_test_scenario_id != scenario_id) {
    s_api_test_scenario_id = scenario_id;
    s_need_to_run_current_api_test_scenario = true;
  }
}

int32_t RunApiTest() {
  int32_t scenario_id = CurrentApiTestScenarioId();
  int32_t res = 0;
  switch (scenario_id) {
    case 1:
      res = RunApiTestScenarioSensorCore();
      s_need_to_run_current_api_test_scenario = false;
      break;
    case 2:
      res = RunApiTestScenarioSensorStream();
      s_need_to_run_current_api_test_scenario = false;
      break;
    case 3:
      res = RunApiTestScenarioSensorAct();
      s_need_to_run_current_api_test_scenario = false;
      break;
    case 4:
      res = RunApiTestScenarioFrame();
      s_need_to_run_current_api_test_scenario = false;
      break;
    case 5:
      res = RunApiTestScenarioProperty();
      s_need_to_run_current_api_test_scenario = false;
      break;
    case 6:
      res = RunApiTestScenarioChannel();
      s_need_to_run_current_api_test_scenario = false;
      break;
    case 7:
      res = RunApiTestScenarioError();
      s_need_to_run_current_api_test_scenario = false;
      break;
    default:
      break;
  }

  return res;
}
