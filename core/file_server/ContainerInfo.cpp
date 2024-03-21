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

#include "file_server/ContainerInfo.h"

#include <memory>

#include "logger/Logger.h"

namespace logtail {

bool ContainerInfo::ParseAllByJSONObj(const Json::Value& paramsAll,
                                      std::unordered_map<std::string, ContainerInfo>& containerInfoMap) {
    containerInfoMap.clear();
    if (!paramsAll.isMember("AllCmd")) {
        return false;
    }

    auto paramsArray = paramsAll["AllCmd"];
    // The json library changes its behavior for {"AllCmd":null}:
    // - v0.16.x: isArray() returns true.
    // - v1.x: isArray() returns false.
    // Do null check for compatibility.
    if (paramsArray.isNull()) {
        return true;
    }
    if (!paramsArray.isArray()) {
        return false;
    }
    for (Json::Value::iterator iter = paramsArray.begin(); iter != paramsArray.end(); ++iter) {
        Json::Value params = *iter;
        ContainerInfo nowDCP;
        nowDCP.mJsonStr = params.toStyledString();
        if (!ParseByJSONObj(params, nowDCP)) {
            LOG_ERROR(sLogger, ("parse sub docker container params error", nowDCP.mJsonStr));
            return false;
        }
        containerInfoMap[nowDCP.mContainerID] = nowDCP;
    }
    return true;
}

bool ContainerInfo::ParseByJSONObj(const Json::Value& params, ContainerInfo& containerInfo) {
    if (params.isMember("ID") && params["ID"].isString()) {
        containerInfo.mContainerID = params["ID"].asString();
    }

    if (params.isMember("Mounts") && params["Mounts"].isArray()) {
        const Json::Value& mounts = params["Mounts"];
        for (Json::ArrayIndex i = 0; i < mounts.size(); i++) {
            if (mounts[i].isMember("Source") && mounts[i]["Source"].isString() && mounts[i].isMember("Destination")
                && mounts[i]["Destination"].isString()) {
                std::string dst = mounts[i]["Destination"].asString();
                std::string src = mounts[i]["Source"].asString();

                Mount mount;
                mount.Source = src;
                mount.Destination = dst;
                containerInfo.mMounts.push_back(mount);
            }
        }
    }
    if (params.isMember("UpperDir") && params["UpperDir"].isString()) {
        containerInfo.mUpperDir = params["UpperDir"].asString();
    }
    if (params.isMember("StdoutPath") && params["StdoutPath"].isString()) {
        containerInfo.mStdoutPath = params["StdoutPath"].asString();
    }
    if (params.isMember("Tags") && params["Tags"].isArray()) {
        const Json::Value& tags = params["Tags"];
        for (Json::ArrayIndex i = 1; i < tags.size(); i += 2) {
            if (tags[i].isString() && tags[i - 1].isString()) {
                sls_logs::LogTag tag;
                tag.set_key(tags[i - 1].asString());
                tag.set_value(tags[i].asString());
                containerInfo.mContainerTags.push_back(tag);
            }
        }
    }
    // only check containerID (others are null when parse delete cmd)
    if (containerInfo.mContainerID.size() > 0) {
        return true;
    }
    return false;
}

} // namespace logtail
