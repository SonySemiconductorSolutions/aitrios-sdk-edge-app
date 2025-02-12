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

#include "apitest_sensor.h"

#include <stdlib.h>

#include "log.h"
#include "sensor.h"

#if MOCK_APITEST
#include "sensor/mock_sensor.hpp"
#endif  // MOCK_APITEST

using namespace EdgeAppLib;

// Error code XXYYZZAA
// XX Method type 　
#define CORE_INIT 1000000
#define CORE_EXIT 2000000
#define CORE_OPEN_STREAM 3000000
#define CORE_CLOSE_STREAM 4000000
#define START 5000000
#define STOP 6000000
#define GET_FRAME 7000000
#define RELEASE_FRAME 8000000
#define STREAM_GET_PROPERTY 9000000
#define STREAM_SET_PROPERTY 10000000
#define FRAME_GET_CHANNEL_FROM_CHANNEL_ID 11000000
#define CHANNEL_GET_RAW_DATA 12000000
#define CHANNEL_GET_PROPERTY 13000000
#define GET_LAST_ERROR_STRING 16000000
#define INPUT_DATA_TYPE_ENABLE_CHANNEL 17000000
#define GET_LAST_ERROR_LEVEL 18000000
#define GET_LAST_ERROR_CAUSE 19000000
// YY Each parameter and return value
#define PARAM_01 10000
#define PARAM_02 20000
#define PARAM_03 30000
#define PARAM_04 40000
#define PARAM_ALL 100000
#define RETUN 900000
// ZZ Normal, abnormal , boundary value
#define RETUN_NRM 1000
#define BOUNDARY_MIN 2000
#define BOUNDARY_MAX 3000
#define BOUNDARY_OVER_MIN 5000
#define BOUNDARY_OVER_MAX 6000
#define RETUN_ERR 9000
// AA Execution results

#define CLEANUP_COREEXIT SensorCoreExit(core);
#define CLEANUP_CORECLOSE              \
  SensorCoreCloseStream(core, stream); \
  SensorCoreExit(core);
#define CLEANUP_STOP                   \
  SensorStop(stream);                  \
  SensorCoreCloseStream(core, stream); \
  SensorCoreExit(core);
#define CLEANUP_RELEASEFRAME           \
  SensorReleaseFrame(stream, frame);   \
  SensorStop(stream);                  \
  SensorCoreCloseStream(core, stream); \
  SensorCoreExit(core);

int32_t RunApiTestScenarioSensorCore() {
  // API SensorCoreInit NULL, ret:-1
  int32_t res_sensor = SensorCoreInit(NULL);
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    return -CORE_INIT - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreInit core, ret:0
  EdgeAppLibSensorCore core = 0;
  res_sensor = SensorCoreInit(&core);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    return -CORE_INIT - PARAM_01 - RETUN_NRM + res_sensor;
  }

  // API SensorCoreExit 0, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorCoreExitFail();
#endif  // MOCK_APITEST
  res_sensor = SensorCoreExit(0);
#if MOCK_APITEST
  resetEdgeAppLibSensorCoreExitSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    return -CORE_EXIT - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreExit core, ret:0
  res_sensor = SensorCoreExit(core);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    return -CORE_EXIT - PARAM_01 - RETUN_NRM + res_sensor;
  }

  return 0;
}

int32_t RunApiTestScenarioSensorStream() {
  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream;
  int32_t res_sensor = 0;
  SensorCoreInit(&core);

  // API SensorCoreOpenStream 0, stream_key, stream ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorCoreOpenStreamFail();
#endif  // MOCK_APITEST
  res_sensor =
      SensorCoreOpenStream(0, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
#if MOCK_APITEST
  resetEdgeAppLibSensorCoreOpenStreamSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -CORE_OPEN_STREAM - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreOpenStream core, not_stream_key, stream ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorCoreOpenStreamFail();
#endif  // MOCK_APITEST
  res_sensor = SensorCoreOpenStream(core, "xyz", &stream);
#if MOCK_APITEST
  resetEdgeAppLibSensorCoreOpenStreamSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -CORE_OPEN_STREAM - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreOpenStream core, NULL, stream ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorCoreOpenStreamFail();
#endif  // MOCK_APITEST
  res_sensor = SensorCoreOpenStream(core, NULL, &stream);
#if MOCK_APITEST
  resetEdgeAppLibSensorCoreOpenStreamSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -CORE_OPEN_STREAM - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreOpenStream core, stream_key, NULL ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorCoreOpenStreamFail();
#endif  // MOCK_APITEST
  res_sensor =
      SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, NULL);
#if MOCK_APITEST
  resetEdgeAppLibSensorCoreOpenStreamSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -CORE_OPEN_STREAM - PARAM_03 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreOpenStream core, stream_key, stream, ret:0
  res_sensor =
      SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_COREEXIT
    return -CORE_OPEN_STREAM - PARAM_ALL - RETUN_NRM + res_sensor;
  }

  // API SensorCoreCloseStream 0, stream, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorCoreCloseStreamFail();
#endif  // MOCK_APITEST
  res_sensor = SensorCoreCloseStream(0, stream);
#if MOCK_APITEST
  resetEdgeAppLibSensorCoreCloseStreamSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -CORE_CLOSE_STREAM - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreCloseStream core, 0, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorCoreCloseStreamFail();
#endif  // MOCK_APITEST
  res_sensor = SensorCoreCloseStream(core, 0);
#if MOCK_APITEST
  resetEdgeAppLibSensorCoreCloseStreamSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -CORE_CLOSE_STREAM - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorCoreCloseStream core, stream, ret:0
  res_sensor = SensorCoreCloseStream(core, stream);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_COREEXIT
    return -CORE_CLOSE_STREAM - PARAM_ALL - RETUN_NRM + res_sensor;
  }

  CLEANUP_COREEXIT
  return 0;
}

int32_t RunApiTestScenarioSensorAct() {
  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream;
  EdgeAppLibSensorFrame frame;
  int32_t res_sensor;
  SensorCoreInit(&core);
  SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);

  // API SensorStart 0, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStartFail();
#endif  // MOCK_APITEST
  res_sensor = SensorStart(0);
#if MOCK_APITEST
  resetEdgeAppLibSensorStartSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -START - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorStart stream, ret:0
  res_sensor = SensorStart(stream);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -START - PARAM_01 - RETUN_NRM + res_sensor;
  }

  // API SensorStop 0, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStopFail();
#endif  // MOCK_APITEST
  res_sensor = SensorStop(0);
#if MOCK_APITEST
  resetEdgeAppLibSensorStopSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -STOP - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorStop stream, ret:0
  res_sensor = SensorStop(stream);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_CORECLOSE
    return -STOP - PARAM_01 - RETUN_NRM + res_sensor;
  }

  CLEANUP_CORECLOSE
  return 0;
}

int32_t RunApiTestScenarioFrame() {
  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream;
  EdgeAppLibSensorFrame frame;
  int32_t res_sensor;
  SensorCoreInit(&core);
  SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
  SensorStart(stream);

  // API SensorGetFrame 0, frame, infinitely, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorGetFrameFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetFrame(0, &frame, -1);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetFrameSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -GET_FRAME - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorGetFrame stream, NULL, infinitely, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorGetFrameFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetFrame(stream, NULL, -1);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetFrameSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -GET_FRAME - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorGetFrame stream, frame, 100, ret:0
  res_sensor = SensorGetFrame(stream, &frame, 100);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_STOP
    return -GET_FRAME - PARAM_03 - RETUN_NRM + res_sensor;
  }
  SensorReleaseFrame(stream, frame);

  // API SensorGetFrame stream, frame, 0, ret:0
  res_sensor = SensorGetFrame(stream, &frame, 0);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_STOP
    return -GET_FRAME - PARAM_03 - RETUN_NRM + res_sensor;
  }
  SensorReleaseFrame(stream, frame);

  // API SensorGetFrame stream, frame, infinitely, ret:0
  res_sensor = SensorGetFrame(stream, &frame, -1);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_STOP
    return -GET_FRAME - PARAM_03 - BOUNDARY_MIN + res_sensor;
  }
  SensorReleaseFrame(stream, frame);

  EdgeAppLibSensorChannel channel = 0;
  SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);

  // API SensorReleaseFrame 0, frame, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorReleaseFrameFail();
#endif  // MOCK_APITEST
  res_sensor = SensorReleaseFrame(0, frame);
#if MOCK_APITEST
  resetEdgeAppLibSensorReleaseFrameSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_STOP
    return -RELEASE_FRAME - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorReleaseFrame stream, 0, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorReleaseFrameFail();
#endif  // MOCK_APITEST
  res_sensor = SensorReleaseFrame(stream, 0);
#if MOCK_APITEST
  resetEdgeAppLibSensorReleaseFrameSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_STOP
    return -RELEASE_FRAME - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorReleaseFrame stream, frame, ret:0
  res_sensor = SensorReleaseFrame(stream, frame);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_STOP
    return -RELEASE_FRAME - PARAM_ALL - RETUN_NRM + res_sensor;
  }

  CLEANUP_STOP
}

int32_t RunApiTestScenarioProperty() {
  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream;
  EdgeAppLibSensorFrame frame;
  int32_t res_sensor;
  SensorCoreInit(&core);
  SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
  SensorStart(stream);
  SensorGetFrame(stream, &frame, -1);

  EdgeAppLibSensorChannel channel = 0;
  res_sensor = SensorFrameGetChannelFromChannelId(
      frame, AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);

  const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
  EdgeAppLibSensorAiModelBundleIdProperty property2;

  // API SensorStreamGetProperty 0, key, value, value_size, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStreamGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorStreamGetProperty(0, key, &property2, sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorStreamGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -STREAM_GET_PROPERTY - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorStreamGetProperty stream, no_key, value, value_size, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStreamGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor =
      SensorStreamGetProperty(stream, "no_key", &property2, sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorStreamGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -STREAM_GET_PROPERTY - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorStreamGetProperty stream, NULL, value, value_size, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStreamGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor =
      SensorStreamGetProperty(stream, NULL, &property2, sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorStreamGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -STREAM_GET_PROPERTY - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorStreamGetProperty stream, key, NULL, value_size, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStreamGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorStreamGetProperty(stream, key, NULL, sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorStreamGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -STREAM_GET_PROPERTY - PARAM_03 - RETUN_ERR + res_sensor;
  }

  // API SensorStreamGetProperty stream, key, value, not_value_size, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStreamGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorStreamGetProperty(stream, key, &property2, 1);
#if MOCK_APITEST
  resetEdgeAppLibSensorStreamGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -STREAM_GET_PROPERTY - PARAM_04 - RETUN_ERR + res_sensor;
  }

  // API SensorStreamGetProperty stream, key, value, value_size, ret:0
  res_sensor =
      SensorStreamGetProperty(stream, key, &property2, sizeof(property2));
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -STREAM_GET_PROPERTY - PARAM_ALL - RETUN_NRM + res_sensor;
  }

  uint32_t ai_model_bundle_ids[] = {0x900100, 0x000100, 0x0, 0xffffff};
  // API SensorStreamSetProperty 0, key, value, value_size, ret:-1
  for (int i = 0;
       i < sizeof(ai_model_bundle_ids) / sizeof(ai_model_bundle_ids[0]); i++) {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property = {};
    property.ai_model_bundle_id[0] = ai_model_bundle_ids[i];
#if MOCK_APITEST
    setEdgeAppLibSensorStreamSetPropertyFail();
#endif  // MOCK_APITEST
    res_sensor = SensorStreamSetProperty(0, key, &property, sizeof(property));
#if MOCK_APITEST
    resetEdgeAppLibSensorStreamSetPropertySuccess();
#endif  // MOCK_APITEST
    if (res_sensor != -1) {
      LOG_WARN("ApiTest failed %d\n", res_sensor);
      CLEANUP_RELEASEFRAME
      return -STREAM_SET_PROPERTY - PARAM_01 - RETUN_ERR + res_sensor;
    }
  }

  // API SensorStreamSetProperty stream, no_key, value, value_size, ret:-1
  for (int i = 0;
       i < sizeof(ai_model_bundle_ids) / sizeof(ai_model_bundle_ids[0]); i++) {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property = {};
    property.ai_model_bundle_id[0] = ai_model_bundle_ids[i];
#if MOCK_APITEST
    setEdgeAppLibSensorStreamSetPropertyFail();
#endif  // MOCK_APITEST
    res_sensor =
        SensorStreamSetProperty(stream, "no_key", &property, sizeof(property));
#if MOCK_APITEST
    resetEdgeAppLibSensorStreamSetPropertySuccess();
#endif  // MOCK_APITEST
    if (res_sensor != -1) {
      LOG_WARN("ApiTest failed %d\n", res_sensor);
      CLEANUP_RELEASEFRAME
      return -STREAM_SET_PROPERTY - PARAM_02 - RETUN_ERR + res_sensor;
    }
  }

  // API SensorStreamSetProperty stream, NULL, value, value_size, ret:-1
  for (int i = 0;
       i < sizeof(ai_model_bundle_ids) / sizeof(ai_model_bundle_ids[0]); i++) {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property = {};
    property.ai_model_bundle_id[0] = ai_model_bundle_ids[i];
#if MOCK_APITEST
    setEdgeAppLibSensorStreamSetPropertyFail();
#endif  // MOCK_APITEST
    res_sensor =
        SensorStreamSetProperty(stream, NULL, &property, sizeof(property));
#if MOCK_APITEST
    resetEdgeAppLibSensorStreamSetPropertySuccess();
#endif  // MOCK_APITEST
    if (res_sensor != -1) {
      LOG_WARN("ApiTest failed %d\n", res_sensor);
      CLEANUP_RELEASEFRAME
      return -STREAM_SET_PROPERTY - PARAM_02 - RETUN_ERR + res_sensor;
    }
  }

  // API SensorStreamSetProperty stream, key, NULL, value_size, ret:-1
  for (int i = 0;
       i < sizeof(ai_model_bundle_ids) / sizeof(ai_model_bundle_ids[0]); i++) {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property = {};
    property.ai_model_bundle_id[0] = ai_model_bundle_ids[i];
#if MOCK_APITEST
    setEdgeAppLibSensorStreamSetPropertyFail();
#endif  // MOCK_APITEST
    res_sensor = SensorStreamSetProperty(stream, key, NULL, sizeof(property));
#if MOCK_APITEST
    resetEdgeAppLibSensorStreamSetPropertySuccess();
#endif  // MOCK_APITEST
    if (res_sensor != -1) {
      LOG_WARN("ApiTest failed %d\n", res_sensor);
      CLEANUP_RELEASEFRAME
      return -STREAM_SET_PROPERTY - PARAM_03 - RETUN_ERR + res_sensor;
    }
  }

  // API SensorStreamSetProperty stream, key, value, not_value_size, ret:-1
  for (int i = 0;
       i < sizeof(ai_model_bundle_ids) / sizeof(ai_model_bundle_ids[0]); i++) {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property = {};
    property.ai_model_bundle_id[0] = ai_model_bundle_ids[i];
#if MOCK_APITEST
    setEdgeAppLibSensorStreamSetPropertyFail();
#endif  // MOCK_APITEST
    res_sensor = SensorStreamSetProperty(stream, key, &property, 1);
#if MOCK_APITEST
    resetEdgeAppLibSensorStreamSetPropertySuccess();
#endif  // MOCK_APITEST
    if (res_sensor != -1) {
      LOG_WARN("ApiTest failed %d\n", res_sensor);
      CLEANUP_RELEASEFRAME
      return -STREAM_SET_PROPERTY - PARAM_04 - RETUN_ERR + res_sensor;
    }
  }

  // API SensorStreamSetProperty stream, key, value, value_size, ret:0
  for (int i = 0;
       i < sizeof(ai_model_bundle_ids) / sizeof(ai_model_bundle_ids[0]); i++) {
    const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
    EdgeAppLibSensorAiModelBundleIdProperty property = {};
    property.ai_model_bundle_id[0] = ai_model_bundle_ids[i];
    res_sensor =
        SensorStreamSetProperty(stream, key, &property, sizeof(property));
    if (res_sensor != 0) {
      LOG_WARN("ApiTest failed %d\n", res_sensor);
      CLEANUP_RELEASEFRAME
      return -STREAM_SET_PROPERTY - PARAM_ALL - RETUN_NRM + res_sensor;
    }
  }

  CLEANUP_RELEASEFRAME
  return 0;
}

int32_t RunApiTestScenarioChannel() {
  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream;
  EdgeAppLibSensorFrame frame;
  int32_t res_sensor;
  uint32_t channel_id;
  EdgeAppLibSensorChannel channel = 0;
  SensorCoreInit(&core);
  SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
  SensorStart(stream);
  SensorGetFrame(stream, &frame, -1);

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT;
  // API SensorFrameGetChannelFromChannelId 0, channel_id:0, channel, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
#endif  // MOCK_APITEST
  res_sensor = SensorFrameGetChannelFromChannelId(0, channel_id, &channel);
#if MOCK_APITEST
  resetEdgeAppLibSensorFrameGetChannelFromChannelIdSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -FRAME_GET_CHANNEL_FROM_CHANNEL_ID - PARAM_01 - RETUN_ERR +
           res_sensor;
  }

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE + 1;
  // API SensorFrameGetChannelFromChannelId frame, channel_id:2, channel, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
#endif  // MOCK_APITEST
  res_sensor = SensorFrameGetChannelFromChannelId(frame, channel_id, &channel);
#if MOCK_APITEST
  resetEdgeAppLibSensorFrameGetChannelFromChannelIdSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -FRAME_GET_CHANNEL_FROM_CHANNEL_ID - PARAM_02 - BOUNDARY_OVER_MAX +
           res_sensor;
  }

  // API SensorFrameGetChannelFromChannelId frame, 999, channel, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
#endif  // MOCK_APITEST
  res_sensor = SensorFrameGetChannelFromChannelId(frame, 999, &channel);
#if MOCK_APITEST
  resetEdgeAppLibSensorFrameGetChannelFromChannelIdSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -FRAME_GET_CHANNEL_FROM_CHANNEL_ID - PARAM_02 - RETUN_ERR +
           res_sensor;
  }

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE;
  // API SensorFrameGetChannelFromChannelId frame, channel_id:1, NULL, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorFrameGetChannelFromChannelIdFail();
#endif  // MOCK_APITEST
  res_sensor = SensorFrameGetChannelFromChannelId(frame, channel_id, NULL);
#if MOCK_APITEST
  resetEdgeAppLibSensorFrameGetChannelFromChannelIdSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -FRAME_GET_CHANNEL_FROM_CHANNEL_ID - PARAM_03 - RETUN_ERR +
           res_sensor;
  }

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT;
  // API SensorFrameGetChannelFromChannelId frame, channel_id:0, channel, ret:0
  res_sensor = SensorFrameGetChannelFromChannelId(frame, channel_id, &channel);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -FRAME_GET_CHANNEL_FROM_CHANNEL_ID - PARAM_ALL - RETUN_NRM +
           res_sensor;
  }

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE;
  // API SensorFrameGetChannelFromChannelId frame, channel_id:1, channel, ret:0
  res_sensor = SensorFrameGetChannelFromChannelId(frame, channel_id, &channel);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -FRAME_GET_CHANNEL_FROM_CHANNEL_ID - PARAM_ALL - BOUNDARY_MAX +
           res_sensor;
  }

  struct EdgeAppLibSensorRawData raw_data;
  // API SensorChannelGetRawData 0 raw_data, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorChannelGetRawDataFail();
#endif  // MOCK_APITEST
  res_sensor = SensorChannelGetRawData(0, &raw_data);
#if MOCK_APITEST
  resetEdgeAppLibSensorChannelGetRawDataSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_RAW_DATA - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorChannelGetRawData channel NULL, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorChannelGetRawDataFail();
#endif  // MOCK_APITEST
  res_sensor = SensorChannelGetRawData(channel, NULL);
#if MOCK_APITEST
  resetEdgeAppLibSensorChannelGetRawDataSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_RAW_DATA - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorChannelGetRawData channel raw_data, ret:0
  res_sensor = SensorChannelGetRawData(channel, &raw_data);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_RAW_DATA - PARAM_ALL - RETUN_NRM + res_sensor;
  }

  const char *key = AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY;
  EdgeAppLibSensorAiModelBundleIdProperty property2;

  // API SensorChannelGetProperty 0, property_key, value, value_size,
  // ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorChannelGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorChannelGetProperty(0, key, &property2, sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorChannelGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_PROPERTY - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorChannelGetProperty channel, no_key, value, value_size, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorChannelGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorChannelGetProperty(channel, "no_key", &property2,
                                        sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorChannelGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_PROPERTY - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorChannelGetProperty channel, NULL, value, value_size, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorChannelGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor =
      SensorChannelGetProperty(channel, NULL, &property2, sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorChannelGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_PROPERTY - PARAM_02 - RETUN_ERR + res_sensor;
  }

  // API SensorChannelGetProperty channel, property_key, NULL, value_size,
  // ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorChannelGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorChannelGetProperty(channel, key, NULL, sizeof(property2));
#if MOCK_APITEST
  resetEdgeAppLibSensorChannelGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_PROPERTY - PARAM_03 - RETUN_ERR + res_sensor;
  }

  // API SensorChannelGetProperty channel, property_key, value, 1, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorChannelGetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorChannelGetProperty(channel, key, &property2, 1);
#if MOCK_APITEST
  resetEdgeAppLibSensorChannelGetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_PROPERTY - PARAM_04 - RETUN_ERR + res_sensor;
  }

  // API SensorChannelGetProperty channel property_key, value, value_size,
  // ret:0
  res_sensor =
      SensorChannelGetProperty(channel, key, &property2, sizeof(property2));
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -CHANNEL_GET_PROPERTY - PARAM_ALL - RETUN_NRM + res_sensor;
  }

  EdgeAppLibSensorInputDataTypeProperty enabled = {};
  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE;

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT;
  // API SensorInputDataTypeEnableChannel NULL, channel_id:0, true, ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorInputDataTypeEnableChannelFail();
#endif  // MOCK_APITEST
  res_sensor = SensorInputDataTypeEnableChannel(NULL, channel_id, true);
#if MOCK_APITEST
  resetEdgeAppLibSensorInputDataTypeEnableChannelSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_01 - RETUN_ERR + res_sensor;
  }

  // API SensorInputDataTypeEnableChannel property, channel_id:2, true, ret:0
  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE + 1;
  res_sensor = SensorInputDataTypeEnableChannel(&enabled, channel_id, true);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_02 - BOUNDARY_OVER_MAX +
           res_sensor;
  }
  // API SensorInputDataTypeEnableChannel > SensorStreamSetProperty ret:-1
#if MOCK_APITEST
  setEdgeAppLibSensorStreamSetPropertyFail();
#endif  // MOCK_APITEST
  res_sensor = SensorStreamSetProperty(
      stream, AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY, &enabled,
      sizeof(enabled));
#if MOCK_APITEST
  resetEdgeAppLibSensorStreamSetPropertySuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_02 - BOUNDARY_OVER_MAX +
           res_sensor;
  }

  // API SensorInputDataTypeEnableChannel property, 128, true, ret:-1
  EdgeAppLibSensorInputDataTypeProperty enabled_err = {};
  int i;
  for (i = 0; i < AITRIOS_SENSOR_CHANNEL_LIST_MAX; i++) {
    res_sensor = SensorInputDataTypeEnableChannel(&enabled, i, true);
    if (res_sensor != 0) {
      LOG_WARN("ApiTest failed %d\n", res_sensor);
      CLEANUP_RELEASEFRAME
      return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_02 - RETUN_ERR +
             res_sensor;
    }
  }
  // API SensorInputDataTypeEnableChannel property, 129, true, ret:-1
  i += 1;
  res_sensor = SensorInputDataTypeEnableChannel(&enabled, i, true);
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_02 - BOUNDARY_OVER_MAX +
           res_sensor;
  }

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT;
  // API SensorInputDataTypeEnableChannel property, channel_id:0, true, ret:0
  res_sensor = SensorInputDataTypeEnableChannel(&enabled, channel_id, true);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_01 - RETUN_NRM + res_sensor;
  }

  // API SensorInputDataTypeEnableChannel property, channel_id:0, false, ret:0
  res_sensor = SensorInputDataTypeEnableChannel(&enabled, channel_id, false);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_03 - RETUN_NRM + res_sensor;
  }

  channel_id = AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE;
  // API SensorInputDataTypeEnableChannel property, channel_id:1, true, ret:0
  res_sensor = SensorInputDataTypeEnableChannel(&enabled, channel_id, true);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    CLEANUP_RELEASEFRAME
    return -INPUT_DATA_TYPE_ENABLE_CHANNEL - PARAM_ALL - BOUNDARY_MAX +
           res_sensor;
  }

  CLEANUP_RELEASEFRAME
  return 0;
}

int32_t RunApiTestScenarioError() {
  EdgeAppLibSensorCore core = 0;
  EdgeAppLibSensorStream stream;
  EdgeAppLibSensorFrame frame;
  int32_t res_sensor;
  EdgeAppLibSensorChannel channel = 0;
  SensorCoreInit(&core);
  SensorCoreOpenStream(core, AITRIOS_SENSOR_STREAM_KEY_DEFAULT, &stream);
  SensorStart(stream);

  EdgeAppLibSensorStatusParam param;
  char *buffer;
  uint32_t buffer_length = 256;
  // API SensorGetLastErrorString param_OVER_MAX, buffer, length, ret:-1
  buffer_length = 256;
  buffer = (char *)malloc(buffer_length);
  param = (EdgeAppLibSensorStatusParam)(AITRIOS_SENSOR_STATUS_PARAM_TRACE + 1);
#if MOCK_APITEST
  setEdgeAppLibSensorGetLastErrorStringFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetLastErrorString(param, buffer, &buffer_length);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetLastErrorStringSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_01 - BOUNDARY_OVER_MAX + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorString param_OVER_MIN, buffer, length, ret:-1
  buffer_length = 256;
  buffer = (char *)malloc(buffer_length);
  param =
      (EdgeAppLibSensorStatusParam)(AITRIOS_SENSOR_STATUS_PARAM_MESSAGE - 1);
#if MOCK_APITEST
  setEdgeAppLibSensorGetLastErrorStringFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetLastErrorString(param, buffer, &buffer_length);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetLastErrorStringSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_01 - BOUNDARY_OVER_MIN + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorString NULL, buffer, length, ret:-1
  buffer_length = 256;
  buffer = (char *)malloc(buffer_length);
  param = (EdgeAppLibSensorStatusParam)NULL;
#if MOCK_APITEST
  setEdgeAppLibSensorGetLastErrorStringFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetLastErrorString(param, buffer, &buffer_length);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetLastErrorStringSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_01 - RETUN_ERR + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorString param_MESSAGE, NULL, length, ret:-1
  buffer_length = 256;
  buffer = (char *)malloc(buffer_length);
  param = AITRIOS_SENSOR_STATUS_PARAM_MESSAGE;
#if MOCK_APITEST
  setEdgeAppLibSensorGetLastErrorStringFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetLastErrorString(param, NULL, &buffer_length);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetLastErrorStringSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_02 - RETUN_ERR + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorString param_MESSAGE, buffer, BOUNDARY_MIN, ret:0
  buffer_length = 0;
  buffer = (char *)malloc(buffer_length);
  param = AITRIOS_SENSOR_STATUS_PARAM_MESSAGE;
  res_sensor = SensorGetLastErrorString(param, buffer, &buffer_length);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_03 - BOUNDARY_MIN + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorString param_MESSAGE, buffer, NULL, ret:-1
  buffer_length = 256;
  buffer = (char *)malloc(buffer_length);
  param = AITRIOS_SENSOR_STATUS_PARAM_MESSAGE;
#if MOCK_APITEST
  setEdgeAppLibSensorGetLastErrorStringFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetLastErrorString(param, buffer, NULL);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetLastErrorStringSuccess();
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_03 - RETUN_ERR + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorString param_MESSAGE, buffer, length, ret:0
  buffer = (char *)malloc(buffer_length);
  param = AITRIOS_SENSOR_STATUS_PARAM_MESSAGE;
  res_sensor = SensorGetLastErrorString(param, buffer, &buffer_length);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_01 - RETUN_NRM + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorString param_TRACE, buffer, length, ret:0
  buffer_length = 256;
  buffer = (char *)malloc(buffer_length);
  param = AITRIOS_SENSOR_STATUS_PARAM_TRACE;
  res_sensor = SensorGetLastErrorString(param, buffer, &buffer_length);
  if (res_sensor != 0) {
    LOG_WARN("ApiTest failed %d\n", res_sensor);
    free(buffer);
    CLEANUP_STOP
    return -GET_LAST_ERROR_STRING - PARAM_01 - BOUNDARY_MAX + res_sensor;
  }
  free(buffer);

  // API SensorGetLastErrorLevel ret:LEVEL_UNDEFINED
  EdgeAppLibSensorErrorLevel level = SensorGetLastErrorLevel();
  if (level != AITRIOS_SENSOR_LEVEL_UNDEFINED) {
    LOG_WARN("ApiTest failed %d\n", level);
    CLEANUP_STOP
    return -GET_LAST_ERROR_LEVEL - RETUN - RETUN_NRM + level;
  }

  // API SensorGetLastErrorCause ret:ERROR_NONE
  EdgeAppLibSensorErrorCause cause = SensorGetLastErrorCause();
  if (cause != AITRIOS_SENSOR_ERROR_NONE) {
    LOG_WARN("ApiTest failed %d\n", cause);
    CLEANUP_STOP
    return -GET_LAST_ERROR_CAUSE - RETUN - RETUN_NRM + cause;
  }

#if MOCK_APITEST
  setEdgeAppLibSensorGetFrameFail();
#endif  // MOCK_APITEST
  res_sensor = SensorGetFrame(0, &frame, -1);
#if MOCK_APITEST
  resetEdgeAppLibSensorGetFrameSuccess();
  // SensorReleaseFrame(NULL, frame);
#endif  // MOCK_APITEST
  if (res_sensor != -1) {
    LOG_WARN("ApiTest failed %d\n", cause);
    CLEANUP_STOP
    return -GET_LAST_ERROR_LEVEL - RETUN - RETUN_ERR + res_sensor;
  }

  // API SensorGetLastErrorLevel ret:LEVEL_FAIL
#if MOCK_APITEST
  setEdgeAppLibSensorGetLastErrorLevelFail();
#endif  // MOCK_APITEST
  level = SensorGetLastErrorLevel();
#if MOCK_APITEST
  resetEdgeAppLibSensorGetLastErrorLevelSuccess();
#endif  // MOCK_APITEST
  if (level != AITRIOS_SENSOR_LEVEL_FAIL) {
    LOG_WARN("ApiTest failed %d\n", level);
    CLEANUP_STOP
    return -GET_LAST_ERROR_LEVEL - RETUN - RETUN_ERR + level;
  }

  // API SensorGetLastErrorCause ret:ERROR_INVALID_ARGUMENT
#if MOCK_APITEST
  setEdgeAppLibSensorGetLastErrorCauseFail2(
      AITRIOS_SENSOR_ERROR_INVALID_ARGUMENT);
#endif  // MOCK_APITEST
  cause = SensorGetLastErrorCause();
#if MOCK_APITEST
  resetEdgeAppLibSensorGetLastErrorCauseSuccess();
#endif  // MOCK_APITEST
  if (cause != AITRIOS_SENSOR_ERROR_INVALID_ARGUMENT) {
    LOG_WARN("ApiTest failed %d\n", cause);
    CLEANUP_STOP
    return -GET_LAST_ERROR_CAUSE - RETUN - RETUN_ERR + cause;
  }

  CLEANUP_STOP
  return 0;
}
