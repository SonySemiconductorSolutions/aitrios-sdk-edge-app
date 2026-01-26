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

#ifndef PREPROCESS_LIB_H_
#define PREPROCESS_LIB_H_

#include "edgeapp_core.h"

namespace PreprocessLib {

// Normalize pixel values from [0,255] to [0,1]
EdgeAppCoreResult normalizePreprocess(const void *input_data,
                                      EdgeAppLibImageProperty input_property,
                                      void **output_data,
                                      EdgeAppLibImageProperty *output_property);

// Convert RGB to grayscale
EdgeAppCoreResult grayscalePreprocess(const void *input_data,
                                      EdgeAppLibImageProperty input_property,
                                      EdgeAppCore::Tensor *output_tensor);

}  // namespace PreprocessLib

#endif  // PREPROCESS_LIB_H_
