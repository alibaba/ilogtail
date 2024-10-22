// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "TagConstants.h"

namespace logtail {

////////////////////////// COMMON ////////////////////////
    const std::string DEAULFT_TAG_NAMESPACE = "namespace";
    const std::string DEAULFT_TAG_POD_NAME = "pod_name";
    const std::string DEAULFT_TAG_POD_UID = "pod_uid";
    const std::string DEAULFT_TAG_CONTAINER_NAME = "container_name";
    const std::string DEAULFT_TAG_CONTAINER_IP = "container_ip";
    const std::string DEAULFT_TAG_IMAGE_NAME = "image_name";

////////////////////////// LOG ////////////////////////
#ifndef __ENTERPRISE__
    const std::string DEAULFT_LOG_TAG_NAMESPACE = DEAULFT_TAG_NAMESPACE;
    const std::string DEAULFT_LOG_TAG_POD_NAME = DEAULFT_TAG_POD_NAME;
    const std::string DEAULFT_LOG_TAG_POD_UID = DEAULFT_TAG_POD_UID;
    const std::string DEAULFT_LOG_TAG_CONTAINER_NAME = DEAULFT_TAG_CONTAINER_NAME;
    const std::string DEAULFT_LOG_TAG_CONTAINER_IP = DEAULFT_TAG_CONTAINER_IP;
    const std::string DEAULFT_LOG_TAG_IMAGE_NAME = DEAULFT_TAG_IMAGE_NAME;
#else
    const std::string DEAULFT_LOG_TAG_NAMESPACE = "_namespace_";
    const std::string DEAULFT_LOG_TAG_POD_NAME = "_pod_name_";
    const std::string DEAULFT_LOG_TAG_POD_UID = "_pod_uid_";
    const std::string DEAULFT_LOG_TAG_CONTAINER_NAME = "_container_name_";
    const std::string DEAULFT_LOG_TAG_CONTAINER_IP = "_container_ip_";
    const std::string DEAULFT_LOG_TAG_IMAGE_NAME = "_image_name_";
#endif

////////////////////////// METRIC ////////////////////////
    const std::string DEAULFT_METRIC_TAG_NAMESPACE = DEAULFT_TAG_NAMESPACE;
    const std::string DEAULFT_METRIC_TAG_POD_NAME = DEAULFT_TAG_POD_NAME;
    const std::string DEAULFT_METRIC_TAG_POD_UID = DEAULFT_TAG_POD_UID;
    const std::string DEAULFT_METRIC_TAG_CONTAINER_NAME = DEAULFT_TAG_CONTAINER_NAME;
    const std::string DEAULFT_METRIC_TAG_CONTAINER_IP = DEAULFT_TAG_CONTAINER_IP;
    const std::string DEAULFT_METRIC_TAG_IMAGE_NAME = DEAULFT_TAG_IMAGE_NAME;

} // namespace logtail