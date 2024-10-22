/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <string>

namespace logtail {

////////////////////////// LOG ////////////////////////
    extern const std::string DEAULFT_LOG_TAG_NAMESPACE;
    extern const std::string DEAULFT_LOG_TAG_POD_NAME;
    extern const std::string DEAULFT_LOG_TAG_POD_UID;
    extern const std::string DEAULFT_LOG_TAG_CONTAINER_NAME;
    extern const std::string DEAULFT_LOG_TAG_CONTAINER_IP;
    extern const std::string DEAULFT_LOG_TAG_IMAGE_NAME;

////////////////////////// METRIC ////////////////////////
    extern const std::string DEAULFT_METRIC_TAG_NAMESPACE;
    extern const std::string DEAULFT_METRIC_TAG_POD_NAME;
    extern const std::string DEAULFT_METRIC_TAG_POD_UID;
    extern const std::string DEAULFT_METRIC_TAG_CONTAINER_NAME;
    extern const std::string DEAULFT_METRIC_TAG_CONTAINER_IP;
    extern const std::string DEAULFT_METRIC_TAG_IMAGE_NAME;

} // namespace logtail