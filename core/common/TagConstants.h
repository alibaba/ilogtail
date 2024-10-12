/*
 * Copyright 2024 iLogtail Authors
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
 */

#pragma once

#include <string>

namespace logtail {

const std::string LOG_RESERVED_KEY_SOURCE = "__source__";
const std::string LOG_RESERVED_KEY_TOPIC = "__topic__";
const std::string LOG_RESERVED_KEY_MACHINE_UUID = "__machine_uuid__";
const std::string LOG_RESERVED_KEY_PACKAGE_ID = "__pack_id__";
const std::string LOG_RESERVED_KEY_TRUNCATE_INFO = "__truncate_info__";

const std::string DEFAULT_CONFIG_TAG_KEY_VALUE = "__default__";

enum TagKey {
    FILE_OFFSET_KEY = 0,
    FILE_INODE_TAG_KEY = 1,
    FILE_PATH_TAG_KEY = 2,
    K8S_NAMESPACE_TAG_KEY = 3,
    K8S_POD_NAME_TAG_KEY = 4,
    K8S_POD_UID_TAG_KEY = 5,
    CONTAINER_NAME_TAG_KEY = 6,
    CONTAINER_IP_TAG_KEY = 7,
    CONTAINER_IMAGE_NAME_TAG_KEY = 8,
    K8S_CONTAINER_NAME_TAG_KEY = 9,
    K8S_CONTAINER_IMAGE_NAME_TAG_KEY = 10,
    K8S_CONTAINER_IP_TAG_KEY = 11,
    HOST_NAME = 12,
    NUM_VALUES = 13,
    UNKOWN = 14,
};

#ifdef __ENTERPRISE__
const std::string TagDefaultKey[NUM_VALUES] = {
    "__file_offset__",
    "__inode__",
    "__path__",
    "_namespace_",
    "_pod_name_",
    "_pod_uid_",
    "_container_name_",
    "_container_ip_",
    "_image_name_",
    "_container_name_",
    "_image_name_",
    "_container_ip_",
    "__hostname__",
};
const std::string AGENT_TAG_DEFAULT_KEY = "__user_defined_id__";
#else
const std::string TagDefaultKey[NUM_VALUES] = {
    "log.file.offset",
    "log.file.inode",
    "log.file.path",
    "k8s.namespace.name",
    "k8s.pod.name",
    "k8s.pod.uid",
    "container.name",
    "container.ip",
    "container.image.name",
    "k8s.container.name",
    "k8s.container.image.name",
    "k8s.container.ip",
    "host.name",
};
const std::string HOST_IP_DEFAULT_KEY = "host.ip";
#endif

} // namespace logtail