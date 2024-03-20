// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "input/InputContainerLog.h"

#include "app_config/AppConfig.h"
#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"
#include "file_server/FileServer.h"
#include "pipeline/Pipeline.h"

using namespace std;

DECLARE_FLAG_STRING(default_container_host_path);

namespace logtail {

const string InputContainerLog::sName = "input_container_log";

bool InputContainerLog::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;
    if (!AppConfig::GetInstance()->IsPurageContainerMode()) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           "iLogtail is not in container, but container stdout collection is required.",
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }

    static Json::Value fileDiscoveryConfig(Json::objectValue);
    if (fileDiscoveryConfig.empty()) {
        fileDiscoveryConfig["FilePaths"] = Json::Value(Json::arrayValue);
        fileDiscoveryConfig["FilePaths"].append("/**/*.log*");
        fileDiscoveryConfig["AllowingCollectingFilesInRootDir"] = true;
    }

    {
        string key = "AllowingIncludedByMultiConfigs";
        const Json::Value* itr = config.find(key.c_str(), key.c_str() + key.length());
        if (itr != nullptr) {
            fileDiscoveryConfig[key] = *itr;
        }
    }
    if (!mFileDiscovery.Init(fileDiscoveryConfig, *mContext, sName)) {
        return false;
    }
    mFileDiscovery.SetEnableContainerDiscoveryFlag(true);
    mFileDiscovery.SetUpdateContainerInfoFunc(UpdateContainerInfoFunc);
    mFileDiscovery.SetDeleteContainerInfoFunc(DeleteContainerInfo);
    mFileDiscovery.SetIsSameDockerContainerPathFunc(IsSameDockerContainerPath);

    if (!mContainerDiscovery.Init(config, *mContext, sName)) {
        return false;
    }
    mContainerDiscovery.GenerateContainerMetaFetchingGoPipeline(optionalGoPipeline);

    if (!mFileReader.Init(config, *mContext, sName)) {
        return false;
    }

    // Multiline
    {
        const char* key = "Multiline";
        const Json::Value* itr = config.find(key, key + strlen(key));
        if (itr) {
            if (!itr->isObject()) {
                PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                     mContext->GetAlarm(),
                                     "param Multiline is not of type object",
                                     sName,
                                     mContext->GetConfigName(),
                                     mContext->GetProjectName(),
                                     mContext->GetLogstoreName(),
                                     mContext->GetRegion());
            } else if (!mMultiline.Init(*itr, *mContext, sName)) {
                return false;
            }
        }
    }

    // IgnoringStdout
    if (!GetOptionalBoolParam(config, "IgnoringStdout", mIgnoringStdout, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoringStdout,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // IgnoringStderr
    if (!GetOptionalBoolParam(config, "IgnoringStderr", mIgnoringStderr, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoringStderr,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // IgnoreParseWarning
    if (!GetOptionalBoolParam(config, "IgnoreParseWarning", mIgnoreParseWarning, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoreParseWarning,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // KeepingSourceWhenParseFail
    if (!GetOptionalBoolParam(config, "KeepingSourceWhenParseFail", mKeepingSourceWhenParseFail, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mKeepingSourceWhenParseFail,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    if (!mIgnoringStdout && !mIgnoringStderr && mMultiline.IsMultiline()) {
        string warningMsg = "This logtail config has multiple lines of configuration, and when collecting stdout and "
                            "stderr logs at the same time, there may be issues with merging multiple lines";
        LOG_WARNING(sLogger, ("warning", warningMsg)("config", mContext->GetConfigName()));
        warningMsg = "warning msg: " + warningMsg + "\tconfig: " + mContext->GetConfigName();
        LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                               warningMsg,
                                               GetContext().GetProjectName(),
                                               GetContext().GetLogstoreName(),
                                               GetContext().GetRegion());
    }

    return true;
}

bool InputContainerLog::DeleteContainerInfo(const Json::Value& paramsJSON, FileDiscoveryOptions* fileDiscovery) {
    DockerContainerPath dockerContainerPath;
    if (!DockerContainerPath::ParseByJSONObj(paramsJSON, dockerContainerPath)) {
        return false;
    }
    for (vector<DockerContainerPath>::iterator iter = fileDiscovery->GetContainerInfo()->begin();
         iter != fileDiscovery->GetContainerInfo()->end();
         ++iter) {
        if (iter->mContainerID == dockerContainerPath.mContainerID) {
            fileDiscovery->GetContainerInfo()->erase(iter);
            break;
        }
    }
    return true;
}

void SetContainerPath(DockerContainerPath& dockerContainerPath) {
    dockerContainerPath.mInputType = DockerContainerPath::InputType::InputContainerLog;
    size_t pos = dockerContainerPath.mStdoutPath.find_last_of('/');
    if (pos != std::string::npos) {
        dockerContainerPath.mContainerPath = dockerContainerPath.mStdoutPath.substr(0, pos);
    }
    if (dockerContainerPath.mContainerPath.length() > 1 && dockerContainerPath.mContainerPath.back() == '/') {
        dockerContainerPath.mContainerPath.pop_back();
    }
    dockerContainerPath.mContainerPath
        = STRING_FLAG(default_container_host_path).c_str() + dockerContainerPath.mContainerPath;
    LOG_DEBUG(sLogger, ("docker container path", dockerContainerPath.mContainerPath));
}

bool InputContainerLog::UpdateContainerInfoFunc(const Json::Value& paramsJSON,
                                                bool allFlag,
                                                FileDiscoveryOptions* fileDiscovery) {
    if (!allFlag) {
        DockerContainerPath dockerContainerPath;
        if (!DockerContainerPath::ParseByJSONObj(paramsJSON, dockerContainerPath)) {
            LOG_ERROR(sLogger,
                      ("invalid docker container params", "skip this path")("params", paramsJSON.toStyledString()));
            return false;
        }
        SetContainerPath(dockerContainerPath);
        // try update
        for (size_t i = 0; i < fileDiscovery->GetContainerInfo()->size(); ++i) {
            if ((*fileDiscovery->GetContainerInfo())[i].mContainerID == dockerContainerPath.mContainerID) {
                // update
                (*fileDiscovery->GetContainerInfo())[i] = dockerContainerPath;
                return true;
            }
        }
        // add
        fileDiscovery->GetContainerInfo()->push_back(dockerContainerPath);
        return true;
    }

    unordered_map<string, DockerContainerPath> allPathMap;
    if (!DockerContainerPath::ParseAllByJSONObj(paramsJSON, allPathMap)) {
        LOG_ERROR(sLogger,
                  ("invalid all docker container params", "skip this path")("params", paramsJSON.toStyledString()));
        return false;
    }
    // if update all, clear and reset
    fileDiscovery->GetContainerInfo()->clear();
    for (unordered_map<string, DockerContainerPath>::iterator iter = allPathMap.begin(); iter != allPathMap.end();
         ++iter) {
        SetContainerPath(iter->second);
        fileDiscovery->GetContainerInfo()->push_back(iter->second);
    }
    return true;
}

bool InputContainerLog::IsSameDockerContainerPath(const Json::Value& paramsJSON,
                                                  bool allFlag,
                                                  FileDiscoveryOptions* fileDiscovery) {
    if (!allFlag) {
        DockerContainerPath dockerContainerPath;
        if (!DockerContainerPath::ParseByJSONObj(paramsJSON, dockerContainerPath)) {
            LOG_ERROR(sLogger,
                      ("invalid docker container params", "skip this path")("params", paramsJSON.toStyledString()));
            return true;
        }
        SetContainerPath(dockerContainerPath);
        // try update
        for (size_t i = 0; i < fileDiscovery->GetContainerInfo()->size(); ++i) {
            if ((*fileDiscovery->GetContainerInfo())[i] == dockerContainerPath) {
                return true;
            }
        }
        return false;
    }

    // check all
    unordered_map<string, DockerContainerPath> allPathMap;
    if (!DockerContainerPath::ParseAllByJSONObj(paramsJSON, allPathMap)) {
        LOG_ERROR(sLogger,
                  ("invalid all docker container params", "skip this path")("params", paramsJSON.toStyledString()));
        return true;
    }

    // need add
    if (fileDiscovery->GetContainerInfo()->size() != allPathMap.size()) {
        return false;
    }

    for (size_t i = 0; i < fileDiscovery->GetContainerInfo()->size(); ++i) {
        unordered_map<string, DockerContainerPath>::iterator iter
            = allPathMap.find((*fileDiscovery->GetContainerInfo())[i].mContainerID);
        // need delete
        if (iter == allPathMap.end()) {
            return false;
        }
        SetContainerPath(iter->second);
        // need update
        if ((*fileDiscovery->GetContainerInfo())[i] != iter->second) {
            return false;
        }
    }
    // same
    return true;
}

bool InputContainerLog::Start() {
    mFileDiscovery.SetContainerInfo(
        FileServer::GetInstance()->GetAndRemoveContainerInfo(mContext->GetPipeline().Name()));
    FileServer::GetInstance()->AddFileDiscoveryConfig(mContext->GetConfigName(), &mFileDiscovery, mContext);
    FileServer::GetInstance()->AddFileReaderConfig(mContext->GetConfigName(), &mFileReader, mContext);
    FileServer::GetInstance()->AddMultilineConfig(mContext->GetConfigName(), &mMultiline, mContext);
    return true;
}

bool InputContainerLog::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        FileServer::GetInstance()->SaveContainerInfo(mContext->GetPipeline().Name(), mFileDiscovery.GetContainerInfo());
    }
    FileServer::GetInstance()->RemoveFileDiscoveryConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveFileReaderConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveMultilineConfig(mContext->GetConfigName());
    return true;
}

} // namespace logtail
