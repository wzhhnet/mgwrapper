/*
 * Mongoose Wrapper
 * Implemented by C++
 *
 * Author wanch
 * Date 2025/11/20
 * Email wzhhnet@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *a
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "mongoose.h"

#define MG_EV_USER_READY MG_EV_USER + 100

#ifndef LOGE
#define LOGE(...) MG_ERROR((__VA_ARGS__))
#endif
#ifndef LOGW
#define LOGW(...) MG_WARNING((__VA_ARGS__))
#endif
#ifndef LOGI
#define LOGI(...) MG_INFO((__VA_ARGS__))
#endif
#ifndef LOGD
#define LOGD(...) MG_DEBUG((__VA_ARGS__))
#endif