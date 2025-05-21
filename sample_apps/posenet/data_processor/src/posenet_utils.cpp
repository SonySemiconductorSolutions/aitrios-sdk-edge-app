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

#include "posenet_utils.hpp"

#include <edgeapp/log.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

#include "data_processor_api.hpp"
#include "data_processor_utils.hpp"
#include "parson.h"
#include "poseestimation_generated.h"
#include "posenet_utils.hpp"

DataProcessorCustomParam g_posenet_param = {
    CST_POSENET_INPUT_TENSOR_WIDTH,
    CST_POSENET_INPUT_TENSOR_HEIGHT,
    CST_POSENET_OUTPUT_TENSOR_WIDTH,
    CST_POSENET_OUTPUT_TENSOR_HEIGHT,
    CST_POSENET_SCORE_THRESHOLD,
    CST_POSENET_IOU_THRESHOLD,
    CST_POSENET_NMS_RADIUS,
    CST_POSENET_MAX_POSE_DETECTIONS,
    CST_POSENET_HEATMAP_INDEX,
    CST_POSENET_OFFSET_INDEX,
    CST_POSENET_FORWARD_DISPLACEMENT_INDEX,
    CST_POSENET_BACKWARD_DISPLACEMENT_INDEX};

DataProcessorCustomParam g_pre_posenet_param = g_posenet_param;

/* -------------------------------------------------------- */
/* structure                                                */
/* -------------------------------------------------------- */
typedef struct tag_part_score_t {
  float score;  // key point's score
  int index_x;  // point in x-coordinate after convolution.
  int index_y;  // point in y-coordionate after convolution.
  int key_id;   // key point's index.
} part_score_t;

typedef struct tag_keypoint_t {
  float point_x;  // point in x-coordinate in input image width
  float point_y;  // point in x-coordinate in input image height
  float score;    // heatmap's score
  int valid;      // if this keypoint is valid, vailid is 1 otherwise 0.
} keypoint_t;

#define CLAMP(val, min_val, max_val) (std::max(min_val, std::min(max_val, val)))

#define FACE_DETECT_MAX_NUM (20U)

#define MAX_DIMENSION (3)
typedef struct tag_dequant_imx500_metadata {
  size_t numOfDimensions;
  size_t dimension_size[MAX_DIMENSION];
  size_t
      total_byte_size;  // ((dimension_size[0])x(dimension_size[1])x(dimension_size[2])
  void *base;  // output tensor already dequantized and it is necessary to
               // transpose
  void *data;  // after transposing tensor
} dequant_imx500_metadata;

typedef struct tag_FaceDetectionResult {
  unsigned int id;  // Face Index
  float faceX;      // Upper left X coordinate of the face in the original image
                    // [pixel]
  float faceY;      // Upper left Y coordinate of the face in the original image
                    // [pixel]
  float faceW;      // Width of the face in the original image [pixel]
  float faceH;      // Height of the face in the original image [pixel]
  float faceScore;  // The score of the detected face area
} FaceDetectionResult;

typedef struct tag_FaceDetectionConfig {
  //=== for decord PoseNet output ========
  float thresholdScore;  // Score threshold (parameter of the face detection)
  int thresholdLocalMaxRad;
  float thresholdNmsRad;
  int thresholdRefineSteps;  // about1-10
  float thresholdClustering;
  //======================================
} FaceDetectionConfig;

// for C++14 or after
typedef struct tag_PoseNetDecodeInfo {
  // Threshold
  float score_thresh;
  int local_max_rad;
  float nms_rad;
  int refine_steps;
  float iou_thresh;

  // Base Image Data
  int base_image_w;
  int base_image_h;

  // PoseNet Inference Input
  int input_tensor_w;
  int input_tensor_h;

  // PoseNet Inference Output Result
  float *heatmap;
  float *offset;
  float *fw_disp;  // forward-displacement
  float *bk_disp;  // back-displacement

  int heatmap_dims_x;
  int heatmap_dims_y;
  int heatmap_dims_num;
  int offset_dims_x;
  int offset_dims_y;
  int offset_dims_num;
  int fw_disp_dims_x;
  int fw_disp_dims_y;
  int fw_disp_dims_num;
  int bk_disp_dims_x;
  int bk_disp_dims_y;
  int bk_disp_dims_num;
} PoseNetDecordInfo;

// init param only. overwritten by ppl parameter.
#define OUTPUT_TENSOR_DIMS_H (23)
#define OUTPUT_TENSOR_DIMS_W (31)

#define OUTPUT_TENSOR_HEATMAP_NUM (17)   // heatmap
#define OUTPUT_TENSOR_OFFSET_NUM (34)    // short vec offset
#define OUTPUT_TENSOR_FORWARD_NUM (32)   // mid vec forward-displacement
#define OUTPUT_TENSOR_BACKWARD_NUM (32)  // mid vec backward-displacement

// from dnnParams.xml(except padding setting)
//  initial parameter
static dequant_imx500_metadata meta_heatmap = {
    .numOfDimensions = 3,
    .dimension_size =
        {OUTPUT_TENSOR_DIMS_H, OUTPUT_TENSOR_DIMS_W,
         OUTPUT_TENSOR_HEATMAP_NUM},  // HWC, we must set HW initial value
    .total_byte_size = 0,             // calculated by createPoseNetData
    .base = nullptr,
    .data = nullptr};

static dequant_imx500_metadata meta_offset = {
    .numOfDimensions = 3,
    .dimension_size = {OUTPUT_TENSOR_DIMS_H, OUTPUT_TENSOR_DIMS_W,
                       OUTPUT_TENSOR_OFFSET_NUM},
    .total_byte_size = 0,  // calculated by createPoseNetData
    .base = nullptr,
    .data = nullptr};

static dequant_imx500_metadata meta_forward_displacement = {
    .numOfDimensions = 3,
    .dimension_size = {OUTPUT_TENSOR_DIMS_H, OUTPUT_TENSOR_DIMS_W,
                       OUTPUT_TENSOR_FORWARD_NUM},
    .total_byte_size = 0,  // calculated by createPoseNetData
    .base = nullptr,
    .data = nullptr};

static dequant_imx500_metadata meta_back_displacement = {
    .numOfDimensions = 3,
    .dimension_size = {OUTPUT_TENSOR_DIMS_H, OUTPUT_TENSOR_DIMS_W,
                       OUTPUT_TENSOR_BACKWARD_NUM},
    .total_byte_size = 0,  // calculated by createPoseNetData
    .base = nullptr,
    .data = nullptr};

// w481 h353 model's order is forward-->backward
// w449 h449 model's order is backward-->forward
// initial parameter
static std::vector<dequant_imx500_metadata> meta_data{
    meta_heatmap,
    meta_offset,
    meta_forward_displacement,
    meta_back_displacement,
};

// initial parameter
static std::vector<void **> rearrange_data{
    &(meta_heatmap.data), &(meta_offset.data),
    &(meta_forward_displacement.data), &(meta_back_displacement.data)};

// for output display
const static int pose_edges[][2] = {
    // parent,            child
    {eKeyNose, eKeyLeftEye},       //  0
    {eKeyLeftEye, eKeyLeftEar},    //  1
    {eKeyNose, eKeyRightEye},      //  2
    {eKeyRightEye, eKeyRightEar},  //  3
#if ONLY_FACE_POINT
#else
    {eKeyNose, eKeyLeftShoulder},         //  4
    {eKeyLeftShoulder, eKeyLeftElbow},    //  5
    {eKeyLeftElbow, eKeyLeftWrist},       //  6
    {eKeyLeftShoulder, eKeyLeftHip},      //  7
    {eKeyLeftHip, eKeyLeftKnee},          //  8
    {eKeyLeftKnee, eKeyLeftAnkle},        //  9
    {eKeyNose, eKeyRightShoulder},        // 10
    {eKeyRightShoulder, eKeyRightElbow},  // 11
    {eKeyRightElbow, eKeyRightWrist},     // 12
    {eKeyRightShoulder, eKeyRightHip},    // 13
    {eKeyRightHip, eKeyRightKnee},        // 14
    {eKeyRightKnee, eKeyRightAnkle},      // 15
#endif  // ONLY_FACE_POINT
};

/* -------------------------------------------------------- */
/* static                                                   */
/* -------------------------------------------------------- */
static bool isIndexValid(std::vector<uint8_t> ppl_idnex,
                         std::vector<uint8_t> expected_index,
                         int &ppl_ng_index);
static bool isMetaChange(DataProcessorCustomParam &left,
                         DataProcessorCustomParam &right);
static void RearrangeMetaDataCore(
    std::vector<dequant_imx500_metadata>::iterator meta);
static void PoseNetPostDecodeOutputMakeArgument(
    const FaceDetectionConfig &config, PoseNetDecordInfo *posenet_info,
    DataProcessorCustomParam &analyze_params);
static float get_heatmap_score(PoseNetDecordInfo *posenet_info, int index_x,
                               int index_y, int key_id);
static bool score_is_max_in_local_window(PoseNetDecordInfo *posenet_info,
                                         int index_x, int index_y, int key_id,
                                         float current_score);
static bool compare_part_score(part_score_t &part_s1, part_score_t &part_s2);
static void make_part_score_list(PoseNetDecordInfo *posenet_info,
                                 std::vector<part_score_t> &part_score_list);
static void get_offset_vector(PoseNetDecordInfo *posenet_info,
                              part_score_t *parts_info, float *offset_x,
                              float *offset_y);
static void get_index_to_pos(PoseNetDecordInfo *posenet_info,
                             part_score_t *parts_info, float *pos_x,
                             float *pos_y);
static bool within_nms_of_corresponding_point(PoseNetDecordInfo *posenet_info,
                                              std::vector<pose_t> &pose_result,
                                              int key_id, float pos_x,
                                              float pos_y,
                                              const int current_face_num);
static void get_pos_to_near_index(PoseNetDecordInfo *posenet_info,
                                  float keypoint_x, float keypoint_y,
                                  int *src_hmap_index_x, int *src_hmap_index_y);
static void get_displacement_vector(PoseNetDecordInfo *posenet_info,
                                    float *displacement_data, int hmap_index_x,
                                    int hmap_index_y, int edge_id,
                                    float *displacement_x,
                                    float *displacement_y);
static void traverse_to_target_key(PoseNetDecordInfo *posenet_info, int edge,
                                   keypoint_t *keypoints, int source_key_id,
                                   int target_key_id, float *displacement_data);
static void decode_pose(PoseNetDecordInfo *posenet_info,
                        part_score_t *parts_info, float pos_x, float pos_y,
                        keypoint_t *key_points);
static float get_instance_score(PoseNetDecordInfo *posenet_info,
                                std::vector<pose_t> &pose_result,
                                keypoint_t *key_points,
                                const int current_face_num);
static int regist_detected_pose(PoseNetDecordInfo *posenet_info,
                                std::vector<pose_t> &pose_result,
                                keypoint_t *key_points, float score_average);
static int PoseNetPostDecodeOutput(PoseNetDecordInfo *posenet_info,
                                   std::vector<pose_t> &pose_result);
static void PoseNetPostPostPackFaceResult(
    PoseNetDecordInfo *posenet_info, std::vector<pose_t> &pose_result,
    std::vector<FaceDetectionResult> &vec_face_result, const int face_num);
static float calc_intersection_over_union(FaceDetectionResult &face0,
                                          FaceDetectionResult &face1);
static bool compare_face_score(FaceDetectionResult &v1,
                               FaceDetectionResult &v2);
static int PoseNetPostNonMaxSuppression(
    std::vector<FaceDetectionResult> &face_list,
    std::vector<FaceDetectionResult> &face_sel_list,
    std::vector<pose_t> &pose_list, std::vector<pose_t> &pose_sel_list,
    float iou_thresh);
static bool compare_pose_score(pose_t &a, pose_t &b);

DataProcessorResultCode ExtractInputWidth(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_width", &aux) == 0) {
    posenet_param_pr->input_width = (uint16_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->input_width = CST_POSENET_INPUT_TENSOR_WIDTH;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_width' parameter "
      "is %d",
      CST_POSENET_INPUT_TENSOR_WIDTH);
  json_object_set_number(json, "input_width", CST_POSENET_INPUT_TENSOR_WIDTH);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractInputHeight(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "input_height", &aux) == 0) {
    posenet_param_pr->input_height = (uint16_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->input_height = CST_POSENET_INPUT_TENSOR_HEIGHT;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'input_height' parameter "
      "is %d",
      CST_POSENET_INPUT_TENSOR_HEIGHT);
  json_object_set_number(json, "input_height", CST_POSENET_INPUT_TENSOR_HEIGHT);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractOutputWidth(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "output_width", &aux) == 0) {
    posenet_param_pr->output_width = (uint16_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->output_width = CST_POSENET_OUTPUT_TENSOR_WIDTH;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'output_width' parameter "
      "is %d",
      CST_POSENET_OUTPUT_TENSOR_WIDTH);
  json_object_set_number(json, "output_width", CST_POSENET_OUTPUT_TENSOR_WIDTH);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractOutputHeight(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "output_height", &aux) == 0) {
    posenet_param_pr->output_height = (uint16_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->output_height = CST_POSENET_OUTPUT_TENSOR_HEIGHT;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'output_height' parameter "
      "is %d",
      CST_POSENET_OUTPUT_TENSOR_HEIGHT);
  json_object_set_number(json, "output_height",
                         CST_POSENET_OUTPUT_TENSOR_HEIGHT);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractScoreThreshold(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "score_threshold", &aux) == 0) {
    posenet_param_pr->score_threshold = (float)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->score_threshold = CST_POSENET_SCORE_THRESHOLD;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'score_threshold' parameter "
      "is %f",
      CST_POSENET_SCORE_THRESHOLD);
  json_object_set_number(json, "score_threshold", CST_POSENET_SCORE_THRESHOLD);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractIoUThreshold(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "iou_threshold", &aux) == 0) {
    posenet_param_pr->iou_threshold = (float)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->iou_threshold = CST_POSENET_IOU_THRESHOLD;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'iou_threshold' parameter "
      "is %f",
      CST_POSENET_IOU_THRESHOLD);
  json_object_set_number(json, "iou_threshold", CST_POSENET_IOU_THRESHOLD);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractNmsRadius(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "nms_radius", &aux) == 0) {
    posenet_param_pr->nms_radius = (uint16_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->nms_radius = CST_POSENET_NMS_RADIUS;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'nms_radius' parameter "
      "is %d",
      CST_POSENET_NMS_RADIUS);
  json_object_set_number(json, "nms_radius", CST_POSENET_NMS_RADIUS);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractMaxPoseDetections(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "max_pose_detections", &aux) == 0) {
    posenet_param_pr->max_pose_detections = (uint16_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->max_pose_detections = CST_POSENET_MAX_POSE_DETECTIONS;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'max_pose_detections' "
      "parameter "
      "is %d",
      CST_POSENET_MAX_POSE_DETECTIONS);
  json_object_set_number(json, "max_pose_detections",
                         CST_POSENET_MAX_POSE_DETECTIONS);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractHeatmapIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "heatmap_index", &aux) == 0) {
    posenet_param_pr->heatmap_index = (uint8_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->heatmap_index = CST_POSENET_HEATMAP_INDEX;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'heatmap_index' parameter "
      "is %d",
      CST_POSENET_HEATMAP_INDEX);
  json_object_set_number(json, "heatmap_index", CST_POSENET_HEATMAP_INDEX);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractOffsetIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "offset_index", &aux) == 0) {
    posenet_param_pr->offset_index = (uint8_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->offset_index = CST_POSENET_OFFSET_INDEX;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'offset_index' parameter "
      "is %d",
      CST_POSENET_OFFSET_INDEX);
  json_object_set_number(json, "offset_index", CST_POSENET_OFFSET_INDEX);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractForwardDisplacementIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "forward_displacement_index", &aux) == 0) {
    posenet_param_pr->forward_displacement_index = (uint8_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->forward_displacement_index =
      CST_POSENET_FORWARD_DISPLACEMENT_INDEX;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'forward_displacement_index' "
      "parameter "
      "is %d",
      CST_POSENET_FORWARD_DISPLACEMENT_INDEX);
  json_object_set_number(json, "forward_displacement_index",
                         CST_POSENET_FORWARD_DISPLACEMENT_INDEX);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode ExtractBackwardDisplacementIndex(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  double aux = 0;
  if (GetValueNumber(json, "backward_displacement_index", &aux) == 0) {
    posenet_param_pr->backward_displacement_index = (uint8_t)aux;
    return kDataProcessorOk;
  }
  posenet_param_pr->backward_displacement_index =
      CST_POSENET_BACKWARD_DISPLACEMENT_INDEX;
  LOG_INFO(
      "DataProcessorConfigure: default value of 'backward_displacement_index' "
      "parameter "
      "is %d",
      CST_POSENET_BACKWARD_DISPLACEMENT_INDEX);
  json_object_set_number(json, "backward_displacement_index",
                         CST_POSENET_BACKWARD_DISPLACEMENT_INDEX);
  return kDataProcessorInvalidParam;
}
DataProcessorResultCode VerifyConstraints(
    JSON_Object *json, DataProcessorCustomParam *posenet_param_pr) {
  if (posenet_param_pr->score_threshold < 0.0 ||
      posenet_param_pr->score_threshold > 1.0) {
    LOG_WARN(
        "score_threshold value out of range, set to default score_threshold");
    posenet_param_pr->score_threshold = CST_POSENET_SCORE_THRESHOLD;
    json_object_set_number(json, "score_threshold",
                           posenet_param_pr->score_threshold);
    return kDataProcessorInvalidParam;
  }

  if (posenet_param_pr->iou_threshold < 0.0 ||
      posenet_param_pr->iou_threshold > 1.0) {
    LOG_WARN("iou_threshold value out of range, set to default iou_threshold");
    posenet_param_pr->iou_threshold = CST_POSENET_IOU_THRESHOLD;
    json_object_set_number(json, "iou_threshold",
                           posenet_param_pr->iou_threshold);
    return kDataProcessorInvalidParam;
  }

  return kDataProcessorOk;
}

/**
 * @brief createPoseNetData
 */
int createPoseNetData(float *data_body, uint32_t in_size,
                      DataProcessorCustomParam &analyze_params,
                      std::vector<pose_t> &pose_result) {
  size_t nSize = in_size * sizeof(float);
  float *p_data = (float *)malloc(nSize);
  if (p_data == NULL) {
    LOG_ERR("malloc failed for creating rearrangement memory, malloc size=%ld",
            nSize);
    return kDataProcessorMemoryError;
  }

  if (isMetaChange(g_pre_posenet_param, analyze_params)) {
    // Order is HWC
    meta_heatmap.dimension_size[0] = analyze_params.output_height;
    meta_heatmap.dimension_size[1] = analyze_params.output_width;

    meta_offset.dimension_size[0] = analyze_params.output_height;
    meta_offset.dimension_size[1] = analyze_params.output_width;

    meta_forward_displacement.dimension_size[0] = analyze_params.output_height;
    meta_forward_displacement.dimension_size[1] = analyze_params.output_width;

    meta_back_displacement.dimension_size[0] = analyze_params.output_height;
    meta_back_displacement.dimension_size[1] = analyze_params.output_width;

    int meta_num = meta_data.size();
    meta_data.clear();
    rearrange_data.clear();

    // reconstruct meta_data and rearrange_data
    for (int i = 0; i < meta_num; i++) {
      if (analyze_params.heatmap_index == i) {
        meta_data.push_back(meta_heatmap);
        rearrange_data.push_back(&(meta_heatmap.data));
      } else if (analyze_params.offset_index == i) {
        meta_data.push_back(meta_offset);
        rearrange_data.push_back(&(meta_offset.data));
      } else if (analyze_params.forward_displacement_index == i) {
        meta_data.push_back(meta_forward_displacement);
        rearrange_data.push_back(&(meta_forward_displacement.data));
      } else {
        // only analyze_params.backward_displacement_index == i
        meta_data.push_back(meta_back_displacement);
        rearrange_data.push_back(&(meta_back_displacement.data));
      }
    }
  }

  size_t meta_all_size = 0;
  int n_cnt = 0;
  for (auto meta = meta_data.begin(); meta != meta_data.end(); meta++) {
    // attenntion do not reflect at master meta_data.
    meta->base = static_cast<void *>(data_body + meta_all_size);
    meta->data = static_cast<void *>(p_data + meta_all_size);

    meta->total_byte_size = 1;
    for (size_t dim_num = 0; dim_num < meta->numOfDimensions; dim_num++) {
      meta->total_byte_size *= meta->dimension_size[dim_num];
    }
    meta_all_size += meta->total_byte_size;

    RearrangeMetaDataCore(meta);

    // it is necessary to update (meta_heatmap, meta_offset,
    // meta_forward_displacement, meta_back_displacement).data
    *(rearrange_data[n_cnt]) = meta->data;

    n_cnt++;
  }

  // Face Detection
  FaceDetectionConfig detect_config;
  detect_config.thresholdScore =
      0.5f;  // heatmap's score is different from pose score
  detect_config.thresholdLocalMaxRad =
      1;  // you should set 1 to thresholdLocalMaxRad
  detect_config.thresholdNmsRad = (float)analyze_params.nms_radius;
  detect_config.thresholdRefineSteps = 10;
  detect_config.thresholdClustering = analyze_params.iou_threshold;

  int face_detect_num = 0;
  std::vector<FaceDetectionResult> face_result;
  PoseNetDecordInfo posenet_info;

  PoseNetPostDecodeOutputMakeArgument(detect_config, &posenet_info,
                                      analyze_params);

  std::vector<pose_t> pose_unclustering_result;
  std::vector<FaceDetectionResult> face_unclustering_result;
  face_detect_num =
      PoseNetPostDecodeOutput(&posenet_info, pose_unclustering_result);
  PoseNetPostPostPackFaceResult(&posenet_info, pose_unclustering_result,
                                face_unclustering_result, face_detect_num);
  face_detect_num = PoseNetPostNonMaxSuppression(
      face_unclustering_result, face_result, pose_unclustering_result,
      pose_result, posenet_info.iou_thresh);

  // pose_result is not sorted by pose_score, sorted by face score
  // so it is necessary to sort by pose_result.
  std::sort(pose_result.begin(), pose_result.end(), compare_pose_score);

  if (face_detect_num > analyze_params.max_pose_detections) {
    pose_result.resize(analyze_params.max_pose_detections);
    face_detect_num = pose_result.size();
  }

  int new_pose_cnt = 0;
  for (auto itr = pose_result.begin(); itr != pose_result.end(); itr++) {
    if (itr->pose_score < analyze_params.score_threshold) {
      break;
    }
    new_pose_cnt++;
  }
  if (face_detect_num > new_pose_cnt) {
    pose_result.resize(new_pose_cnt);
    face_detect_num = pose_result.size();
  }

#if POSENET_DBGOUT_
  LOG_DBG("input_width = %d, input_height = %d", analyze_params.input_width,
          analyze_params.input_height);
  for (int i = 0; i < pose_result.size(); i++) {
    pose_t *pose_data = &pose_result[i];
    if (pose_data->pose_score >= analyze_params.score_threshold) {
      LOG_DBG("i = %d", i);
      LOG_DBG("score = %f", pose_data->pose_score);
      for (int j = 0; j < OUTPUT_TENSOR_HEATMAP_NUM; j++) {
        int x = pose_data->keypoint[j].x * analyze_params.input_width;
        int y = pose_data->keypoint[j].y * analyze_params.input_height;
        float score = pose_data->keypoint[j].score;
        LOG_DBG("%d,%d,%f", x, y, score);
      }
    }
  }
#endif

  LOG_INFO("detect face num : %d", face_detect_num);
  free(p_data);
  g_pre_posenet_param = analyze_params;

  return 0;
}

/* Function to serialize PoseEstimation output tensor data into Flatbuffers.
 */
void createPoseEstimationOutputFlatbuffer(
    flatbuffers::FlatBufferBuilder *builder,
    DataProcessorCustomParam &analyze_params,
    std::vector<pose_t> &pose_result) {
  std::vector<flatbuffers::Offset<SmartCamera::GeneralPose>> gpose_vector;

  uint16_t w = analyze_params.input_width;
  uint16_t h = analyze_params.input_height;

  LOG_DBG("createFlatbuffer");

  for (int i = 0; i < pose_result.size(); i++) {
    pose_t *pose_data = &pose_result[i];
    std::vector<flatbuffers::Offset<SmartCamera::KeyPoint>> keypoint_vector;
    for (int8_t j = 0; j <= SmartCamera::KeyPointName_MAX; j++) {
      int32_t x = std::round(pose_data->keypoint[j].x * (w - 1));
      int32_t y = std::round(pose_data->keypoint[j].y * (h - 1));
      auto pt = SmartCamera::CreatePoint2d(*builder, x, y);
      auto each_kp = SmartCamera::CreateKeyPoint(
          *builder, pose_data->keypoint[j].score, SmartCamera::Point_Point2d,
          pt.Union(), (SmartCamera::KeyPointName)j);
      keypoint_vector.push_back(each_kp);
    }
    auto v_kp = builder->CreateVector(keypoint_vector);
    auto general_pose =
        SmartCamera::CreateGeneralPose(*builder, pose_data->pose_score, v_kp);
    gpose_vector.push_back(general_pose);
  }
  auto v_pose = builder->CreateVector(gpose_vector);
  auto od_data = SmartCamera::CreatePoseEstimationData(*builder, v_pose);
  auto out_data = SmartCamera::CreatePoseEstimationTop(*builder, od_data);

  builder->Finish(out_data);

  return;
}

JSON_Value *CreatePoseNetOutputJson(std::vector<pose_t> &pose_result,
                                    DataProcessorCustomParam &analyze_params) {
  uint16_t w = analyze_params.input_width;
  uint16_t h = analyze_params.input_height;

  LOG_DBG("Creating JSON from array of floats");

  JSON_Value *posenets_value = json_value_init_array();
  JSON_Array *posenets = json_array(posenets_value);

  for (int i = 0; i < pose_result.size(); i++) {
    JSON_Value *posenet_value = json_value_init_object();
    JSON_Object *posenet = json_object(posenet_value);

    JSON_Value *keypoints_value = json_value_init_array();
    JSON_Array *keypoints = json_array(keypoints_value);

    pose_t *pose_data = &pose_result[i];

    for (int8_t j = 0; j <= SmartCamera::KeyPointName_MAX; j++) {
      JSON_Value *keypoint_value = json_value_init_object();
      JSON_Object *keypoint = json_object(keypoint_value);

      json_object_set_number(keypoint, "score", pose_data->keypoint[j].score);

      JSON_Value *point_value = json_value_init_object();
      JSON_Object *point = json_object(point_value);

      int32_t x = std::round(pose_data->keypoint[j].x * (w - 1));
      int32_t y = std::round(pose_data->keypoint[j].y * (h - 1));

      json_object_set_number(point, "x", x);
      json_object_set_number(point, "y", y);

      json_object_set_value(keypoint, "point", point_value);
      json_object_set_number(keypoint, "name", j);

      json_array_append_value(keypoints, keypoint_value);
    }

    json_object_set_value(posenet, "keypoint", keypoints_value);
    json_object_set_number(posenet, "score", pose_data->pose_score);

    json_array_append_value(posenets, posenet_value);
  }

  return posenets_value;
}

/* -------------------------------------------------------- */
/* static                                                   */
/* -------------------------------------------------------- */

/**
 * @brief isIndexValid
 */
static bool isIndexValid(std::vector<uint8_t> ppl_idnex,
                         std::vector<uint8_t> expected_index,
                         int &ppl_ng_index) {
  bool is_valid = true;
  ppl_ng_index = 0;
  for (auto ppl_itr = ppl_idnex.begin(); ppl_itr != ppl_idnex.end();
       ppl_itr++) {
    int hit_index = -1;
    int index = 0;
    for (auto exp_itr = expected_index.begin(); exp_itr != expected_index.end();
         exp_itr++) {
      if (*ppl_itr == *exp_itr) {
        hit_index = index;
        break;
      }
      index++;
    }
    if (hit_index < 0) {
      is_valid = false;
      break;
    } else {
      expected_index.erase(expected_index.begin() + hit_index);
    }
    ppl_ng_index++;
  }

  return is_valid;
}

/**
 * @brief isMetaChange
 */
static bool isMetaChange(DataProcessorCustomParam &left,
                         DataProcessorCustomParam &right) {
  // only meta parameter
  if ((left.input_width == right.input_width) &&
      (left.input_height == right.input_height) &&
      (left.output_width == right.output_width) &&
      (left.output_height == right.output_height) &&
      (left.heatmap_index == right.heatmap_index) &&
      (left.offset_index == right.offset_index) &&
      (left.forward_displacement_index == right.forward_displacement_index) &&
      (left.backward_displacement_index == right.backward_displacement_index)) {
    return false;
  } else {
    return true;
  }
}

// it is necessary to transpose the output tensor because TFLite output tensor
// order is not same with IMX500 output tensor order. TFLite HWC(23, 31, 17) -->
// IMX500 output CWH(17, 31, 23), ordinal:0 is inner loop
/**
 * @brief RearrangeMetaDataCore
 */
static void RearrangeMetaDataCore(
    std::vector<dequant_imx500_metadata>::iterator meta) {
  size_t h_len = meta->dimension_size[0];
  size_t w_len = meta->dimension_size[1];
  size_t c_base = meta->dimension_size[2];

  // it is necessary to transpose from HWC to CWH
  float *pData = (float(*))(meta->data);
  float *pBase = (float(*))(meta->base);
  for (size_t index_c = 0; index_c < c_base; index_c++) {
    for (size_t index_w = 0; index_w < w_len; index_w++) {
      for (size_t index_h = 0; index_h < h_len; index_h++) {
        size_t dest_index = index_c + (c_base * (index_w + (w_len * index_h)));
        size_t base_index = index_h + (h_len * (index_w + (w_len * index_c)));
        float org_data = (pBase[base_index]);
        pData[dest_index] = org_data;
      }
    }
  }
}

/**
 * @brief PoseNetPostDecodeOutputMakeArgument
 */
static void PoseNetPostDecodeOutputMakeArgument(
    const FaceDetectionConfig &config, PoseNetDecordInfo *posenet_info,
    DataProcessorCustomParam &analyze_params) {
  posenet_info->score_thresh = config.thresholdScore;
  posenet_info->local_max_rad = config.thresholdLocalMaxRad;
  posenet_info->nms_rad = config.thresholdNmsRad;
  posenet_info->refine_steps = config.thresholdRefineSteps;
  posenet_info->iou_thresh = config.thresholdClustering;

  // Base Image Data
  posenet_info->base_image_w = analyze_params.input_width;
  posenet_info->base_image_h = analyze_params.input_height;

  // PoseNet Inference Input
  posenet_info->input_tensor_w = analyze_params.input_width;
  posenet_info->input_tensor_h = analyze_params.input_height;

  // PoseNet Inference Output Result
  posenet_info->heatmap = static_cast<float *>(meta_heatmap.data);
  posenet_info->offset = static_cast<float *>(meta_offset.data);
  posenet_info->fw_disp = static_cast<float *>(meta_forward_displacement.data);
  posenet_info->bk_disp = static_cast<float *>(meta_back_displacement.data);

  // dimension_size's order is HWC. index1 is x direction, index0 is y direction
  posenet_info->heatmap_dims_x = meta_heatmap.dimension_size[1];
  posenet_info->heatmap_dims_y = meta_heatmap.dimension_size[0];
  posenet_info->heatmap_dims_num = meta_heatmap.dimension_size[2];

  posenet_info->offset_dims_x = meta_offset.dimension_size[1];
  posenet_info->offset_dims_y = meta_offset.dimension_size[0];
  posenet_info->offset_dims_num = meta_offset.dimension_size[2];

  posenet_info->fw_disp_dims_x = meta_forward_displacement.dimension_size[1];
  posenet_info->fw_disp_dims_y = meta_forward_displacement.dimension_size[0];
  posenet_info->fw_disp_dims_num = meta_forward_displacement.dimension_size[2];

  posenet_info->bk_disp_dims_x = meta_back_displacement.dimension_size[1];
  posenet_info->bk_disp_dims_y = meta_back_displacement.dimension_size[0];
  posenet_info->bk_disp_dims_num = meta_back_displacement.dimension_size[2];

#if POSENET_DBGOUT_
  LOG_INFO("posenet_info->score_thresh : %f", posenet_info->score_thresh);
  LOG_INFO("posenet_info->local_max_rad : %d", posenet_info->local_max_rad);
  LOG_INFO("posenet_info->nms_rad : %f", posenet_info->nms_rad);
  LOG_INFO("posenet_info->refine_steps : %d", posenet_info->refine_steps);
  LOG_INFO("posenet_info->iou_thresh : %f", posenet_info->iou_thresh);
  LOG_INFO("posenet_info->base_image_w : %d", posenet_info->base_image_w);
  LOG_INFO("posenet_info->base_image_h : %d", posenet_info->base_image_h);
  LOG_INFO("posenet_info->input_tensor_w : %d", posenet_info->input_tensor_w);
  LOG_INFO("posenet_info->input_tensor_h : %d", posenet_info->input_tensor_h);
  LOG_INFO("posenet_info->heatmap : %p", posenet_info->heatmap);
  LOG_INFO("posenet_info->offset : %p", posenet_info->offset);
  LOG_INFO("posenet_info->fw_disp : %p", posenet_info->fw_disp);
  LOG_INFO("posenet_info->bk_disp : %p", posenet_info->bk_disp);
  LOG_INFO("posenet_info->heatmap_dims_x : %d", posenet_info->heatmap_dims_x);
  LOG_INFO("posenet_info->heatmap_dims_y : %d", posenet_info->heatmap_dims_y);
  LOG_INFO("posenet_info->heatmap_dims_num : %d",
           posenet_info->heatmap_dims_num);
  LOG_INFO("posenet_info->offset_dims_x : %d", posenet_info->offset_dims_x);
  LOG_INFO("posenet_info->offset_dims_y : %d", posenet_info->offset_dims_y);
  LOG_INFO("posenet_info->offset_dims_num : %d", posenet_info->offset_dims_num);
  LOG_INFO("posenet_info->fw_disp_dims_x : %d", posenet_info->fw_disp_dims_x);
  LOG_INFO("posenet_info->fw_disp_dims_y : %d", posenet_info->fw_disp_dims_y);
  LOG_INFO("posenet_info->fw_disp_dims_num : %d",
           posenet_info->fw_disp_dims_num);
  LOG_INFO("posenet_info->bk_disp_dims_x : %d", posenet_info->bk_disp_dims_x);
  LOG_INFO("posenet_info->bk_disp_dims_y : %d", posenet_info->bk_disp_dims_y);
  LOG_INFO("posenet_info->bk_disp_dims_num : %d",
           posenet_info->bk_disp_dims_num);
#endif  // POSENET_DBGOUT_
}

/**
 * @brief get_heatmap_score
 */
static float get_heatmap_score(PoseNetDecordInfo *posenet_info, int index_x,
                               int index_y, int key_id) {
  int index = (index_y * posenet_info->heatmap_dims_x *
               posenet_info->heatmap_dims_num) +
              (index_x * posenet_info->heatmap_dims_num) + key_id;
  float score_sigmoid = 1.0f / (1.0f + exp(-posenet_info->heatmap[index]));
  return score_sigmoid;
}

/*
 * If the score is the highest in local window, return true.
 *
 *    xs    xe
 *   +--+--+--+
 *   |  |  |  | ys
 *   +--+--+--+
 *   |  |##|  |         ##: (idx_x, idx_y)
 *   +--+--+--+
 *   |  |  |  | ye
 *   +--+--+--+
 */
/**
 * @brief score_is_max_in_local_window
 */
static bool score_is_max_in_local_window(PoseNetDecordInfo *posenet_info,
                                         int index_x, int index_y, int key_id,
                                         float current_score) {
  if (posenet_info->local_max_rad <= 0) {
    return true;
  }

  int max_rad = posenet_info->local_max_rad;
  int x_start = std::max(index_x - max_rad, 0);
  int y_start = std::max(index_y - max_rad, 0);
  int x_end = std::min(index_x + max_rad + 1, posenet_info->heatmap_dims_x);
  int y_end = std::min(index_y + max_rad + 1, posenet_info->heatmap_dims_y);

  for (int y = y_start; y < y_end; y++) {
    for (int x = x_start; x < x_end; x++) {
      // if a higher score is found, return false
      if (get_heatmap_score(posenet_info, x, y, key_id) > current_score) {
        return false;
      }
    }
  }
  return true;
}

/**
 * @brief compare_part_score
 */
static bool compare_part_score(part_score_t &part_s1, part_score_t &part_s2) {
  if (part_s1.score > part_s2.score)
    return true;
  else
    return false;
}

/**
 * @brief make_part_score_list
 */
static void make_part_score_list(PoseNetDecordInfo *posenet_info,
                                 std::vector<part_score_t> &part_score_list) {
  part_score_t part_item;

  for (int y = 0; y < posenet_info->heatmap_dims_y; y++) {
    for (int x = 0; x < posenet_info->heatmap_dims_x; x++) {
      for (int key = 0; key < eKeyPoseNum; key++) {
        float score = get_heatmap_score(posenet_info, x, y, key);
        // if this score is lower than thresh, skip this pixel.
        if (score < posenet_info->score_thresh) {
          continue;
        }
        // if there is a higher score near this pixel, skip this pixel.
        if (score_is_max_in_local_window(posenet_info, x, y, key, score) ==
            false) {
          continue;
        }
        part_item.score = score;
        part_item.index_x = x;
        part_item.index_y = y;
        part_item.key_id = key;
        part_score_list.push_back(part_item);
      }
    }
  }
  std::sort(part_score_list.begin(), part_score_list.end(), compare_part_score);
#if POSENET_DBGOUT_
  // for debug
  for (auto itr = part_score_list.begin(); itr != part_score_list.end();
       itr++) {
    part_score_t part_data = *itr;
    LOG_INFO("score = %f, index_x = %d, index_y = %d, key_id = %d",
             part_data.score, part_data.index_x, part_data.index_y,
             part_data.key_id);
  }
#endif  // POSENET_DBGOUT_
}

/**
 * @brief get_offset_vector
 */
static void get_offset_vector(PoseNetDecordInfo *posenet_info,
                              part_score_t *parts_info, float *offset_x,
                              float *offset_y) {
  int x_offset_index = (parts_info->index_y * posenet_info->offset_dims_x *
                        posenet_info->offset_dims_num) +
                       (parts_info->index_x * posenet_info->offset_dims_num) +
                       (parts_info->key_id + posenet_info->heatmap_dims_num);
  int y_offset_index = (parts_info->index_y * posenet_info->offset_dims_x *
                        posenet_info->offset_dims_num) +
                       (parts_info->index_x * posenet_info->offset_dims_num) +
                       (parts_info->key_id);
  *offset_x = posenet_info->offset[x_offset_index];
  *offset_y = posenet_info->offset[y_offset_index];
#if POSENET_DBGOUT_
  LOG_INFO("x_offset_index = %d", x_offset_index);
  LOG_INFO("y_offset_index = %d", y_offset_index);
  LOG_INFO("offset_x = %f", *offset_x);
  LOG_INFO("offset_y = %f", *offset_y);
#endif  // POSENET_DBGOUT_
}

/**
 * @brief get_index_to_pos
 */
static void get_index_to_pos(PoseNetDecordInfo *posenet_info,
                             part_score_t *parts_info, float *pos_x,
                             float *pos_y) {
  float offset_x, offset_y;
  get_offset_vector(posenet_info, parts_info, &offset_x, &offset_y);

  // normalize
  float rel_x = ((float)parts_info->index_x) /
                ((float)posenet_info->heatmap_dims_x - 1.0f);
  float rel_y = ((float)parts_info->index_y) /
                ((float)posenet_info->heatmap_dims_y - 1.0f);

  // denormalize
  *pos_x = (rel_x * posenet_info->input_tensor_w) + offset_x;
  *pos_y = (rel_y * posenet_info->input_tensor_h) + offset_y;
#if POSENET_DBGOUT_
  LOG_INFO("offset_x = %f", offset_x);
  LOG_INFO("offset_y = %f", offset_y);
  LOG_INFO("rel_x = %f", rel_x);
  LOG_INFO("rel_y = %f", rel_y);
  LOG_INFO("pos_x = %f", *pos_x);
  LOG_INFO("pos_y = %f", *pos_y);
#endif  // POSENET_DBGOUT_
}

/**
 * @brief within_nms_of_corresponding_point
 */
static bool within_nms_of_corresponding_point(PoseNetDecordInfo *posenet_info,
                                              std::vector<pose_t> &pose_result,
                                              int key_id, float pos_x,
                                              float pos_y,
                                              const int current_face_num) {
  if (pose_result.empty()) {
    return false;
  }

  for (int i = 0; i < current_face_num; i++) {
    pose_t *pose = &pose_result[i];
    float prev_keypos_x =
        pose->keypoint[key_id].x * posenet_info->input_tensor_w;
    float prev_keypos_y =
        pose->keypoint[key_id].y * posenet_info->input_tensor_h;

    float dx = pos_x - prev_keypos_x;
    float dy = pos_y - prev_keypos_y;
    float key_distance = (dx * dx) + (dy * dy);

    if (key_distance <= (posenet_info->nms_rad * posenet_info->nms_rad)) {
      return true;
    }
  }
  return false;
}

/*
 *  0      28.5    57.1    85.6   114.2   142.7   171.3   199.9   228.4   257
 * [pos_x]
 *  |---+---|---+---|---+---|---+---|---+---|---+---|---+---|---+---|---+---|
 *     0.0     1.0     2.0     3.0     4.0     5.0     6.0     7.0     8.0
 * [hmp_pos_x]
 */
/**
 * @brief get_pos_to_near_index
 */
static void get_pos_to_near_index(PoseNetDecordInfo *posenet_info,
                                  float keypoint_x, float keypoint_y,
                                  int *src_hmap_index_x,
                                  int *src_hmap_index_y) {
  float ratio_x = keypoint_x / (float)posenet_info->input_tensor_w;
  float ratio_y = keypoint_y / (float)posenet_info->input_tensor_h;

  float hmap_pos_x = ratio_x * (posenet_info->heatmap_dims_x - 1);
  float hmap_pos_y = ratio_y * (posenet_info->heatmap_dims_y - 1);

  int hmap_index_x = std::roundf(hmap_pos_x);
  int hmap_index_y = std::roundf(hmap_pos_y);

  hmap_index_x = CLAMP(hmap_index_x, 0, (posenet_info->heatmap_dims_x - 1));
  hmap_index_y = CLAMP(hmap_index_y, 0, (posenet_info->heatmap_dims_y - 1));

  *src_hmap_index_x = hmap_index_x;
  *src_hmap_index_y = hmap_index_y;

#if POSENET_DBGOUT_
  LOG_INFO("keypoint_x = %f", keypoint_x);
  LOG_INFO("keypoint_y = %f", keypoint_y);
  LOG_INFO("ratio_x = %f", ratio_x);
  LOG_INFO("ratio_y = %f", ratio_y);
  LOG_INFO("hmap_pos_x = %f", hmap_pos_x);
  LOG_INFO("hmap_pos_y = %f", hmap_pos_y);
  LOG_INFO("src_hmap_index_x = %d", *src_hmap_index_x);
  LOG_INFO("src_hmap_index_y = %d", *src_hmap_index_y);
#endif  // POSENET_DBGOUT_
}

/**
 * @brief get_displacement_vector
 */
static void get_displacement_vector(PoseNetDecordInfo *posenet_info,
                                    float *displacement_data, int hmap_index_x,
                                    int hmap_index_y, int edge_id,
                                    float *displacement_x,
                                    float *displacement_y) {
  int pose_edge_num =
      posenet_info->fw_disp_dims_num / 2;  // correct in this case (= stride)
  int x_displacement_index =
      (hmap_index_y * posenet_info->heatmap_dims_x * pose_edge_num * 2) +
      (hmap_index_x * pose_edge_num * 2) + (edge_id + pose_edge_num);
  int y_displacement_index =
      (hmap_index_y * posenet_info->heatmap_dims_x * pose_edge_num * 2) +
      (hmap_index_x * pose_edge_num * 2) + (edge_id);
  *displacement_x = displacement_data[x_displacement_index];
  *displacement_y = displacement_data[y_displacement_index];
#if POSENET_DBGOUT_
  LOG_INFO("x_displacement_index = %d", x_displacement_index);
  LOG_INFO("y_displacement_index = %d", y_displacement_index);
  LOG_INFO("displacement_x = %f", *displacement_x);
  LOG_INFO("displacement_y = %f", *displacement_y);
#endif  // POSENET_DBGOUT_
}

/**
 * @brief traverse_to_target_key
 */
static void traverse_to_target_key(PoseNetDecordInfo *posenet_info, int edge,
                                   keypoint_t *keypoints, int source_key_id,
                                   int target_key_id,
                                   float *displacement_data) {
  float src_point_x = keypoints[source_key_id].point_x;
  float src_point_y = keypoints[source_key_id].point_y;

  int src_hmap_index_x, src_hmap_index_y;
  get_pos_to_near_index(posenet_info, src_point_x, src_point_y,
                        &src_hmap_index_x, &src_hmap_index_y);

  /* get displacement vector from source to target */
  float displacement_x, displacement_y;
  get_displacement_vector(posenet_info, displacement_data, src_hmap_index_x,
                          src_hmap_index_y, edge, &displacement_x,
                          &displacement_y);
  // calculate target position
  float target_point_x = src_point_x + displacement_x;
  float target_point_y = src_point_y + displacement_y;

  int target_index_x = 0, target_index_y = 0;
  part_score_t target_part_info;
  int offset_refine_step = posenet_info->refine_steps;
  // refine process
  for (int i = 0; i < offset_refine_step; i++) {
    get_pos_to_near_index(posenet_info, target_point_x, target_point_y,
                          &target_index_x, &target_index_y);
    target_part_info.index_x = target_index_x;
    target_part_info.index_y = target_index_y;
    target_part_info.key_id = target_key_id;
    target_part_info.score = 0;
    float pre_target_point_x = target_point_x;
    float pre_target_point_y = target_point_y;
    get_index_to_pos(posenet_info, &target_part_info, &target_point_x,
                     &target_point_y);
    if ((pre_target_point_x == target_point_x) &&
        (pre_target_point_y == target_point_y)) {
      break;
    }
  }

  // make return struct values
  keypoints[target_key_id].point_x = target_point_x;
  keypoints[target_key_id].point_y = target_point_y;
  keypoints[target_key_id].score = get_heatmap_score(
      posenet_info, target_index_x, target_index_y, target_key_id);
  keypoints[target_key_id].valid = 1;
}

/**
 * @brief decode_pose
 */
static void decode_pose(PoseNetDecordInfo *posenet_info,
                        part_score_t *parts_info, float pos_x, float pos_y,
                        keypoint_t *key_points) {
  // calculate root key position.
  int key_id = parts_info->key_id;
  int pose_edge_num = sizeof(pose_edges) / sizeof(pose_edges[0]);

  key_points[key_id].point_x = pos_x;
  key_points[key_id].point_y = pos_y;
  key_points[key_id].score = parts_info->score;
  key_points[key_id].valid = 1;

  // backward
  for (int edge = pose_edge_num - 1; edge >= 0; edge--) {
    int source_key_id = pose_edges[edge][1];
    int target_key_id = pose_edges[edge][0];

    if (key_points[source_key_id].valid && !key_points[target_key_id].valid) {
      traverse_to_target_key(posenet_info, edge, key_points, source_key_id,
                             target_key_id, posenet_info->bk_disp);
    }
  }

  // forward
  for (int edge = 0; edge < pose_edge_num; edge++) {
    int source_key_id = pose_edges[edge][0];
    int target_key_id = pose_edges[edge][1];

    if (key_points[source_key_id].valid && !key_points[target_key_id].valid) {
      traverse_to_target_key(posenet_info, edge, key_points, source_key_id,
                             target_key_id, posenet_info->fw_disp);
    }
  }
}

/**
 * @brief get_instance_score
 */
static float get_instance_score(PoseNetDecordInfo *posenet_info,
                                std::vector<pose_t> &pose_result,
                                keypoint_t *key_points,
                                const int current_face_num) {
  float score_total = 0.0f;
  for (int key = 0; key < eKeyPoseNum; key++) {
    float pos_x = key_points[key].point_x;
    float pos_y = key_points[key].point_y;
    if (within_nms_of_corresponding_point(posenet_info, pose_result, key, pos_x,
                                          pos_y, current_face_num) == true) {
      continue;
    }

    score_total += key_points[key].score;
  }
  return (score_total / (float)eKeyPoseNum);  // average score
}

/**
 * @brief regist_detected_pose
 */
static int regist_detected_pose(PoseNetDecordInfo *posenet_info,
                                std::vector<pose_t> &pose_result,
                                keypoint_t *key_points, float score_average) {
  pose_t pose;

  for (int key = 0; key < eKeyPoseNum; key++) {
    pose.keypoint[key].x =
        key_points[key].point_x / (float)posenet_info->input_tensor_w;
    pose.keypoint[key].y =
        key_points[key].point_y / (float)posenet_info->input_tensor_h;
    pose.keypoint[key].score = key_points[key].score;
#if POSENET_DBGOUT_
    LOG_INFO("- regist_keypoints[%d].point_x = %f", key,
             key_points[key].point_x);
    LOG_INFO("- regist_keypoints[%d].point_y = %f", key,
             key_points[key].point_y);
    LOG_INFO("- regist_keypoints[%d].score = %f", key, key_points[key].score);
    LOG_INFO("- regist_pose.keypoint[%d].x = %f", key, pose.keypoint[key].x);
    LOG_INFO("- regist_pose.keypoint[%d].y = %f", key, pose.keypoint[key].y);
    LOG_INFO("- regist_pose.keypoint[%d].score = %f", key,
             pose.keypoint[key].score);
#endif  // POSENET_DBGOUT_
  }
  pose.pose_score = score_average;

  pose_result.push_back(pose);

  return 1;
}

/**
 * @brief PoseNetPostDecodeOutput
 */
static int PoseNetPostDecodeOutput(PoseNetDecordInfo *posenet_info,
                                   std::vector<pose_t> &pose_result) {
  int face_num = 0;
  size_t list_index = 0;
  std::vector<part_score_t> part_score_list;
  float pos_x, pos_y;

  make_part_score_list(posenet_info, part_score_list);

  if (part_score_list.empty()) {
    return 0;
  }

  while (face_num < (int)FACE_DETECT_MAX_NUM &&
         list_index < part_score_list.size()) {
    part_score_t *parts_info = &part_score_list[list_index];
#if POSENET_DBGOUT_
    LOG_INFO("score = %f, index_x = %d, index_y = %d, key_id = %d",
             parts_info->score, parts_info->index_x, parts_info->index_y,
             parts_info->key_id);
#endif  // POSENET_DBGOUT_

    get_index_to_pos(posenet_info, parts_info, &pos_x, &pos_y);

    if (within_nms_of_corresponding_point(posenet_info, pose_result,
                                          parts_info->key_id, pos_x, pos_y,
                                          face_num) == true) {
      list_index += 1;
      continue;
    }

    keypoint_t key_points[eKeyPoseNum] = {0.0f, 0.0f, 0.0f, 0};
    decode_pose(posenet_info, parts_info, pos_x, pos_y, key_points);
#if POSENET_DBGOUT_
    for (int key = 0; key < eKeyPoseNum; key++) {
      LOG_INFO("keypoints[%d].point_x = %f", key, key_points[key].point_x);
      LOG_INFO("keypoints[%d].point_y = %f", key, key_points[key].point_y);
      LOG_INFO("keypoints[%d].score = %f", key, key_points[key].score);
      LOG_INFO("keypoints[%d].valid = %d", key, key_points[key].valid);
    }
#endif  // POSENET_DBGOUT_

    float score_average =
        get_instance_score(posenet_info, pose_result, key_points, face_num);
    face_num += regist_detected_pose(posenet_info, pose_result, key_points,
                                     score_average);

    list_index += 1;
  }

#if POSENET_DBGOUT_
  // for debug
  for (auto itr = pose_result.begin(); itr != pose_result.end(); itr++) {
    pose_t pose_data = *itr;
    LOG_INFO("pose_score  = %f", pose_data.pose_score);
    for (int key = 0; key < eKeyPoseNum; key++) {
      LOG_INFO("x____[%d] = %f", key, pose_data.keypoint[key].x);
      LOG_INFO("y____[%d] = %f", key, pose_data.keypoint[key].y);
      LOG_INFO("score[%d] = %f", key, pose_data.keypoint[key].score);
    }
  }
  LOG_INFO("face_num = %d", face_num);
#endif  // POSENET_DBGOUT_
  return face_num;
}

/**
 * @brief PoseNetPostPostPackFaceResult
 */
static void PoseNetPostPostPackFaceResult(
    PoseNetDecordInfo *posenet_info, std::vector<pose_t> &pose_result,
    std::vector<FaceDetectionResult> &vec_face_result, const int face_num) {
  float base_width = (float)posenet_info->base_image_w;
  float base_height = (float)posenet_info->base_image_h;

  int key_loop_num = std::min((eKeyRightEar + 1), (int)eKeyPoseNum);

  // assert (face_num <= FACE_DETECT_MAX_NUM)
  for (int num_faces = 0; num_faces < face_num; num_faces++) {
    FaceDetectionResult face_result;
    pose_t *pose_data = &pose_result[num_faces];
    float x_average = 0.0f;
    float y_average = 0.0f;
    float face_size = 0.0f;
    float face_dist_x_R, face_dist_x_L, face_dist_y_R, face_dist_y_L;
    face_result.id = static_cast<unsigned int>(num_faces);
    face_result.faceScore = pose_data->pose_score;

    for (int key = 0; key < key_loop_num; key++) {
      face_result.faceScore =
          std::max(face_result.faceScore, pose_data->keypoint[key].score);
      x_average += pose_data->keypoint[key].x;
      y_average += pose_data->keypoint[key].y;
    }
    x_average = x_average / key_loop_num;
    y_average = y_average / key_loop_num;

    face_dist_x_R = (x_average - pose_data->keypoint[eKeyRightEar].x);
    face_dist_y_R = (y_average - pose_data->keypoint[eKeyRightEar].y);
    face_dist_x_L = (x_average - pose_data->keypoint[eKeyLeftEar].x);
    face_dist_y_L = (y_average - pose_data->keypoint[eKeyLeftEar].y);
    face_size += sqrtf((face_dist_x_R * face_dist_x_R) +
                       (face_dist_y_R * face_dist_y_R));
    face_size += sqrtf((face_dist_x_L * face_dist_x_L) +
                       (face_dist_y_L * face_dist_y_L));
    face_size *= base_width * 1.5;

    face_result.faceW = face_size;
    face_result.faceH = face_size;
    face_result.faceX = (x_average * base_width) - (face_size / 2);
    face_result.faceY = (y_average * base_height) - (face_size / 2);

    face_result.faceW = std::roundf(face_result.faceW);
    face_result.faceH = std::roundf(face_result.faceH);

    face_result.faceX = std::roundf(face_result.faceX);
    face_result.faceY = std::roundf(face_result.faceY);
    face_result.faceX = CLAMP(face_result.faceX, 0.0f, (float)(base_width - 1));
    face_result.faceY =
        CLAMP(face_result.faceY, 0.0f, (float)(base_height - 1));
    vec_face_result.push_back(face_result);

#if POSENET_DBGOUT_
    {
      LOG_INFO("face_result[%d].id = %d", num_faces, face_result.id);
      LOG_INFO("face_result[%d].faceScore = %f", num_faces,
               face_result.faceScore);
      LOG_INFO("face_result[%d].faceX = %f", num_faces, face_result.faceX);
      LOG_INFO("face_result[%d].faceY = %f", num_faces, face_result.faceY);
      LOG_INFO("face_result[%d].faceW = %f", num_faces, face_result.faceW);
      LOG_INFO("face_result[%d].faceH = %f", num_faces, face_result.faceH);
    }
#endif  // POSENET_DBGOUT_
  }
}

/**
 * @brief calc_intersection_over_union
 */
static float calc_intersection_over_union(FaceDetectionResult &face0,
                                          FaceDetectionResult &face1) {
  float sx0 = face0.faceX;
  float sy0 = face0.faceY;
  float ex0 = face0.faceX + face0.faceW;
  float ey0 = face0.faceY + face0.faceH;
  float sx1 = face1.faceX;
  float sy1 = face1.faceY;
  float ex1 = face1.faceX + face1.faceW;
  float ey1 = face1.faceY + face1.faceH;

  float xmin0 = std::min(sx0, ex0);
  float ymin0 = std::min(sy0, ey0);
  float xmax0 = std::max(sx0, ex0);
  float ymax0 = std::max(sy0, ey0);
  float xmin1 = std::min(sx1, ex1);
  float ymin1 = std::min(sy1, ey1);
  float xmax1 = std::max(sx1, ex1);
  float ymax1 = std::max(sy1, ey1);

  float area0 = (ymax0 - ymin0) * (xmax0 - xmin0);
  float area1 = (ymax1 - ymin1) * (xmax1 - xmin1);
  if (area0 <= 0 || area1 <= 0) return 0.0f;

  float intersect_xmin = std::max(xmin0, xmin1);
  float intersect_ymin = std::max(ymin0, ymin1);
  float intersect_xmax = std::min(xmax0, xmax1);
  float intersect_ymax = std::min(ymax0, ymax1);

  float intersect_area = std::max(intersect_ymax - intersect_ymin, 0.0f) *
                         std::max(intersect_xmax - intersect_xmin, 0.0f);

  return intersect_area / (area0 + area1 - intersect_area);
}

/**
 * @brief compare_face_score
 */
static bool compare_face_score(FaceDetectionResult &v1,
                               FaceDetectionResult &v2) {
  if (v1.faceScore > v2.faceScore)
    return true;
  else
    return false;
}

/**
 * @brief PoseNetPostNonMaxSuppression
 */
static int PoseNetPostNonMaxSuppression(
    std::vector<FaceDetectionResult> &face_list,
    std::vector<FaceDetectionResult> &face_sel_list,
    std::vector<pose_t> &pose_list, std::vector<pose_t> &pose_sel_list,
    float iou_thresh) {
  std::sort(face_list.begin(), face_list.end(), compare_face_score);

  for (auto itr = face_list.begin(); itr != face_list.end(); itr++) {
    FaceDetectionResult face_candidate = *itr;

    int ignore_candidate = false;
    for (auto itr_sel = face_sel_list.rbegin(); itr_sel != face_sel_list.rend();
         itr_sel++) {
      FaceDetectionResult face_sel = *itr_sel;

      float iou = calc_intersection_over_union(face_candidate, face_sel);
      if (iou >= iou_thresh) {
        ignore_candidate = true;
        break;
      }
    }

    if (!ignore_candidate) {
      // clustering pose
      pose_sel_list.push_back(pose_list[face_candidate.id]);

      face_candidate.id = face_sel_list.size();
      face_sel_list.push_back(face_candidate);
#if POSENET_DBGOUT_
      LOG_INFO("clustering_face_result[%ld].id = %d", face_sel_list.size() - 1,
               face_candidate.id);
      LOG_INFO("clustering_face_result[%ld].faceScore = %f",
               face_sel_list.size() - 1, face_candidate.faceScore);
      LOG_INFO("clustering_face_result[%ld].faceX = %f",
               face_sel_list.size() - 1, face_candidate.faceX);
      LOG_INFO("clustering_face_result[%ld].faceY = %f",
               face_sel_list.size() - 1, face_candidate.faceY);
      LOG_INFO("clustering_face_result[%ld].faceW = %f",
               face_sel_list.size() - 1, face_candidate.faceW);
      LOG_INFO("clustering_face_result[%ld].faceH = %f",
               face_sel_list.size() - 1, face_candidate.faceH);
#endif  // POSENET_DBGOUT_
        //  assert (face_sel_list.size() >= FACE_DETECT_MAX_NUM)
    }
  }

  return face_sel_list.size();
}

/**
 * @brief compare_pose_score
 */
static bool compare_pose_score(pose_t &a, pose_t &b) {
  if (a.pose_score > b.pose_score) {
    return true;
  } else {
    return false;
  }
}
