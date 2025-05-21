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

#ifndef DATA_PROCESSOR_API_H
#define DATA_PROCESSOR_API_H

#include <edgeapp/send_data_types.h>

#include <cstdint>

typedef enum {
  kDataProcessorOk,
  kDataProcessorUninitialized,
  kDataProcessorInvalidParam,
  kDataProcessorMemoryError,
  kDataProcessorInvalidState,
  kDataProcessorOther,
  kDataProcessorOutOfRange,
  kDataProcessorInvalidParamSetError,
} DataProcessorResultCode;

/**
 * Initializes the Data Processor and sets default values.
 *
 * @return DataProcessorResultCode
 */
DataProcessorResultCode DataProcessorInitialize();

/**
 * @brief Configures the data processor based on the provided JSON
 * configuration.
 *
 * It initializes or updates global variables with the extracted parameters.
 * Additionally, it handles error cases and generates error messages in JSON
 * format if necessary.
 *
 * @param config_json A pointer to the JSON configuration string.
 * @param out_config_json A pointer to a pointer where the resulting
 * configuration JSON string will be stored in case of errors. If the return is
 * different from kDataProcessorOk, it contains the correct JSON error message
 * with the updated values.
 * @return DataProcessorResultCode
 *
 * @example
 * `config_json`:
 *
 *    {
 *        "ai_models": {
 *            "classification": {
 *                "parameters": {
 *                    "max_predictions": 10
 *                }
 *            }
 *        }
 *    }
 *
 * `out_config_json` (if required):
 *
 *    {
 *        "res_info": {
 *            "res_id": "2acc77f6-b8c5-44ca-8b51-86f11c26eb97",
 *            "code": 2,
 *            "detail_msg": "Max predictions cannot be higher than output
 * classes"
 *        },
 *        "ai_models": {
 *            "classification": {
 *                "parameters": {
 *                    "max_predictions": 5
 *                }
 *            }
 *        }
 *    }
 */
DataProcessorResultCode DataProcessorConfigure(char *config_json,
                                               char **out_config_json);

/**
 * Post processing function
 *
 * @param in_data Output tensor as an array of floats.
 * @param in_size Output tensor array of floats size(bytes).
 * @param out_data Pointer of postprocessed FlatBuffer (classification,
 * detection, etc.).
 * @param out_size Pointer of postprocessed FlatBuffer size.
 * memory
 * @return DataProcessorResultCode
 */
DataProcessorResultCode DataProcessorAnalyze(float *in_data, uint32_t in_size,
                                             char **out_data,
                                             uint32_t *out_size);

/**
 * Resets current running state.
 * Configuration is preserved.
 *
 * @return DataProcessorResultCode
 */
DataProcessorResultCode DataProcessorResetState();

/**
 * Finalizes the Data Processor
 *
 * @return DataProcessorResultCode
 */
DataProcessorResultCode DataProcessorFinalize();

EdgeAppLibSendDataType DataProcessorGetDataType();

/**
 * output tensor format processing function
 *
 * @param in_data Pointer of output tensor buffer.
 * @param in_size Pointer of output tensor buffer size.
 * @param timestamp Timestamp of output tensor.
 * @param out_data Pointer of output tensor with Json format.
 * @param out_size Pointer of output tensor with Json format's size.
 * memory
 * @return DataProcessorResultCode
 */
DataProcessorResultCode DataProcessorJsonFormat(void *in_data, uint32_t in_size,
                                                uint64_t timestamp,
                                                char **out_data,
                                                uint32_t *out_size);
#endif  // DATA_PROCESSOR_API_H
