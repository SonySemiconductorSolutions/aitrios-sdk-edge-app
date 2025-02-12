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

#ifndef _AITRIOS_SENSOR_H_
#define _AITRIOS_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * @def AITRIOS_SENSOR_STREAM_KEY_DEFAULT
 * @brief Key of inference stream
 * @details The camera image is inferred using the inference model, and
 * that output is the inference stream.
 */
#define AITRIOS_SENSOR_STREAM_KEY_DEFAULT "inference_stream"

/**
 * @def AITRIOS_SENSOR_STREAM_TYPE_INFERENCE_STREAM
 * @brief Type of inference stream
 * @details The camera image is inferred using the inference model, and
 * that output is the inference stream.
 */
#define AITRIOS_SENSOR_STREAM_TYPE_INFERENCE_STREAM "inference"

/**
 * @def AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT
 * @brief Channel ID for inference output
 * @details Used as the ID to get stream whose raw data type is
 * AITRIOS_SENSOR_RAW_DATA_TYPE_INFERENCE
 */
#define AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_OUTPUT (0x00000000)

/**
 * @def AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE
 * @brief Channel ID for inference input
 * @details Used as the ID to get stream whose raw data type is
 * AITRIOS_SENSOR_RAW_DATA_TYPE_IMAGE
 */
#define AITRIOS_SENSOR_CHANNEL_ID_INFERENCE_INPUT_IMAGE (0x00000001)

/**
 * @def AITRIOS_SENSOR_RAW_DATA_TYPE_INFERENCE
 * @brief Raw data type for inference output
 * @details String to represent raw data type of inference output
 */
#define AITRIOS_SENSOR_RAW_DATA_TYPE_INFERENCE "inference_data"

/**
 * @def AITRIOS_SENSOR_RAW_DATA_TYPE_IMAGE
 * @brief Raw data type for inference input
 * @details String to represent raw data type of inference input
 */
#define AITRIOS_SENSOR_RAW_DATA_TYPE_IMAGE "image_data"

/**
 * @def AITRIOS_SENSOR_INFERENCE_POST_PROCESS_PARAM_SIZE
 * @brief Size of param member in EdgeAppLibSensorPostProcessParameterProperty
 * @details Size in bytes
 */
#define AITRIOS_SENSOR_INFERENCE_POST_PROCESS_PARAM_SIZE (256)

/**
 * @def AITRIOS_SENSOR_CHANNEL_LIST_MAX
 * @brief Maximum number of supported channels
 * @details Number of channels
 */
#define AITRIOS_SENSOR_CHANNEL_LIST_MAX (8)

/**
 * @def AITRIOS_SENSOR_PIXEL_FORMAT_LENGTH
 * @brief Length of the pixel format string in EdgeAppLibSensorImageProperty
 * @details Length of the pixel format string
 */
#define AITRIOS_SENSOR_PIXEL_FORMAT_LENGTH (64)

/**
 * @def AITRIOS_SENSOR_RAWDATA_TYPE_LENGTH
 * @brief Length of the raw data type string in EdgeAppLibSensorChannelInfo
 * @details Length of the raw data type string
 */
#define AITRIOS_SENSOR_RAWDATA_TYPE_LENGTH (16)

/**
 * @def AITRIOS_SENSOR_CHANNEL_DESCRIPTION_LENGTH
 * @brief Length of description string of channel in EdgeAppLibSensorChannelInfo
 * @details Length of description string of channel
 */
#define AITRIOS_SENSOR_CHANNEL_DESCRIPTION_LENGTH (32)

/** Maximum number of temperature list. */
/**
 * @def AITRIOS_SENSOR_TEMPERATURE_LIST_MAX
 * @brief Maximum number of temperature list in
 * EdgeAppLibSensorTemperatureEnableProperty
 * @details Maximum number of temperature list
 */
#define AITRIOS_SENSOR_TEMPERATURE_LIST_MAX (10)

/**
 * @def AITRIOS_SENSOR_LATENCY_POINTS_MAX
 * @brief Maximum number of latency points
 * @details Number of latency points captured by sensor
 */
#define AITRIOS_SENSOR_LATENCY_POINTS_MAX (8)
/**
 * @enum EdgeAppLibSensorErrorLevel
 * @brief Level of error
 * @details Defines the level of error occurred in EdgeAppLibSensor
 */
enum EdgeAppLibSensorErrorLevel {
  AITRIOS_SENSOR_LEVEL_UNDEFINED = 0,
  AITRIOS_SENSOR_LEVEL_FAIL,
  AITRIOS_SENSOR_LEVEL_FATAL,
};

/**
 * @enum EdgeAppLibSensorErrorCause
 * @brief Cause of error
 * @details Defines the cause of error occurred in EdgeAppLibSensor
 */
enum EdgeAppLibSensorErrorCause {
  AITRIOS_SENSOR_ERROR_NONE = 0,
  AITRIOS_SENSOR_ERROR_NOT_FOUND,
  AITRIOS_SENSOR_ERROR_INVALID_ARGUMENT,
  AITRIOS_SENSOR_ERROR_RESOURCE_EXHAUSTED,
  AITRIOS_SENSOR_ERROR_PERMISSION_DENIED,
  AITRIOS_SENSOR_ERROR_BUSY,
  AITRIOS_SENSOR_ERROR_TIMEOUT,
  AITRIOS_SENSOR_ERROR_CANCELLED,
  AITRIOS_SENSOR_ERROR_ABORTED,
  AITRIOS_SENSOR_ERROR_ALREADY_EXISTS,
  AITRIOS_SENSOR_ERROR_INVALID_OPERATION,
  AITRIOS_SENSOR_ERROR_OUT_OF_RANGE,
  AITRIOS_SENSOR_ERROR_DATA_LOSS,
  AITRIOS_SENSOR_ERROR_HARDWARE_ERROR,
  AITRIOS_SENSOR_ERROR_NOT_SUPPORTED,
  AITRIOS_SENSOR_ERROR_UNKNOWN,
  AITRIOS_SENSOR_ERROR_INVALID_CAMERA_OPERATION_PARAMETER,
};

/**
 * @enum EdgeAppLibSensorStatusParam
 * @brief Flag to switch the kind of error text
 * @details 1st argument of EdgeAppLibSensorGetLastErrorString
 */
enum EdgeAppLibSensorStatusParam {
  AITRIOS_SENSOR_STATUS_PARAM_MESSAGE,
  AITRIOS_SENSOR_STATUS_PARAM_BLOCK,
  AITRIOS_SENSOR_STATUS_PARAM_TRACE,
};

/**
 * @typedef EdgeAppLibSensorHandle
 * @brief Common type for EdgeAppLibSensor handles
 * @details All EdgeAppLibSensor handles have the same type
 */
typedef uint64_t EdgeAppLibSensorHandle;

/**
 * @typedef EdgeAppLibSensorCore
 * @brief Handle of EdgeAppLibSensor core object
 * @details Used to open/close stream
 */
typedef EdgeAppLibSensorHandle EdgeAppLibSensorCore;

/**
 * @typedef EdgeAppLibSensorStream
 * @brief Handle of EdgeAppLibSensor stream object
 * @details Used to start/stop stream and access to frame/property
 */
typedef EdgeAppLibSensorHandle EdgeAppLibSensorStream;

/**
 * @typedef EdgeAppLibSensorFrame
 * @brief Handle of EdgeAppLibSensor frame object
 * @details Used to specify frame
 */
typedef EdgeAppLibSensorHandle EdgeAppLibSensorFrame;

/**
 * @typedef EdgeAppLibSensorChannel
 * @brief Handle of EdgeAppLibSensor channel object
 * @details Used to specify channel
 */
typedef EdgeAppLibSensorHandle EdgeAppLibSensorChannel;

/**
 * @struct EdgeAppLibSensorRawData
 * @brief The structure of raw data
 * @details Output from EdgeAppLibSensor channel object
 */
struct EdgeAppLibSensorRawData {
  void *address;      /**< virtual address */
  size_t size;        /**< data size */
  char *type;         /**< data type*/
  uint64_t timestamp; /**< nanoseconds timestamp captured by the device */
};

typedef struct {
  uint64_t points[AITRIOS_SENSOR_LATENCY_POINTS_MAX];
} EdgeAppLibLatencyTimestamps;

/**
 * @struct EdgeAppLibSensorStatus
 * @brief The structure of error information
 * @details Error from EdgeAppLibSensor API
 */
struct EdgeAppLibSensorStatus {
  enum EdgeAppLibSensorErrorLevel level; /* level of the error occurred */
  enum EdgeAppLibSensorErrorCause cause; /* cause of the error occurred */
  const char *message;                   /* error message */
  /* -------------------------------------------------------- */
  /* Note: message : decode specification                     */
  /*   all 10 digits : exam...0x0123456789                    */
  /*   01   : FileID.                                         */
  /*   2345 : File Line Number                                */
  /*   67   : ret (signed)                                    */
  /*   89   : errno                                           */
  /* -------------------------------------------------------- */
  const char *block; /* internal block from where the error has occurred */
};

/*
 * ### Definition of Properties ###
 */

/* == ImageCropProperty == */
/**
 * @def AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorImageCropProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY "image_crop_property"
/**
 * @struct EdgeAppLibSensorImageCropProperty
 * @brief Value of AITRIOS_SENSOR_IMAGE_CROP_PROPERTY_KEY
 * @details Property is the region to crop camera captured image for inference
 */
struct EdgeAppLibSensorImageCropProperty {
  uint32_t left;   /**< start xpoint */
  uint32_t top;    /**< start ypoint */
  uint32_t width;  /**< width */
  uint32_t height; /**< height */
};

/* == AIModelBundleIdProperty == */
/**
 * @def AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorAiModelBundleIdProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY \
  "ai_model_bundle_id_property"
/**
 * @struct EdgeAppLibSensorAiModelBundleIdProperty
 * @brief Value of AITRIOS_SENSOR_AI_MODEL_BUNDLE_ID_PROPERTY_KEY
 * @details ID of AI model bundle. Setting this ID,
 * AI model bundle can be switched only when stream is ready.
 */
#define AI_MODEL_BUNDLE_ID_SIZE 128
struct EdgeAppLibSensorAiModelBundleIdProperty {
  char ai_model_bundle_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
};

/* == ImageRotationProperty == */
/**
 * @def AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorImageRotationProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY "image_rotation_property"
/**
 * @enum EdgeAppLibSensorRotationAngle
 * @brief Supported angle of rotation
 * @details enum of rotation_angle member in
 * EdgeAppLibSensorImageRotationProperty
 */
enum EdgeAppLibSensorRotationAngle {
  AITRIOS_SENSOR_ROTATION_ANGLE_0_DEG,
  AITRIOS_SENSOR_ROTATION_ANGLE_90_DEG,
  AITRIOS_SENSOR_ROTATION_ANGLE_180_DEG,
  AITRIOS_SENSOR_ROTATION_ANGLE_270_DEG,
};
/**
 * @struct EdgeAppLibSensorImageRotationProperty
 * @brief Value of AITRIOS_SENSOR_IMAGE_ROTATION_PROPERTY_KEY
 * @details Setting of clockwise rotation to input image for inference
 */
struct EdgeAppLibSensorImageRotationProperty {
  enum EdgeAppLibSensorRotationAngle rotation_angle;
};

/* == CameraFrameRateProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraFrameRateProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY \
  "camera_frame_rate_property"
/**
 * @struct EdgeAppLibSensorCameraFrameRateProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_FRAME_RATE_PROPERTY_KEY
 * @details Frame rate of camera
   @verbatim
   e.g.)
   +-------------+------+-------+
   | frame rate  | num  | denom |
   +-------------+------+-------+
   |  0.99 [fps] |  99  |  100  |
   +-------------+------+-------+
   | 29.97 [fps] | 2997 |  100  |
   +-------------+------+-------+
   @endverbatim
 */
struct EdgeAppLibSensorCameraFrameRateProperty {
  uint32_t num;
  uint32_t denom;
};

/* == CameraImageSizeProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraImageSizeProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY \
  "camera_image_size_property"
/**
 * @enum EdgeAppLibSensorCameraScalingPolicy
 * @brief Policy of scaling
 * @details enum of scaling_policy member in
 * EdgeAppLibSensorCameraImageSizeProperty
 */
enum EdgeAppLibSensorCameraScalingPolicy {
  AITRIOS_SENSOR_CAMERA_SCALING_POLICY_SENSITIVITY = 1,
  AITRIOS_SENSOR_CAMERA_SCALING_POLICY_RESOLUTION = 2,
};
/**
 * @struct EdgeAppLibSensorCameraImageSizeProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_IMAGE_SIZE_PROPERTY_KEY
 * @details Image size of camera captured image. EdgeAppLibSensorStart can fail
 * if bad combination with other properties occurred.
 */
struct EdgeAppLibSensorCameraImageSizeProperty {
  uint32_t width;
  uint32_t height;
  enum EdgeAppLibSensorCameraScalingPolicy scaling_policy;
};

/* == CameraImageFlipProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraImageFlipProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY \
  "camera_image_flip_property"
/**
 * @struct EdgeAppLibSensorCameraImageFlipProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_IMAGE_FLIP_PROPERTY_KEY
 * @details Flip setting of camera captured image
 */
struct EdgeAppLibSensorCameraImageFlipProperty {
  bool flip_horizontal;
  bool flip_vertical;
};

/* == CameraDigitalZoomProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraDigitalZoomProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY \
  "camera_digital_zoom_property"
/**
 * @struct EdgeAppLibSensorCameraDigitalZoomProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY
 * @details Zoom ratio of camera captured image.
 * EdgeAppLibSensorStreamSetProperty fails if the value is less than 1.
 * EdgeAppLibSensorStart can fail if bad combination with other properties
 * occurred.
 */
struct EdgeAppLibSensorCameraDigitalZoomProperty {
  float magnification;
};

/* == CameraExposureModeProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraExposureModeProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY \
  "camera_exposure_mode_property"
/**
 * @enum EdgeAppLibSensorCameraExposureMode
 * @brief Exposure mode for camera
 * @details enum of mode member in EdgeAppLibSensorCameraExposureModeProperty
 */
enum EdgeAppLibSensorCameraExposureMode {
  AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO = 0,
  AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_MANUAL = 3,
};
/**
 * @struct EdgeAppLibSensorCameraExposureModeProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_PROPERTY_KEY
 * @details Exposure mode setting for camera.
 */
struct EdgeAppLibSensorCameraExposureModeProperty {
  enum EdgeAppLibSensorCameraExposureMode mode;
};

/* == CameraAutoExposureProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraAutoExposureProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY \
  "camera_auto_exposure_property"
/**
 * @struct EdgeAppLibSensorCameraAutoExposureProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY
 * @details Parameters when EdgeAppLibSensorCameraExposureModeProperty is
 * AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO. There is a possibility that
 * the value may be rounded down depend on H/W.
 */
struct EdgeAppLibSensorCameraAutoExposureProperty {
  uint32_t max_exposure_time;
  uint32_t min_exposure_time;
  float max_gain;
  uint32_t convergence_speed;
};

/* == CameraAutoExposureMeteringProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraAutoExposureMeteringProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY \
  "camera_auto_exposure_metering_property"

/**
 * @enum EdgeAppLibSensorCameraAutoExposureMeteringMode
 * @brief Metering mode
 * @details enum of metering mode in
 * EdgeAppLibSensorCameraAutoExposureMeteringProperty
 * - FULL_SCREEN: Detects the average area of the camera image.
 *   The value of the window member is ignored.
 * - USER_WINDOW: The range specified by the window member is detected.
 */
enum EdgeAppLibSensorCameraAutoExposureMeteringMode {
  AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_MODE_FULL_SCREEN,
  AITRIOS_SENSOR_CAMERA_AUTO_EXPOSURE_METERING_MODE_USER_WINDOW
};

/**
 * @struct EdgeAppLibSensorCameraAutoExposureMeteringProperty
 * @brief Value of EdgeAppLibSensorCameraAutoExposureMeteringProperty
 * @details Setting of detection in Auto Exposure.
 * This property operates when EdgeAppLibSensorCameraExposureModeProperty is
 * AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO. The top, left, bottom and
 * right values specify the detection frame for metering modes that require the
 * user to set the range. The unit is pixels.
 */
struct EdgeAppLibSensorCameraAutoExposureMeteringProperty {
  EdgeAppLibSensorCameraAutoExposureMeteringMode mode; /**< metering mode. */
  uint32_t top;    /**< Y coordinate of top of detection frame. */
  uint32_t left;   /**< X coordinate of left part of detection frame. */
  uint32_t bottom; /**< Y coordinate of bottom part of detection frame. */
  uint32_t right;  /**< X coordinate of right part of detection frame. */
};

/* == CameraEvCompensationProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraEvCompensationProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY \
  "camera_ev_compensation_property"
/**
 * @struct EdgeAppLibSensorCameraEvCompensationProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_EV_COMPENSATION_PROPERTY_KEY
 * @details EV compensation when EdgeAppLibSensorCameraExposureModeProperty is
 * AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO. There is a possibility that
 * the value may be rounded toward zero depend on H/W. Type3 supports
 * from -2.0 to 2.0.
 */
struct EdgeAppLibSensorCameraEvCompensationProperty {
  float ev_compensation;
};

/* == CameraAntiFlickerModeProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraAntiFlickerModeProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY \
  "camera_anti_flicker_mode_property"
/**
 * @enum EdgeAppLibSensorCameraAntiFlickerMode
 * @brief Modes of Anti flicker
 * @details enum of anti_flicker_mode member in
 * EdgeAppLibSensorCameraAntiFlickerModeProperty
 */
enum EdgeAppLibSensorCameraAntiFlickerMode {
  AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_OFF,
  AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_AUTO,
  AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_FORCE_50HZ,
  AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_FORCE_60HZ,
};
/**
 * @struct EdgeAppLibSensorCameraAntiFlickerModeProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY
 * @details Anti flicker setting when EdgeAppLibSensorCameraExposureModeProperty
 * is AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_AUTO. Type3 supports all 4 mode.
 */
struct EdgeAppLibSensorCameraAntiFlickerModeProperty {
  enum EdgeAppLibSensorCameraAntiFlickerMode anti_flicker_mode;
};

/* == CameraManualExposureProperty == */
/**
 * @def AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCameraManualExposureProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY \
  "camera_manual_exposure_property"
/**
 * @struct EdgeAppLibSensorCameraManualExposureProperty
 * @brief Value of AITRIOS_SENSOR_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY
 * @details Parameters when EdgeAppLibSensorCameraExposureModeProperty is
 * AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_MANUAL. There is a possibility that
 * the value may be rounded down depend on H/W.
 */
struct EdgeAppLibSensorCameraManualExposureProperty {
  uint32_t exposure_time;
  float gain;
};

/* == WhiteBalanceModeProperty == */
/**
 * @def AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorWhiteBalanceModeProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY \
  "white_balance_mode_property"
/**
 * @enum EdgeAppLibSensorInferenceWhiteBalanceMode
 * @brief Modes of white balance to input image of inference
 * @details enum of mode member in EdgeAppLibSensorWhiteBalanceModeProperty
 */
enum EdgeAppLibSensorInferenceWhiteBalanceMode {
  AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_AUTO = 0,
  AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_MANUAL_PRESET = 1,
};
/**
 * @struct EdgeAppLibSensorWhiteBalanceModeProperty
 * @brief Value of AITRIOS_SENSOR_WHITE_BALANCE_MODE_PROPERTY_KEY
 * @details White balance mode that has influence only on input image of
 * inference.
 */
struct EdgeAppLibSensorWhiteBalanceModeProperty {
  enum EdgeAppLibSensorInferenceWhiteBalanceMode mode;
};

/* == AutoWhiteBalanceProperty == */
/**
 * @def AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorAutoWhiteBalanceProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY \
  "auto_white_balance_property"
/**
 * @struct EdgeAppLibSensorAutoWhiteBalanceProperty
 * @brief Value of AITRIOS_SENSOR_AUTO_WHITE_BALANCE_PROPERTY_KEY
 * @details Convergence speed when EdgeAppLibSensorWhiteBalanceModeProperty is
 * AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_AUTO. There is a possibility that
 * the value may be rounded down depend on H/W.
 */
struct EdgeAppLibSensorAutoWhiteBalanceProperty {
  uint32_t convergence_speed;
};

/* == ManualWhiteBalancePresetProperty == */
/**
 * @def AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorManualWhiteBalancePresetProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY \
  "manual_white_balance_preset_property"
/**
 * @struct EdgeAppLibSensorManualWhiteBalancePresetProperty
 * @brief Value of AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY
 * @details Color temperature when EdgeAppLibSensorWhiteBalanceModeProperty is
 * AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_MANUAL_PRESET. There is a
 * possibility that closest value may be set depend on H/W. Available values
 * on type3 are 3200[K], 4300[K], 5600[K] and 6500[K].
 */
struct EdgeAppLibSensorManualWhiteBalancePresetProperty {
  uint32_t color_temperature;
};

/* == PostProcessAvailableProperty == */
/**
 * @def AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorPostProcessAvailableProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY \
  "post_process_available_property"
/**
 * @struct EdgeAppLibSensorPostProcessAvailableProperty
 * @brief Value of AITRIOS_SENSOR_POST_PROCESS_AVAILABLE_PROPERTY_KEY
 * @details The exist flag of post process functionality
 */
struct EdgeAppLibSensorPostProcessAvailableProperty {
  bool is_available;
};

/* == PostProcessParameterProperty == */
/**
 * @def AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorPostProcessParameterProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY \
  "post_process_parameter_property"
/**
 * @struct EdgeAppLibSensorPostProcessParameterProperty
 * @brief Value of AITRIOS_SENSOR_POST_PROCESS_PARAMETER_PROPERTY_KEY
 * @details The parameter to be passed to post process
 * @details Recommended sequence)
 * - 1. Set AI model with post process by using
 * EdgeAppLibSensorAiModelBundleIdProperty and
 * EdgeAppLibSensorAiModelIndexProperty
 * - 2. Check if post process is available or not by using
 * EdgeAppLibSensorPostProcessAvailableProperty
 * - 3. If available, pass parameter by using
 * EdgeAppLibSensorPostProcessParameterProperty
 * - 4. Start steam by using EdgeAppLibSensorStart
 */
struct EdgeAppLibSensorPostProcessParameterProperty {
  uint8_t param[AITRIOS_SENSOR_INFERENCE_POST_PROCESS_PARAM_SIZE];
};

/* == InputDataTypeProperty == */
/**
 * @def AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorInputDataTypeProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY "input_data_type_property"
/**
 * @struct EdgeAppLibSensorInputDataTypeProperty
 * @brief Value of AITRIOS_SENSOR_INPUT_DATA_TYPE_PROPERTY_KEY
 * @details Defines the channels enabled in a stream.
 */
struct EdgeAppLibSensorInputDataTypeProperty {
  uint32_t count;
  uint32_t channels[AITRIOS_SENSOR_CHANNEL_LIST_MAX];
};

/* == ImageProperty == */
/**
 * @def AITRIOS_SENSOR_IMAGE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorImageProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_IMAGE_PROPERTY_KEY "image_property"
/**
 * @def AITRIOS_SENSOR_PIXEL_FORMAT_RGB24
 * @brief Pixel formats/Packed RGB
 * @details RGB 888
 */
#define AITRIOS_SENSOR_PIXEL_FORMAT_RGB24 "image_rgb24"
/**
 * @def AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR
 * @brief Pixel formats/Planar RGB
 * @details RGB 8-bit
 */
#define AITRIOS_SENSOR_PIXEL_FORMAT_RGB8_PLANAR "image_rgb8_planar"
/**
 * @struct EdgeAppLibSensorImageProperty
 * @brief Value of AITRIOS_SENSOR_IMAGE_PROPERTY_KEY
 * @details Defines the properties of Raw data of Image and Depth data.
 */
struct EdgeAppLibSensorImageProperty {
  uint32_t width;        /**< Image width. */
  uint32_t height;       /**< Image height. */
  uint32_t stride_bytes; /**< Image stride. */
  /** The format of a pixel. */
  char pixel_format[AITRIOS_SENSOR_PIXEL_FORMAT_LENGTH];
};

/* == CurrentFrameNumProperty == */
/**
 * @def AITRIOS_SENSOR_CURRENT_FRAME_NUM_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorCurrentFrameNumProperty
 * @details String of key to get property
 */
#define AITRIOS_SENSOR_CURRENT_FRAME_NUM_PROPERTY_KEY \
  "current_frame_num_property"
/**
 * @struct EdgeAppLibSensorCurrentFrameNumProperty
 * @brief Value of AITRIOS_SENSOR_CURRENT_FRAME_NUM_PROPERTY_KEY
 * @details Defines the properties of the current buffering frames.
 */
struct EdgeAppLibSensorCurrentFrameNumProperty {
  int32_t arrived_number;  /**< Arrived number. */
  int32_t received_number; /**< Received number. */
};

/* == ChannelInfoProperty == */
/**
 * @struct EdgeAppLibSensorChannelInfo
 * @brief The structure of channel information
 * @details OChannel information
 */
struct EdgeAppLibSensorChannelInfo {
  /** Channel ID. */
  uint32_t channel_id;
  /** Type of raw data. */
  char raw_data_type[AITRIOS_SENSOR_RAWDATA_TYPE_LENGTH];
  /** Channel description. */
  char description[AITRIOS_SENSOR_CHANNEL_DESCRIPTION_LENGTH];
};
/**
 * @def AITRIOS_SENSOR_CHANNEL_INFO_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorChannelInfoProperty
 * @details String of key to get property
 */
#define AITRIOS_SENSOR_CHANNEL_INFO_PROPERTY_KEY "channel_info_property"
/**
 * @struct EdgeAppLibSensorChannelInfoProperty
 * @brief Value of AITRIOS_SENSOR_CHANNEL_INFO_PROPERTY_KEY
 * @details Defines the properties of the channel information
 */
struct EdgeAppLibSensorChannelInfoProperty {
  /** Count of the array. */
  uint32_t count;
  /** Array of the channel information. */
  struct EdgeAppLibSensorChannelInfo channels[AITRIOS_SENSOR_CHANNEL_LIST_MAX];
};

/* == InferenceProperty == */
/**
 * @def AITRIOS_SENSOR_INFERENCE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorInferenceProperty
 * @details String of key to get property
 */
#define AITRIOS_SENSOR_INFERENCE_PROPERTY_KEY "inference_property"
/**
 * @def AITRIOS_SENSOR_INFERENCE_DATA_TYPE_LENGTH
 * @brief Length of the inference data type string in
 * EdgeAppLibSensorInferenceProperty
 * @details Length of the inference data type string
 */
#define AITRIOS_SENSOR_INFERENCE_DATA_TYPE_LENGTH 64
/**
 * @struct EdgeAppLibSensorInferenceProperty
 * @brief Value of AITRIOS_SENSOR_INFERENCE_PROPERTY_KEY
 * @details Defines the properties of the inference properety
 */
struct EdgeAppLibSensorInferenceProperty {
  char data_type[AITRIOS_SENSOR_INFERENCE_DATA_TYPE_LENGTH];
};

/* == TensorShapesProperty == */
/**
 * @def AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorTensorShapesProperty
 * @details String of key to get property
 */
#define AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY "tensor_shapes_property"
/**
 * @def AITRIOS_SENSOR_SHAPES_ARRAY_LENGTH
 * @brief Length of the shapes array string in
 * EdgeAppLibSensorTensorShapesProperty
 * @details Length of the shapes array string
 */
#define AITRIOS_SENSOR_SHAPES_ARRAY_LENGTH 192
/**
 * @struct EdgeAppLibSensorTensorShapesProperty
 * @brief Value of AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY
 * @details Defines the properties of the tensor shapes
 */
struct EdgeAppLibSensorTensorShapesProperty {
  uint32_t tensor_count;
  uint32_t shapes_array[AITRIOS_SENSOR_SHAPES_ARRAY_LENGTH];
};

/* == InfoStringProperty == */
/**
 * @def AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorInfoStringProperty
 * @details String of key to get property
 */
#define AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY "info_string_property"
/**
 * @def AITRIOS_SENSOR_INFO_STRING_LENGTH
 * @brief Length of the info string in
 * EdgeAppLibSensorInfoStringProperty
 * @details Length of the info string
 */
#define AITRIOS_SENSOR_INFO_STRING_LENGTH 128
/**
 * @def AITRIOS_SENSOR_INFO_STRING_SENSOR_NAME
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for AITRIOS hardware info
 */
#define AITRIOS_SENSOR_INFO_STRING_SENSOR_NAME 0x00000000
/**
 * @def AITRIOS_SENSOR_INFO_STRING_SENSOR_ID
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for AITRIOS hardware info
 */
#define AITRIOS_SENSOR_INFO_STRING_SENSOR_ID 0x00000001
/**
 * @def AITRIOS_SENSOR_INFO_STRING_KEY_GENERATION
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for AITRIOS hardware info
 */
#define AITRIOS_SENSOR_INFO_STRING_KEY_GENERATION 0x00000002
/**
 * @def AITRIOS_SENSOR_INFO_STRING_FIRMWARE_VERSION
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for AITRIOS sensor info
 */
#define AITRIOS_SENSOR_INFO_STRING_FIRMWARE_VERSION 0x00010000
/**
 * @def AITRIOS_SENSOR_INFO_STRING_LOADER_VERSION
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for AITRIOS sensor info
 */
#define AITRIOS_SENSOR_INFO_STRING_LOADER_VERSION 0x00010001
/**
 * @def AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for AITRIOS sensor info
 */
#define AITRIOS_SENSOR_INFO_STRING_AI_MODEL_VERSION 0x00010002
/**
 * @def AITRIOS_SENSOR_INFO_STRING_VENDOR_BASE
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for vendor extension
 */
#define AITRIOS_SENSOR_INFO_STRING_VENDOR_BASE 0x80000000
/**
 * @def AITRIOS_SENSOR_INFO_STRING_AIISP_DEVICE_ID
 * @brief Setting the category that member of
 * EdgeAppLibSensorInfoStringProperty
 * @details Setting the category for vendor extension
 */
#define AITRIOS_SENSOR_INFO_STRING_AIISP_DEVICE_ID 0x80000101
/**
 * @struct EdgeAppLibSensorInfoStringProperty
 * @brief Value of AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY
 * @details Defines the properties of the info string
 */
struct EdgeAppLibSensorInfoStringProperty {
  uint32_t category;
  char info[AITRIOS_SENSOR_INFO_STRING_LENGTH];
};

/* == TemperatureEnableProperty == */
/**
 * @struct EdgeAppLibSensorTemperatureEnable
 * @brief The structure of temperature enable information
 * @details Temperature enable information
 */
struct EdgeAppLibSensorTemperatureEnable {
  /** Sensor ID. */
  uint32_t sensor_id;
  /** Temperature data. */
  bool enable;
};
/**
 * @def AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorTemperatureEnableProperty
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY \
  "temperature_enable_property"
/**
 * @struct EdgeAppLibSensorTemperatureEnableProperty
 * @brief Value of AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY
 * @details Defines the properties of the temperature
 */
struct EdgeAppLibSensorTemperatureEnableProperty {
  /** Count of the array. */
  uint32_t count;
  /** Array of availablility for each temperature sensor. */
  struct EdgeAppLibSensorTemperatureEnable
      temperatures[AITRIOS_SENSOR_TEMPERATURE_LIST_MAX];
};

/* == FrameRateProperty == */
/**
 * @def AITRIOS_SENSOR_FRAME_RATE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorFrameRateProperty
 * @details String of key to get property
 */
#define AITRIOS_SENSOR_FRAME_RATE_PROPERTY_KEY "frame_rate_property"
/**
 * @struct EdgeAppLibSensorFrameRateProperty
 * @brief Value of AITRIOS_SENSOR_FRAME_RATE_PROPERTY_KEY
 * @details Defines the properties of the structure for setting frame rate.
 * Specify in the style of numerator / denominator.
 * ex) 60fps : num = 60, denom = 1
 */
struct EdgeAppLibSensorFrameRateProperty {
  uint32_t num;   /**< Framerate numerator. */
  uint32_t denom; /**< Framerate denominator. */
};

/* == RegisterAccess64Property == */
/**
 * @def AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorRegisterAccess64Property
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY \
  "register_access_64_property"
/**
 * @struct EdgeAppLibSensorRegisterAccess64Property
 * @brief Value of AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY
 * @details Defines the property of standard register 64 bit read/write access
 */
struct EdgeAppLibSensorRegisterAccess64Property {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint64_t data;    /**< Writing data or read data. */
};

/* == RegisterAccess32Property == */
/**
 * @def AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorRegisterAccess32Property
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY \
  "register_access_32_property"
/**
 * @struct EdgeAppLibSensorRegisterAccess32Property
 * @brief Value of AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY
 * @details Defines the property of standard register 32 bit read/write access
 */
struct EdgeAppLibSensorRegisterAccess32Property {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint32_t data;    /**< Writing data or read data. */
};

/* == RegisterAccess16Property == */
/**
 * @def AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorRegisterAccess16property
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY \
  "register_access_16_property"
/**
 * @struct EdgeAppLibSensorRegisterAccess16property
 * @brief Value of AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY
 * @details Defines the property of standard register 16 bit read/write access
 */
struct EdgeAppLibSensorRegisterAccess16Property {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint16_t data;    /**< Writing data or read data. */
};

/* == RegisterAccess8Property == */
/**
 * @def AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorRegisterAccess8Property
 * @details String of key to set/get property
 */
#define AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY \
  "register_access_8_property"
/**
 * @struct EdgeAppLibSensorRegisterAccess8Property
 * @brief Value of AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY
 * @details Defines the property of standard register 8 bit read/write access
 */
struct EdgeAppLibSensorRegisterAccess8Property {
  uint32_t id;      /**< Register ID. */
  uint64_t address; /**< Target address. */
  uint8_t data;     /**< Writing data or read data. */
};

#define AITRIOS_SENSOR_REGISTER_ACCESS_PROPERTY_KEY "register_access_property"
/**
 * @enum EdgeAppLibSensorRegisterBitLength
 * @brief Byte length of register
 * @details enum of bit_length member in
 * EdgeAppLibSensorRegisterAccessProperty
 */
enum EdgeAppLibSensorRegisterBitLength {
  AITRIOS_SENSOR_REGISTER_8BIT = 0,
  AITRIOS_SENSOR_REGISTER_16BIT = 1,
  AITRIOS_SENSOR_REGISTER_32BIT = 2,
  AITRIOS_SENSOR_REGISTER_64BIT = 3,
};

typedef union {
  uint8_t data8;
  uint16_t data16;
  uint32_t data32;
  uint64_t data64;
} RegisterData;

struct EdgeAppLibSensorRegisterAccessProperty {
  uint32_t id;       /**< Register ID. */
  uint64_t address;  /**< Target address. */
  RegisterData data; /**< Writing data or read data. */
  enum EdgeAppLibSensorRegisterBitLength bit_length;
};

/* == TemperatureProperty == */
/**
 * @def AITRIOS_SENSOR_TEMPERATURE_DESCRIPTION_LENGTH
 * @brief Length of the temperature description string in
 * EdgeAppLibSensorTemperatureInfo
 * @details Length of the temperature description string
 */
#define AITRIOS_SENSOR_TEMPERATURE_DESCRIPTION_LENGTH (32)
/**
 * @def AITRIOS_SENSOR_TEMPERATURE_LIST_MAX
 * @brief Maximum number of temperature list in
 * EdgeAppLibSensorTemperatureProperty
 * @details Maximum number of temperature list
 */
#define AITRIOS_SENSOR_TEMPERATURE_LIST_MAX (10)
/**
 * @brief Temperature information.
 * @see senscord::TemperatureInfo
 */
struct EdgeAppLibSensorTemperatureInfo {
  /** Sensor ID. */
  uint32_t sensor_id;
  /** Temperature data. */
  float temperature;
  /** Description of sensor. */
  char description[AITRIOS_SENSOR_TEMPERATURE_DESCRIPTION_LENGTH];
};
/**
 * @def AITRIOS_SENSOR_TEMPERATURE_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorTemperatureProperty
 * @details String of key to get property
 */
#define AITRIOS_SENSOR_TEMPERATURE_PROPERTY_KEY "temperature_property"
/**
 * @struct EdgeAppLibSensorTemperatureProperty
 * @brief Value of AITRIOS_SENSOR_TEMPERATURE_PROPERTY_KEY
 * @details Defines the properties for the temperature
 */
struct EdgeAppLibSensorTemperatureProperty {
  /** Count of the array. */
  uint32_t count;
  /** Array of information for each temperature sensor. */
  struct EdgeAppLibSensorTemperatureInfo
      temperatures[AITRIOS_SENSOR_TEMPERATURE_LIST_MAX];
};

/* == SubFrameProperty == */
/**
 * @def AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY
 * @brief Key of EdgeAppLibSensorSubFramePropery
 * @details Defines the properties for the sub frame
 */
#define AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY "sub_frame_property"
/**
 * @struct EdgeAppLibSensorSubFrameProperty
 * @brief Value of AITRIOS_SENSOR_SUB_FRAME_PROPERTY_KEY
 * @details Defines the properties for the bus frame
 */
struct EdgeAppLibSensorSubFrameProperty {
  /** Number of divisions of the Frame output */
  uint32_t current_num;
  /** Total number of divisions of the Frame output */
  uint32_t division_num;
};

#ifdef __cplusplus
}
#endif

namespace EdgeAppLib {

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize EdgeAppLibSensor
 * @param[out] core Handle of EdgeAppLibSensor core object
 * @return Zero for success or negative value for failure
 * @details Initialize EdgeAppLibSensor and make instance
 */
int32_t SensorCoreInit(EdgeAppLibSensorCore *core);

/**
 * @brief Terminate EdgeAppLibSensor
 * @param[in] core Handle of EdgeAppLibSensor core object
 * @return Zero for success or negative value for failure
 * @details Terminate EdgeAppLibSensor core
 */
int32_t SensorCoreExit(EdgeAppLibSensorCore core);

/**
 * @brief Open EdgeAppLibSensor stream
 * @param[in] core Handle of EdgeAppLibSensor core object
 * @param[in] stream_key Key string to open EdgeAppLibSensor stream
 * @param[out] stream Handle of newly opened EdgeAppLibSensor stream
 * @return Zero for success or negative value for failure
 * @details stream_key should be AITRIOS_SENSOR_STREAM_KEY
 */
int32_t SensorCoreOpenStream(EdgeAppLibSensorCore core, const char *stream_key,
                             EdgeAppLibSensorStream *stream);

/**
 * @brief Close EdgeAppLibSensor stream
 * @param[in] core Handle of EdgeAppLibSensor core object
 * @param[in] stream Handle of EdgeAppLibSensor stream to be closed
 * @return Zero for success or negative value for failure
 * @details Frames are also freed and this function does not return
 * until callback is done
 */
int32_t SensorCoreCloseStream(EdgeAppLibSensorCore core,
                              EdgeAppLibSensorStream stream);

/**
 * @brief Start EdgeAppLibSensor stream
 * @param[in] stream Handle opened by EdgeAppLibSensorCoreOpenStream
 * @return Zero for success or negative value for failure
 * @details Fail if already started or if bad combination of properties occurred
 */
int32_t SensorStart(EdgeAppLibSensorStream stream);

/**
 * @brief Stop EdgeAppLibSensor stream
 * @param[in] stream Handle to be stopped
 * @return Zero for success or negative value for failure
 * @details Stop capturing into frame buffer and this function does not return
 * until callback is done
 */
int32_t SensorStop(EdgeAppLibSensorStream stream);

/**
 * @brief Get EdgeAppLibSensor frame
 * @param[in] stream Handle of stream to get frame
 * @param[out] frame Handle of frame obtained
 * @param[in] timeout_msec timeout value
 * (0: timeout immediately, -1: not timeout infinitely)
 * @return Zero for success or negative value for failure
 * @details Return the oldest unobtained handle of frame
 */
int32_t SensorGetFrame(EdgeAppLibSensorStream stream,
                       EdgeAppLibSensorFrame *frame, int32_t timeout_msec);

/**
 * @brief Release EdgeAppLibSensor frame
 * @param[in] stream Handle of stream to release frame
 * @param[in] frame Handle of frame to be released
 * @return Zero for success or negative value for failure
 * @details Release frame identified by frame param
 */
int32_t SensorReleaseFrame(EdgeAppLibSensorStream stream,
                           EdgeAppLibSensorFrame frame);

/**
 * @brief Get property value from EdgeAppLibSensor stream
 * @param[in] stream Handle of stream to get property
 * @param[in] property_key Key of property
 * @param[out] value Pointer to memory area to store property value
 * @param[in] value_size Size of property value in bytes
 * @return Zero for success or negative value for failure
 * @details Memory area of value must be allocated by caller
 */
int32_t SensorStreamGetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, void *value,
                                size_t value_size);

/**
 * @brief Set property value to EdgeAppLibSensor stream
 * @param[in] stream Handle of stream to set property
 * @param[in] property_key Key of property
 * @param[in] value Pointer to memory area to store property value
 * @param[in] value_size Size of property value in bytes
 * @return Zero for success or negative value for failure
 * @details Memory area of value must be allocated by caller
 */
int32_t SensorStreamSetProperty(EdgeAppLibSensorStream stream,
                                const char *property_key, const void *value,
                                size_t value_size);

/**
 * @brief Get channel that is specified by Channel ID
 * @param[in] frame Handle of frame to get channel
 * @param[in] channel_id Channel ID of the channel to be gotten
 * @param[out] channel Pointer to obtained channel handle
 * @return Zero for success or negative value for failure
 * @details Channel handle is valid until EdgeAppLibSensorReleaseFrame is called
 */
int32_t SensorFrameGetChannelFromChannelId(EdgeAppLibSensorFrame frame,
                                           uint32_t channel_id,
                                           EdgeAppLibSensorChannel *channel);

/**
 * @brief Get sensing data (raw data information) from channel
 * @param[in] channel Handle of channel to get raw data
 * @param[out] raw_data Pointer of EdgeAppLibSensorRawData to store obtained raw
 * data
 * @return Zero for success or negative value for failure
 * @details This function can be called whenever channel is valid
 */
int32_t SensorChannelGetRawData(EdgeAppLibSensorChannel channel,
                                struct EdgeAppLibSensorRawData *raw_data);

/**
 * @brief Get property value from EdgeAppLibSensor channel
 * @param[in] channel Handle of channel to get property
 * @param[in] property_key Key of property
 * @param[out] value Pointer to memory area to store property value
 * @param[in] value_size Size of property value in bytes
 * @return Zero for success or negative value for failure
 * @details Memory area of value must be allocated by caller
   @verbatim
   Channel Properties
   +------------+--------------------------------------+
   | Channel ID | Property                             |
   +------------+--------------------------------------+
   | 0x00000000 | EdgeAppLibSensorAiModelBundleIdProperty |
   |            | EdgeAppLibSensorImageCropProperty       |
   +------------+--------------------------------------+
   @endverbatim
 */
int32_t SensorChannelGetProperty(EdgeAppLibSensorChannel channel,
                                 const char *property_key, void *value,
                                 size_t value_size);

/**
 * @brief Enable or disable a channel in EdgeAppLibSensorInputDataTypeProperty
 * @param[in] property Pointer to the EdgeAppLibSensorInputDataTypeProperty to
 * modify
 * @param[in] channel_id Channel ID of the channel to enable or disable
 * @param[in] enable Whether the channel should be enabled or disable
 * @return Zero for success or negative value for failure
 */
int32_t SensorInputDataTypeEnableChannel(
    EdgeAppLibSensorInputDataTypeProperty *property, uint32_t channel_id,
    bool enable);

/**
 * @brief Get level of last error occurred in EdgeAppLibSensor
 * @return One of EdgeAppLibSensorErrorLevel that indicates level of last error
 * @details There is no error information about EdgeAppLibSensorCoreInit and
 * EdgeAppLibSensorCoreExit. The error information is not updated if API is done
 * successfully.
 */
enum EdgeAppLibSensorErrorLevel SensorGetLastErrorLevel(void);

/**
 * @brief Get cause of last error occurred in EdgeAppLibSensor
 * @return One of EdgeAppLibSensorErrorCause that indicates cause of last error
 * @details There is no error information about EdgeAppLibSensorCoreInit and
 * EdgeAppLibSensorCoreExit. The error information is not updated if API is done
 * successfully.
 */
enum EdgeAppLibSensorErrorCause SensorGetLastErrorCause(void);

/**
 * @brief Get string of last error occurred in EdgeAppLibSensor
 * @param[in] param Flag to switch the kind of error string
 * @param[out] buffer Memory region to store error string
 * @param[in,out] length Buffer size for input, string length for output
 * @return Zero for success or negative value for failure
 * @details There is no error information about EdgeAppLibSensorCoreInit and
 * EdgeAppLibSensorCoreExit. The error information is not updated if API is done
 * successfully.
 * @note Output length does not include terminating null byte
 */
int32_t SensorGetLastErrorString(enum EdgeAppLibSensorStatusParam param,
                                 char *buffer, uint32_t *length);

/**
 * @brief Get the frame latency information
 * @param[in] frame Handle of frame to get latency information
 * @param[out] sequence_number Sequence number of the frame
 * @param[out] info Pointer to EdgeAppLibLatencyTimestamps to store latency
 * information
 * @return Zero for success or negative value for failure
 * @details This function can be called whenever frame is valid
 *
 * @note The latency information is valid until EdgeAppLibSensorReleaseFrame is
 * called
 */
int32_t SensorGetFrameLatency(EdgeAppLibSensorFrame frame,
                              uint64_t *sequence_number,
                              EdgeAppLibLatencyTimestamps *info);

/**
 * @brief Set the mode of sensor latency
 * @param[in] is_enable Enable or disable the sensor latency
 * @param[in] backlog The number of frames to backlog
 */
int32_t SensorLatencySetMode(bool is_enable, uint32_t backlog);
#ifdef __cplusplus
}
#endif
}  // namespace EdgeAppLib

#endif  // _AITRIOS_SENSOR_H_
