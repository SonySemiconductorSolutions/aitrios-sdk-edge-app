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

#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H

// to mock functions with gtest we need the virtual keyword but increases code
// size
#ifdef UNIT_TEST
#define UT_ATTRIBUTE virtual
#else
#define UT_ATTRIBUTE
#endif

#endif /* COMMON_MACROS_H */
