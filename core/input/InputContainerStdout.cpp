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

#include "input/InputContainerStdout.h"

#include "app_config/AppConfig.h"
#include "common/ParamExtractor.h"
#include "file_server/FileServer.h"
#include "pipeline/Pipeline.h"

using namespace std;

namespace logtail {

const string InputContainerStdout::sName = "input_container_stdout";

bool InputContainerStdout::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;
    // EnableContainerDiscovery
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
        fileDiscoveryConfig["FilePaths"].append("/**/*");
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

    if (!mContainerDiscovery.Init(config, *mContext, sName)) {
        return false;
    }
    mContainerDiscovery.GenerateContainerMetaFetchingGoPipeline(optionalGoPipeline);

    if (!mFileReader.Init(config, *mContext, sName)) {
        return false;
    }

    // Multiline
    {
        string key = "Multiline";
        const Json::Value* itr = config.find(key.c_str(), key.c_str() + key.length());
        if (itr != nullptr) {
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
    return true;
}

bool InputContainerStdout::Start() {
    mFileDiscovery.SetContainerInfo(
        FileServer::GetInstance()->GetAndRemoveContainerInfo(mContext->GetPipeline().Name()));
    FileServer::GetInstance()->AddFileDiscoveryConfig(mContext->GetConfigName(), &mFileDiscovery, mContext);
    FileServer::GetInstance()->AddFileReaderConfig(mContext->GetConfigName(), &mFileReader, mContext);
    FileServer::GetInstance()->AddMultilineConfig(mContext->GetConfigName(), &mMultiline, mContext);
    return true;
}

bool InputContainerStdout::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        FileServer::GetInstance()->SaveContainerInfo(mContext->GetPipeline().Name(), mFileDiscovery.GetContainerInfo());
    }
    FileServer::GetInstance()->RemoveFileDiscoveryConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveFileReaderConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveMultilineConfig(mContext->GetConfigName());
    return true;
}

} // namespace logtail
