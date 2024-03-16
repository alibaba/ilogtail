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

#include "file_server/DockerContainerPath.h"

#include <memory>

#include "file_server/FileServer.h"
#include "input/InputContainerLog.h"
#include "input/InputFile.h"
#include "logger/Logger.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineManager.h"

namespace logtail {

bool DockerContainerPath::ParseAllByJSONStr(
    const DockerContainerPathCmd* pCmd, std::unordered_map<std::string, DockerContainerPath>& dockerContainerPathMap) {
    dockerContainerPathMap.clear();
    Json::Value paramsAll;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> jsonReader(builder.newCharReader());
    std::string jsonParseErrs;
    if (pCmd->mParams.size() < 5UL
        || !jsonReader->parse(
            pCmd->mParams.data(), pCmd->mParams.data() + pCmd->mParams.size(), &paramsAll, &jsonParseErrs)) {
        LOG_ERROR(sLogger, ("invalid docker container params error", pCmd->mParams));
        return false;
    }
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
        DockerContainerPath nowDCP;
        nowDCP.mJsonStr = params.toStyledString();
        if (!ParseByJSONObj(params, pCmd, nowDCP)) {
            LOG_ERROR(sLogger, ("parse sub docker container params error", nowDCP.mJsonStr));
            return false;
        }
        dockerContainerPathMap[nowDCP.mContainerID] = nowDCP;
    }
    return true;
}

bool hasPrefix(const std::string& fullString, const std::string& prefix) {
    if (fullString.length() < prefix.length()) {
        return false;
    }
    return fullString.compare(0, prefix.length(), prefix) == 0;
}

bool DockerContainerPath::ParseByJSONObj(const Json::Value& params,
                                         const DockerContainerPathCmd* pCmd,
                                         DockerContainerPath& dockerContainerPath) {
    if (params.isMember("ID") && params["ID"].isString()) {
        dockerContainerPath.mContainerID = params["ID"].asString();
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
                dockerContainerPath.mMounts.push_back(mount);
            }
        }
    }
    if (params.isMember("DefaultRootPath") && params["DefaultRootPath"].isString()) {
        dockerContainerPath.mDefaultRootPath = params["DefaultRootPath"].asString();
    }
    if (params.isMember("StreamLogPath") && params["StreamLogPath"].isString()) {
        dockerContainerPath.mStreamLogPath = params["StreamLogPath"].asString();
    }
    if (params.isMember("StreamLogType") && params["StreamLogType"].isString()) {
        dockerContainerPath.mStreamLogType = params["StreamLogType"].asString();
    }
    if (params.isMember("Tags") && params["Tags"].isArray()) {
        const Json::Value& tags = params["Tags"];
        for (Json::ArrayIndex i = 1; i < tags.size(); i += 2) {
            if (tags[i].isString() && tags[i - 1].isString()) {
                sls_logs::LogTag tag;
                tag.set_key(tags[i - 1].asString());
                tag.set_value(tags[i].asString());
                dockerContainerPath.mContainerTags.push_back(tag);
            }
        }
    }
    // 找config
    std::shared_ptr<Pipeline> config = PipelineManager::GetInstance()->FindPipelineByName(pCmd->mConfigName);
    if (config->GetInputs().size() == 0) {
        LOG_ERROR(sLogger, ("config has no input", pCmd->mConfigName));
        return false;
    }
    std::string name = config->GetInputs()[0]->GetPlugin()->Name();

    if (name == InputFile::sName) {
        const InputFile* inputFile = static_cast<const InputFile*>(config->GetInputs()[0]->GetPlugin());
        std::string logPath;
        if (!inputFile->mFileDiscovery.GetWildcardPaths().empty()) {
            logPath = inputFile->mFileDiscovery.GetWildcardPaths()[0];
        } else {
            logPath = inputFile->mFileDiscovery.GetBasePath();
        }
        int pthSize = logPath.size();

        Mount bestMatchedMounts;

        for (size_t i = 0; i < dockerContainerPath.mMounts.size(); i++) {
            std::string dst = dockerContainerPath.mMounts[i].Destination;
            int dstSize = dst.size();

            if (hasPrefix(logPath, dst)
                && (pthSize == dstSize || (pthSize > dstSize && (logPath[dstSize] == '/' || logPath[dstSize] == '\\')))
                && bestMatchedMounts.Destination.size() < dstSize) {
                bestMatchedMounts = dockerContainerPath.mMounts[i];
            }
        }
        if (bestMatchedMounts.Source.size() > 0) {
            dockerContainerPath.mContainerPath
                = bestMatchedMounts.Source + logPath.substr(bestMatchedMounts.Destination.size());
            LOG_DEBUG(sLogger,
                      ("docker container path", dockerContainerPath.mContainerPath)("source", bestMatchedMounts.Source)(
                          "destination", bestMatchedMounts.Destination)("logPath", logPath)("input", name));
        } else {
            dockerContainerPath.mContainerPath = dockerContainerPath.mDefaultRootPath + logPath;
            LOG_DEBUG(sLogger,
                      ("docker container path", dockerContainerPath.mContainerPath)(
                          "defaultRootPath", dockerContainerPath.mDefaultRootPath)("logPath", logPath)("input", name));
        }
    }

    if (name == InputContainerLog::sName) {
        FileReaderConfig readerConfig = FileServer::GetInstance()->GetFileReaderConfig(pCmd->mConfigName);
        logtail::FileReaderOptions* ops = const_cast<logtail::FileReaderOptions*>(readerConfig.first);
        if (dockerContainerPath.mStreamLogType == "docker_json-file") {
            ops->mFileEncoding = FileReaderOptions::Encoding::DOCKER_JSON_FILE;
        } else if (dockerContainerPath.mStreamLogType == "containerd_text") {
            ops->mFileEncoding = FileReaderOptions::Encoding::CONTAINERD_TEXT;
        }
        readerConfig = FileServer::GetInstance()->GetFileReaderConfig(pCmd->mConfigName);
        std::string containerdLogType;
        if (readerConfig.first->mFileEncoding == FileReaderOptions::Encoding::DOCKER_JSON_FILE) {
            containerdLogType = "docker_json-file";
        } else if (readerConfig.first->mFileEncoding == FileReaderOptions::Encoding::CONTAINERD_TEXT) {
            containerdLogType = "containerd_text";
        } else {
            containerdLogType = "unknown";
        }

        readerConfig = FileServer::GetInstance()->GetFileReaderConfig(pCmd->mConfigName);
        size_t pos = dockerContainerPath.mStreamLogPath.find_last_of('/');
        if (pos != std::string::npos) {
            dockerContainerPath.mContainerPath = dockerContainerPath.mStreamLogPath.substr(0, pos);
        }
        if (dockerContainerPath.mContainerPath.length() > 1 && dockerContainerPath.mContainerPath.back() == '/') {
            dockerContainerPath.mContainerPath.pop_back();
        }
        LOG_DEBUG(sLogger,
                  ("docker container path", dockerContainerPath.mContainerPath)("input", name)("containerd log type",
                                                                                               containerdLogType));
    }

    // only check containerID (others are null when parse delete cmd)
    if (dockerContainerPath.mContainerID.size() > 0) {
        return true;
    }
    return false;
}

bool DockerContainerPath::ParseByJSONStr(const DockerContainerPathCmd* pCmd, DockerContainerPath& dockerContainerPath) {
    dockerContainerPath.mJsonStr = pCmd->mParams;
    Json::Value params;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> jsonReader(builder.newCharReader());
    std::string jsonParseErrs;
    if (pCmd->mParams.size() < 5UL
        || !jsonReader->parse(
            pCmd->mParams.data(), pCmd->mParams.data() + pCmd->mParams.size(), &params, &jsonParseErrs)) {
        LOG_ERROR(sLogger, ("invalid docker container params", pCmd->mParams));
        return false;
    }
    return ParseByJSONObj(params, pCmd, dockerContainerPath);
}

} // namespace logtail
