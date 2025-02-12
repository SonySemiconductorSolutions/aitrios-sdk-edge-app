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

#ifndef _AITRIOS_SENSOR_UNSUPPORTED_H_
#define _AITRIOS_SENSOR_UNSUPPORTED_H_

enum EdgeAppLibSensorCameraScalingPolicyUnsupported {
  /** Currently, not supported. */
  AITRIOS_SENSOR_CAMERA_SCALING_POLICY_AUTO_UNSUPPORTED = 0,
};

enum EdgeAppLibSensorCameraExposureModeUnsupported {
  /**Currently, not supported. */
  AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_GAIN_FIX_UNSUPPORTED = 1,
  /**Currently, not supported. */
  AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_TIME_FIX_UNSUPPORTED = 2,
  /** Currently, not supported. */
  AITRIOS_SENSOR_CAMERA_EXPOSURE_MODE_HOLD_UNSUPPORTED = 4,
};

enum EdgeAppLibSensorInferenceWhiteBalanceModeUnsupported {
  /** Currently, not supported. */
  AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_MANUAL_GAIN_UNSUPPORTED = 2,
  /** Currently, not supported. (one push white balance) */
  AITRIOS_SENSOR_INFERENCE_WHITE_BALANCE_MODE_HOLD_UNSUPPORTED = 3,
};

/**
 * 5.4.23. ManualWhiteBalanceGainProperty
 * Currently, not supported.
 */
#define AITRIOS_SENSOR_MANUAL_WHITE_BALANCE_GAIN_PROPERTY_KEY_UNSUPPORTED \
  "manual_white_balance_gain_property"

/**
 * 5.4.1. ChannelInfoProperty
 */
#define AITRIOS_SENSOR_CHANNEL_INFO_PROPERTY_KEY_UNSUPPORTED \
  "channel_info_property"

/**
 * 5.4.2. ChannelMaskProperty
 */
#define AITRIOS_SENSOR_CHANNEL_MASK_PROPERTY_KEY_UNSUPPORTED \
  "channel_mask_property"

/**
 * 5.4.3. CurrentFrameNumProperty
 */
#define AITRIOS_SENSOR_CURRENT_FRAME_NUM_PROPERTY_KEY_UNSUPPORTED \
  "current_frame_num_property"

/**
 * 5.4.4. FrameRateProperty
 */
/**
 * FrameRateProperty
 * @see senscord::kFrameRatePropertyKey
 */
#define AITRIOS_SENSOR_FRAME_RATE_PROPERTY_KEY_UNSUPPORTED "frame_rate_property"

/**
 * 5.4.5. ImageProperty
 */
/**
 * ImageProperty
 * @see senscord::kImagePropertyKey
 */
#define AITRIOS_SENSOR_IMAGE_PROPERTY_KEY_UNSUPPORTED "image_property"

/**
 * 5.4.9. InferenceModelIndexProperty
 */
/**
 * @def AITRIOS_SENSOR_AI_MODEL_INDEX_PROPERTY_KEY
 * @brief Property key to set/get property EdgeAppLibSensorAiModelIndexProperty
 * @details Property is the index number of AI model in AI model bundle
 */
#define AITRIOS_SENSOR_AI_MODEL_INDEX_PROPERTY_KEY_UNSUPPORTED \
  "ai_model_index_property"

/**
 * 5.4.28. InferenceProperty
 */
#define AITRIOS_SENSOR_INFERENCE_PROPERTY_KEY_UNSUPPORTED "inference_property"

/**
 * 5.4.29. TensorShapesProperty
 */
#define AITRIOS_SENSOR_TENSOR_SHAPES_PROPERTY_KEY_UNSUPPORTED \
  "tensor_shapes_property"

/**
 * 5.4.30. InfoStringProperty
 */
#define AITRIOS_SENSOR_INFO_STRING_PROPERTY_KEY_UNSUPPORTED \
  "info_string_property"

/**
 * 5.4.31. TemperatureEnableProperty
 */
#define AITRIOS_SENSOR_TEMPERATURE_ENABLE_PROPERTY_KEY_UNSUPPORTED \
  "temperature_enable_property_property"

/**
 * 5.4.32. TemperatureProperty
 */
#define AITRIOS_SENSOR_TEMPERATURE_PROPERTY_KEY_UNSUPPORTED \
  "temperature_property"

/**
 * 5.4.33. RegisterAccessProperty
 */
#define AITRIOS_SENSOR_REGISTER_ACCESS_8_PROPERTY_KEY_UNSUPPORTED \
  "register_access_8_property"
#define AITRIOS_SENSOR_REGISTER_ACCESS_16_PROPERTY_KEY_UNSUPPORTED \
  "register_access_16_property"
#define AITRIOS_SENSOR_REGISTER_ACCESS_32_PROPERTY_KEY_UNSUPPORTED \
  "register_access_32_property"
#define AITRIOS_SENSOR_REGISTER_ACCESS_64_PROPERTY_KEY_UNSUPPORTED \
  "register_access_64_property"

#endif  // _AITRIOS_SENSOR_UNSUPPORTED_H_
