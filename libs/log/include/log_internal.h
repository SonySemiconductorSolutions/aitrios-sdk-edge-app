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

/**
 * @file log_internal.h
 * @brief Header file for Log API for sdk
 * @details This file defines the interface for configuration of logging.
 *          It provides functionalities to set and get configuration of logging.
 */

#ifndef _AITRIOS_LOG_INTERNAL_H_
#define _AITRIOS_LOG_INTERNAL_H_

#include <stdint.h>

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Set a log level.
 * @param[in] log_level A log level.
 * @details Set the log level received in Configuration to the Log API.
 */
void SetLogLevel(LogLevel log_level);

/**
 * @brief Get a log level.
 * @return A log level.
 * @details Get the log level set in the Log API.
 */
LogLevel GetLogLevel();

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _AITRIOS_LOG_INTERNAL_H_ */
