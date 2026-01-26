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

#ifndef MOCK_AITRIOS_PREPROCESS_LIB_H
#define MOCK_AITRIOS_PREPROCESS_LIB_H

#include "edgeapp_core.h"

namespace PreprocessLib {

EdgeAppCoreResult normalizePreprocess(const void *input_data,
                                      EdgeAppLibImageProperty input_property,
                                      void **output_data,
                                      EdgeAppLibImageProperty *output_property);
EdgeAppCoreResult grayscalePreprocess(const void *input_data,
                                      EdgeAppLibImageProperty input_property,
                                      EdgeAppCore::Tensor *output_tensor);

}  // namespace PreprocessLib

#endif /* MOCK_AITRIOS_PREPROCESS_LIB_H */
