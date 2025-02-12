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

#ifndef STATES_STATE_DEFS_H
#define STATES_STATE_DEFS_H

#define EVP_PROCESSEVENT_TIMEOUT_MS 1000

#define ON_START "onStart"
#define ON_ITERATE "onIterate"
#define ON_STOP "onStop"
#define ON_CREATE "onCreate"
#define ON_DESTROY "onDestroy"
#define ON_CONFIGURE "onConfigure"

#define AITRIOS_DATA_EXPORT_INITIALIZE "EdgeAppLibDataExportInitialize"
#define AITRIOS_DATA_EXPORT_UNINITIALIZE "EdgeAppLibDataExportUnInitialize"

#define SENSOR_CORE_INIT "EdgeAppLibSensorCoreInit"
#define SENSOR_CORE_EXIT "EdgeAppLibSensorCoreExit"
#define SENSOR_CORE_OPEN_STREAM "EdgeAppLibSensorCoreOpenStream"
#define SENSOR_CORE_CLOSE_STREAM "EdgeAppLibSensorCoreCloseStream"

#endif /* STATES_STATE_DEFS_H */
