/*
 * Copyright 2022 iLogtail Authors
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

#include <json/json.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "container_manager/DockerContainerPathCmd.h"
#include "log_pb/sls_logs.pb.h"

namespace logtail {

struct Mount {
    std::string Source;
    std::string Destination;
};

struct DockerContainerPath {
    enum class InputType {
        InputFile = 0,
        InputContainerLog = 1,
    };
    std::string mContainerID; // id of this container
    // container path for this config's path. eg, config path '/home/admin', container path
    // '/host_all/var/lib/xxxxxx/upper/home/admin' if config is wildcard, this will mapping to config->mWildcardPaths[0]
    std::string mContainerPath;

    std::string mStdoutPath;
    std::string mStdoutLogType;
    std::string mUpperDir;
    std::vector<Mount> mMounts; // mounts of this container
    std::vector<sls_logs::LogTag> mContainerTags; // tags extracted from this container
    std::string mJsonStr; // this obj's json string, for saving to local file

    InputType mInputType;

    static bool ParseByJSONObj(const Json::Value& jsonObj, DockerContainerPath& dockerContainerPath);
    static bool ParseAllByJSONObj(const Json::Value& jsonObj,
                                  std::unordered_map<std::string, DockerContainerPath>& dockerContainerPathMap);

    bool operator==(const DockerContainerPath& rhs) const {
        if (mContainerID != rhs.mContainerID) {
            return false;
        }
        if (mContainerPath != rhs.mContainerPath) {
            return false;
        }
        if (mStdoutPath != rhs.mStdoutPath) {
            return false;
        }
        if (mStdoutLogType != rhs.mStdoutLogType) {
            return false;
        }
        if (mUpperDir != rhs.mUpperDir) {
            return false;
        }
        if (mMounts.size() != rhs.mMounts.size()) {
            return false;
        }
        for (size_t idx = 0; idx < mMounts.size(); ++idx) {
            const auto& lhsMount = mMounts[idx];
            const auto& rhsMount = rhs.mMounts[idx];
            if (lhsMount.Source != rhsMount.Source || lhsMount.Destination != rhsMount.Destination) {
                return false;
            }
        }
        if (mContainerTags.size() != rhs.mContainerTags.size()) {
            return false;
        }
        for (size_t idx = 0; idx < mContainerTags.size(); ++idx) {
            const auto& lhsTag = mContainerTags[idx];
            const auto& rhsTag = rhs.mContainerTags[idx];
            if (lhsTag.key() != rhsTag.key() || lhsTag.value() != rhsTag.value()) {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const DockerContainerPath& rhs) const { return !(*this == rhs); }

private:
};

} // namespace logtail
