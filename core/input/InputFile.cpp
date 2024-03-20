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

#include "input/InputFile.h"

#include <filesystem>

#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"
#include "config_manager/ConfigManager.h"
#include "file_server/FileServer.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineManager.h"

using namespace std;

DEFINE_FLAG_INT32(search_checkpoint_default_dir_depth, "0 means only search current directory", 0);
DEFINE_FLAG_INT32(max_exactly_once_concurrency, "", 512);
DECLARE_FLAG_STRING(default_container_host_path);

namespace logtail {

const string InputFile::sName = "input_file";

InputFile::InputFile()
    : mMaxCheckpointDirSearchDepth(static_cast<uint32_t>(INT32_FLAG(search_checkpoint_default_dir_depth))) {
}

bool InputFile::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;

    if (!mFileDiscovery.Init(config, *mContext, sName)) {
        return false;
    }

    // EnableContainerDiscovery
    if (!GetOptionalBoolParam(config, "EnableContainerDiscovery", mEnableContainerDiscovery, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              false,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    } else if (mEnableContainerDiscovery && !AppConfig::GetInstance()->IsPurageContainerMode()) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           "iLogtail is not in container, but container discovery is required",
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }

    if (mEnableContainerDiscovery) {
        mFileDiscovery.SetEnableContainerDiscoveryFlag(true);
        if (!mContainerDiscovery.Init(config, *mContext, sName)) {
            return false;
        }
        mContainerDiscovery.GenerateContainerMetaFetchingGoPipeline(optionalGoPipeline, &mFileDiscovery);
    }

    if (!mFileReader.Init(config, *mContext, sName)) {
        return false;
    }

    // 过渡使用
    mFileDiscovery.SetTailingAllMatchedFiles(mFileReader.mTailingAllMatchedFiles);
    mFileDiscovery.SetUpdateContainerInfoFunc(UpdateContainerInfoFunc);
    mFileDiscovery.SetDeleteContainerInfoFunc(DeleteContainerInfo);
    mFileDiscovery.SetIsSameDockerContainerPathFunc(IsSameDockerContainerPath);

    // Multiline
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

    // MaxCheckpointDirSearchDepth
    if (!GetOptionalUIntParam(config, "MaxCheckpointDirSearchDepth", mMaxCheckpointDirSearchDepth, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mMaxCheckpointDirSearchDepth,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // ExactlyOnceConcurrency (param is unintentionally named as EnableExactlyOnce, which should be deprecated in the
    // future)
    uint32_t exactlyOnceConcurrency = 0;
    if (!GetOptionalUIntParam(config, "EnableExactlyOnce", exactlyOnceConcurrency, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mExactlyOnceConcurrency,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    } else if (exactlyOnceConcurrency > static_cast<uint32_t>(INT32_FLAG(max_exactly_once_concurrency))) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              "uint param EnableExactlyOnce is larger than "
                                  + ToString(INT32_FLAG(max_exactly_once_concurrency)),
                              mExactlyOnceConcurrency,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    } else {
        mExactlyOnceConcurrency = exactlyOnceConcurrency;
    }

    return true;
}

bool InputFile::DeleteContainerInfo(const Json::Value& paramsJSON, FileDiscoveryOptions* fileDiscovery) {
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

bool hasPrefix(const std::string& fullString, const std::string& prefix) {
    if (fullString.length() < prefix.length()) {
        return false;
    }
    return fullString.compare(0, prefix.length(), prefix) == 0;
}

void SetContainerPath(DockerContainerPath& dockerContainerPath, const FileDiscoveryOptions* fileDiscovery) {
    std::string logPath;
    if (!fileDiscovery->GetWildcardPaths().empty()) {
        logPath = fileDiscovery->GetWildcardPaths()[0];
    } else {
        logPath = fileDiscovery->GetBasePath();
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
        dockerContainerPath.mContainerPath = STRING_FLAG(default_container_host_path).c_str() + bestMatchedMounts.Source
            + logPath.substr(bestMatchedMounts.Destination.size());
        LOG_DEBUG(sLogger,
                  ("docker container path", dockerContainerPath.mContainerPath)("source", bestMatchedMounts.Source)(
                      "destination", bestMatchedMounts.Destination)("logPath", logPath));
    } else {
        dockerContainerPath.mContainerPath
            = STRING_FLAG(default_container_host_path).c_str() + dockerContainerPath.mUpperDir + logPath;
        LOG_DEBUG(sLogger,
                  ("docker container path",
                   dockerContainerPath.mContainerPath)("upperDir", dockerContainerPath.mUpperDir)("logPath", logPath));
    }
}

bool InputFile::UpdateContainerInfoFunc(const Json::Value& paramsJSON,
                                        bool allFlag,
                                        FileDiscoveryOptions* fileDiscovery) {
    if (!allFlag) {
        DockerContainerPath dockerContainerPath;
        if (!DockerContainerPath::ParseByJSONObj(paramsJSON, dockerContainerPath)) {
            LOG_ERROR(sLogger,
                      ("invalid docker container params", "skip this path")("params", paramsJSON.toStyledString()));
            return false;
        }
        SetContainerPath(dockerContainerPath, fileDiscovery);
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
        SetContainerPath(iter->second, fileDiscovery);
        fileDiscovery->GetContainerInfo()->push_back(iter->second);
    }
    return true;
}

bool InputFile::IsSameDockerContainerPath(const Json::Value& paramsJSON,
                                          bool allFlag,
                                          FileDiscoveryOptions* fileDiscovery) {
    if (!allFlag) {
        DockerContainerPath dockerContainerPath;
        if (!DockerContainerPath::ParseByJSONObj(paramsJSON, dockerContainerPath)) {
            LOG_ERROR(sLogger,
                      ("invalid docker container params", "skip this path")("params", paramsJSON.toStyledString()));
            return true;
        }
        SetContainerPath(dockerContainerPath, fileDiscovery);
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
        SetContainerPath(iter->second, fileDiscovery);
        // need update
        if ((*fileDiscovery->GetContainerInfo())[i] != iter->second) {
            return false;
        }
    }
    // same
    return true;
}
bool InputFile::Start() {
    if (mEnableContainerDiscovery) {
        mFileDiscovery.SetContainerInfo(
            FileServer::GetInstance()->GetAndRemoveContainerInfo(mContext->GetPipeline().Name()));
    }
    FileServer::GetInstance()->AddFileDiscoveryConfig(mContext->GetConfigName(), &mFileDiscovery, mContext);
    FileServer::GetInstance()->AddFileReaderConfig(mContext->GetConfigName(), &mFileReader, mContext);
    FileServer::GetInstance()->AddMultilineConfig(mContext->GetConfigName(), &mMultiline, mContext);
    FileServer::GetInstance()->AddExactlyOnceConcurrency(mContext->GetConfigName(), mExactlyOnceConcurrency);
    return true;
}

bool InputFile::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving && mEnableContainerDiscovery) {
        FileServer::GetInstance()->SaveContainerInfo(mContext->GetPipeline().Name(), mFileDiscovery.GetContainerInfo());
    }
    FileServer::GetInstance()->RemoveFileDiscoveryConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveFileReaderConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveMultilineConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveExactlyOnceConcurrency(mContext->GetConfigName());
    return true;
}

} // namespace logtail
