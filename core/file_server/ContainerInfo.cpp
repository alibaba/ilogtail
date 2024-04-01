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

#include "common/PathUtil.h"
#include "input/InputContainerLog.h"
#include "input/InputFile.h"
#include "logger/Logger.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineManager.h"

namespace logtail {

bool ContainerInfo::ParseAllByJSONObj(const Json::Value& paramsAll,
                                      std::unordered_map<std::string, ContainerInfo>& containerInfoMap,
                                      std::string& errorMsg,
                                      const std::string& configName) {
    containerInfoMap.clear();

    // The json library changes its behavior for {"AllCmd":null}:
    // - v0.16.x: isArray() returns true.
    // - v1.x: isArray() returns false.
    // Do null check for compatibility.
    if (paramsAll.isNull()) {
        return true;
    }
    if (!paramsAll.isArray()) {
        errorMsg = "param is not of type array, param: " + paramsAll.toStyledString();
        return false;
    }
    for (auto iter = paramsAll.begin(); iter != paramsAll.end(); ++iter) {
        Json::Value params = *iter;
        ContainerInfo nowDCP;
        if (!ParseByJSONObj(params, nowDCP, errorMsg, configName)) {
            errorMsg = "some container info is invalid: " + errorMsg;
            return false;
        }
        containerInfoMap[nowDCP.mID] = nowDCP;
    }
    return true;
}

bool ContainerInfo::ParseByJSONObj(const Json::Value& params,
                                   ContainerInfo& containerInfo,
                                   std::string& errorMsg,
                                   const std::string& configName) {
    containerInfo.mJson = params;
    if (params.isMember("ID") && params["ID"].isString()) {
        if (params["ID"].empty()) {
            errorMsg = "container id is empty, param: " + params.asString();
            return false;
        }
        containerInfo.mID = params["ID"].asString();
    }

    if (params.isMember("Mounts") && params["Mounts"].isArray()) {
        const Json::Value& mounts = params["Mounts"];
        for (Json::ArrayIndex i = 0; i < mounts.size(); ++i) {
            if (mounts[i].isMember("Source") && mounts[i]["Source"].isString() && mounts[i].isMember("Destination")
                && mounts[i]["Destination"].isString()) {
                std::string dst = mounts[i]["Destination"].asString();
                std::string src = mounts[i]["Source"].asString();
                // go传cmd时也做了path.clean, 这边兜底再判断下
                RemoveFilePathTrailingSlash(dst);
                RemoveFilePathTrailingSlash(src);
                containerInfo.mMounts.emplace_back(src, dst);
            }
        }
    }
    if (params.isMember("UpperDir") && params["UpperDir"].isString()) {
        containerInfo.mUpperDir = params["UpperDir"].asString();
        // go传cmd时也做了path.clean, 这边兜底再判断下
        RemoveFilePathTrailingSlash(containerInfo.mUpperDir);
    }
    if (params.isMember("StdoutPath") && params["StdoutPath"].isString()) {
        containerInfo.mLogPath = params["StdoutPath"].asString();
    }
    bool oldVersion = false;
    if (params.isMember("MetaDatas") && params["MetaDatas"].isArray()) {
        const Json::Value& metaDatas = params["MetaDatas"];
        for (Json::ArrayIndex i = 1; i < metaDatas.size(); i += 2) {
            if (metaDatas[i].isString() && metaDatas[i - 1].isString()) {
                sls_logs::LogTag tag;
                tag.set_key(metaDatas[i - 1].asString());
                tag.set_value(metaDatas[i].asString());
                containerInfo.mMetadatas.push_back(tag);
            }
        }
    } else {
        oldVersion = true;
    }
    if (oldVersion) {
        std::shared_ptr<Pipeline> config = PipelineManager::GetInstance()->FindPipelineByName(configName);
        // Determine the type of input plugin.
        std::string name = config->GetInputs()[0]->GetPlugin()->Name();
        const InputFile* inputFile = nullptr;
        const InputContainerLog* inputContainerLog = nullptr;
        std::unordered_map<std::string, std::string> externalEnvTag;
        std::unordered_map<std::string, std::string> externalK8sLabelTag;
        if (name == InputFile::sName) {
            inputFile = static_cast<const InputFile*>(config->GetInputs()[0]->GetPlugin());
            externalEnvTag = inputFile->mContainerDiscovery.mExternalEnvTag;
            externalK8sLabelTag = inputFile->mContainerDiscovery.mExternalK8sLabelTag;

        } else if (name == InputContainerLog::sName) {
            inputContainerLog = static_cast<const InputContainerLog*>(config->GetInputs()[0]->GetPlugin());
            externalEnvTag = inputContainerLog->mContainerDiscovery.mExternalEnvTag;
            externalK8sLabelTag = inputContainerLog->mContainerDiscovery.mExternalK8sLabelTag;
        }
        if (params.isMember("Tags") && params["Tags"].isArray()) {
            const Json::Value& tags = params["Tags"];
            for (Json::ArrayIndex i = 1; i < tags.size(); i += 2) {
                if (tags[i].isString() && tags[i - 1].isString()) {
                    sls_logs::LogTag tag;
                    tag.set_key(tags[i - 1].asString());
                    tag.set_value(tags[i].asString());
                    bool found = false;
                    for (const auto& pair : externalEnvTag) {
                        if (pair.second == tags[i - 1].asString()) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        for (const auto& pair : externalK8sLabelTag) {
                            if (pair.second == tags[i - 1].asString()) {
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) {
                        containerInfo.mMetadatas.push_back(tag);
                    } else {
                        containerInfo.mTags.push_back(tag);
                    }
                }
            }
        }
    } else {
        if (params.isMember("Tags") && params["Tags"].isArray()) {
            const Json::Value& tags = params["Tags"];
            for (Json::ArrayIndex i = 1; i < tags.size(); i += 2) {
                if (tags[i].isString() && tags[i - 1].isString()) {
                    sls_logs::LogTag tag;
                    tag.set_key(tags[i - 1].asString());
                    tag.set_value(tags[i].asString());
                    containerInfo.mTags.push_back(tag);
                }
            }
        }
    }
    if (params.isMember("Path") && params["Path"].isString()) {
        containerInfo.mRealBaseDir = params["Path"].asString();
    }
    return true;
}

} // namespace logtail
