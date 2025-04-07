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

#ifndef DETECTION_UTILS_H
#define DETECTION_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "data_processor_api.hpp"
#include "flatbuffers/flatbuffers.h"
#include "parson.h"
#include "poseestimation_generated.h"

/* -------------------------------------------------------- */
/* define                                                   */
/* -------------------------------------------------------- */

#define CST_POSENET_INPUT_TENSOR_WIDTH (481)
#define CST_POSENET_INPUT_TENSOR_HEIGHT (353)
#define CST_POSENET_OUTPUT_TENSOR_WIDTH (31)
#define CST_POSENET_OUTPUT_TENSOR_HEIGHT (23)
#define CST_POSENET_SCORE_THRESHOLD (0.5f)
#define CST_POSENET_IOU_THRESHOLD (0.28f)
#define CST_POSENET_NMS_RADIUS (20)
#define CST_POSENET_MAX_POSE_DETECTIONS (15)
#define CST_POSENET_HEATMAP_INDEX (0)
#define CST_POSENET_OFFSET_INDEX (1)
#define CST_POSENET_FORWARD_DISPLACEMENT_INDEX (2)
#define CST_POSENET_BACKWARD_DISPLACEMENT_INDEX (3)

typedef struct tagDataProcessorCustomParam {
  uint16_t input_width;
  uint16_t input_height;
  uint16_t output_width;
  uint16_t output_height;
  float score_threshold;
  float iou_threshold;
  uint16_t nms_radius;
  uint16_t max_pose_detections;
  uint8_t heatmap_index;
  uint8_t offset_index;
  uint8_t forward_displacement_index;
  uint8_t backward_displacement_index;
} DataProcessorCustomParam;

#define ONLY_FACE_POINT (0)

enum pose_key_id {
  eKeyNose = 0,  //  0
  eKeyLeftEye,   //  1
  eKeyRightEye,  //  2
  eKeyLeftEar,   //  3
  eKeyRightEar,  //  4
#if ONLY_FACE_POINT
#else
  eKeyLeftShoulder,   //  5
  eKeyRightShoulder,  //  6
  eKeyLeftElbow,      //  7
  eKeyRightElbow,     //  8
  eKeyLeftWrist,      //  9
  eKeyRightWrist,     // 10
  eKeyLeftHip,        // 11
  eKeyRightHip,       // 12
  eKeyLeftKnee,       // 13
  eKeyRightKnee,      // 14
  eKeyLeftAnkle,      // 15
  eKeyRightAnkle,     // 16
#endif         // ONLY_FACE_POINT
  eKeyPoseNum  // MAX
};

typedef struct tag_pose_key_t {
  float x;
  float y;
  float score;
} pose_key_t;

typedef struct tag_pose_t {
  float pose_score;
  pose_key_t keypoint[eKeyPoseNum];
} pose_t;

DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractOutputWidth(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractOutputHeight(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractScoreThreshold(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractIoUThreshold(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractNmsRadius(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractMaxPoseDetections(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractHeatmapIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractOffsetIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractForwardDisplacementIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode ExtractBackwardDisplacementIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr);
DataProcessorResultCode VerifyConstraints(
    JSON_Object *json, DataProcessorCustomParam *yolo_param_pr);

int createPoseNetData(float *data_body, uint32_t in_size,
                      DataProcessorCustomParam &analyze_params,
                      std::vector<pose_t> &pose_result);
void createPoseEstimationOutputFlatbuffer(
    flatbuffers::FlatBufferBuilder *builder,
    DataProcessorCustomParam &analyze_params, std::vector<pose_t> &pose_result);

JSON_Value *CreatePoseNetOutputJson(std::vector<pose_t> &pose_result,
                                    DataProcessorCustomParam &analyze_params);

#endif  // DETECTION_UTILS_H
