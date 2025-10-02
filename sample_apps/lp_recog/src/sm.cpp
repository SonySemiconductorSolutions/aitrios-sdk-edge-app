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

#include "sm.h"

#include <math.h>
#include <stdlib.h>

#include "data_export.h"
#include "data_processor_api.hpp"
#include "draw.h"
#include "edgeapp_core.h"
#include "log.h"
#include "lp_recog_utils.hpp"
#include "receive_data.h"
#include "send_data.h"
#include "sensor.h"
#include "sm_utils.hpp"

#define MAX_PATH_LEN 256
#define DEFAULT_LPR_MODEL_NAME "lp_recognition"

using namespace EdgeAppLib;
EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;
int32_t res_release_frame = -1;
static char lpr_ai_model_path[MAX_PATH_LEN] = DEFAULT_LPR_MODEL_NAME;

static EdgeAppCoreCtx ctx_imx500;
static EdgeAppCoreCtx ctx_cpu;
static EdgeAppCoreCtx *ctx_list[2] = {&ctx_imx500, &ctx_cpu};
static EdgeAppCoreCtx *shared_list[2] = {nullptr, &ctx_imx500};
extern char lpd_imx500_model_id[AI_MODEL_BUNDLE_ID_SIZE];

// Define static vectors for model parameters
static const std::vector<float> imx500_mean = {0.0f, 0.0f, 0.0f};
static const std::vector<float> imx500_std = {0.0f, 0.0f, 0.0f};
static const std::vector<float> cpu_mean = {0.0f, 0.0f, 0.0f};
static const std::vector<float> cpu_std = {0.0f, 0.0f, 0.0f};

EdgeAppCoreModelInfo models[2] = {
    {lpd_imx500_model_id, EdgeAppCoreTarget::edge_imx500, &imx500_mean,
     &imx500_std},
    {lpr_ai_model_path, EdgeAppCoreTarget::edge_cpu, &cpu_mean, &cpu_std}};

static const uint32_t model_count = 2;
// Flag to indicate if this is the first iteration
// Since there is no support yet for Wasi-threads and Wasi-nn, we need to
// initialize the models inside onIterate.
static bool first_flag = true;

EdgeAppLibSensorImageCropProperty roi[2] = {
    {0, 0, 2028, 1520},
    {0, 0, 300, 300}  // It must be smaller than input tensor size of imx500
};                    // These values are initial values.
int onCreate() {
  LOG_TRACE("Inside onCreate.");
  return 0;
}

int onConfigure(char *topic, void *value, int valuesize) {
  LOG_TRACE("Inside onConfigure.");
  if (value == NULL) {
    LOG_ERR("[onConfigure] Invalid param : value=NULL");
    return -1;
  }
  LOG_INFO("[onConfigure] topic:%s\nvalue:%s\nvaluesize:%i\n", topic,
           (char *)value, valuesize);

  char *output = NULL;
  DataProcessorResultCode res;
  if ((res = DataProcessorConfigure((char *)value, &output)) !=
      kDataProcessorOk) {
    DataExportSendState(topic, (void *)output, strlen(output));
    free(value);
    return (res == kDataProcessorInvalidParam) ? 0 : -1;
  }
  DataExportSendState(topic, value, valuesize);
  return 0;
}

int onIterate() {
  LOG_TRACE("Inside onIterate.");

  // Process the frame using the sensor stream
  auto frame = EdgeAppCore::Process(ctx_imx500, &ctx_imx500, 0, roi[0]);
  if (frame == 0) {
    LOG_ERR("Failed to get frame from sensor stream.");
    return -1;
  }
  // Output tensor from detection model
  auto output = EdgeAppCore::GetOutput(ctx_imx500, frame, 4);

  LPDataProcessorAnalyzeParam param;
  LPAnalysisParam lp_param;
  lp_param.roi = &roi[1];
  lp_param.tensor = &output;
  param.app_specific = &lp_param;

  DataProcessorResultCode data_processor_ret =
      LPDDataProcessorAnalyze((float *)output.data, output.size, &param);

  if (data_processor_ret != kDataProcessorOk) {
    LOG_ERR("DataProcessorAnalyze: ret=%d", data_processor_ret);
    return -1;
  }

  LOG_DBG(
      "roi[1]: [left=%d, top=%d, width=%d, "
      "height=%d]",
      roi[1].left, roi[1].top, roi[1].width, roi[1].height);

  auto input = EdgeAppCore::GetInput(ctx_imx500, frame);
  if (input.data == nullptr || input.size == 0) {
    LOG_ERR("Input tensor is empty or invalid.");
    return -1;
  }

  // Draw rectangles on the input image
  if (roi[1].width != 0 && roi[1].height != 0) {
    struct EdgeAppLibDrawBuffer buffer = {
        input.data, input.size, AITRIOS_DRAW_FORMAT_RGB8,
        input.shape_info.dims[2], input.shape_info.dims[1]};
    DrawRectangle(&buffer, roi[1].left, roi[1].top, roi[1].left + roi[1].width,
                  roi[1].top + roi[1].height, AITRIOS_COLOR_BLUE);
  }

  if (EdgeAppCore::SendInputTensor(&input) != EdgeAppCoreResultSuccess) {
    LOG_ERR("Failed to send input tensor.");
  }

  LOG_DBG("Start processing frames for additional models on CPU.");

  for (int i = 1; i < model_count; ++i) {
    LOG_DBG("model_count=%d, i=%d", model_count, i);

    frame = EdgeAppCore::Process(*ctx_list[i], shared_list[i], frame, roi[i]);
    if (frame == 0) {
      LOG_ERR("Failed to process frame for context %d.", i);
      return -1;
    }

    EdgeAppCore::Tensor output_cpu =
        EdgeAppCore::GetOutput(*ctx_list[i], frame, 4);
    LOG_INFO("Output tensor size: %zu", output_cpu.size);
    if (output_cpu.data == nullptr || output_cpu.size == 0) {
      LOG_ERR("Output tensor is empty or invalid.");
      return -1;
    }
    // Analyze (Read Plate)
    char *recognized_data = NULL;
    uint32_t recognized_data_size = 0;
    LPRDataProcessorAnalyze(static_cast<float *>(output_cpu.data),
                            output_cpu.size, &recognized_data,
                            &recognized_data_size);

    // Send plate number if recognized data is valid as Japanese number plate
    if (is_valid_japanese_number_plate(recognized_data)) {
      if (SendDataSyncMeta((void *)recognized_data, recognized_data_size,
                           DataProcessorGetDataType(), output_cpu.timestamp) !=
          EdgeAppLibSendDataResultSuccess) {
        LOG_ERR("Failed to send inference data.");
      }
    }

    if (output_cpu.data) {
      free(output_cpu.data);  // Always free for CPU model
      output_cpu.data = nullptr;
    }
    if (recognized_data != NULL && recognized_data_size > 0) {
      free(recognized_data);
    }
  }
  return 0;
}

int onStop() {
  LOG_TRACE("Inside onStop.");
  EdgeAppCoreCtx *ctx_list[] = {&ctx_imx500, &ctx_cpu};
  for (auto ctx : ctx_list) {
    EdgeAppCore::UnloadModel(*ctx);
  }
  return 0;
}

int onStart() {
  LOG_TRACE("Inside onStart.");
  for (int i = 0; i < model_count; ++i) {
    LOG_DBG(
        "Model ctx %d: sensor_core=%p, sensor_stream=%p, "
        "graph_ctx=%p, target=%d",
        i, ctx_list[i]->sensor_core, ctx_list[i]->sensor_stream,
        ctx_list[i]->graph_ctx, ctx_list[i]->target);
    if (EdgeAppCore::LoadModel(models[i], *ctx_list[i], shared_list[i]) !=
        EdgeAppCoreResultSuccess) {
      LOG_ERR("Failed to load model %d.", i);
    } else {
      LOG_INFO("Successfully loaded model %d: %s", i, models[i].model_name);
    }
    LOG_DBG(
        "Model ctx %d: sensor_core=%p, sensor_stream=%p, "
        "graph_ctx=%p, target=%d",
        i, ctx_list[i]->sensor_core, ctx_list[i]->sensor_stream,
        ctx_list[i]->graph_ctx, ctx_list[i]->target);
  }
  s_stream = *ctx_imx500.sensor_stream;
  return 0;
}

int onDestroy() {
  LOG_TRACE("Inside onDestroy.");
  EdgeAppCoreCtx *ctx_list[] = {&ctx_imx500, &ctx_cpu};
  for (auto ctx : ctx_list) {
    EdgeAppCore::UnloadModel(*ctx);
  }
  return 0;
}
