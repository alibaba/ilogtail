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

#include "ebpf/observer/ObserverOptions.h"

#include "common/ParamExtractor.h"

using namespace std;
namespace logtail {

bool InitObserverFileOption(const Json::Value& probeConfig,
                            ObserverFileOption& thisObserverFileOption,
                            const PipelineContext* mContext,
                            const string& sName) {
    string errorMsg;
    // ProfileRemoteServer (Optional)
    if (!GetOptionalStringParam(
            probeConfig, "ProfileRemoteServer", thisObserverFileOption.mProfileRemoteServer, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    // CpuSkipUpload (Optional)
    if (!GetOptionalBoolParam(probeConfig, "CpuSkipUpload", thisObserverFileOption.mCpuSkipUpload, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    // MemSkipUpload (Optional)
    if (!GetOptionalBoolParam(probeConfig, "MemSkipUpload", thisObserverFileOption.mMemSkipUpload, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    return true;
}

bool InitObserverProcessOption(const Json::Value& probeConfig,
                               ObserverProcessOption& thisObserverProcessOption,
                               const PipelineContext* mContext,
                               const string& sName) {
    string errorMsg;
    // IncludeCmdRegex (Optional)
    if (!GetOptionalListParam(probeConfig, "IncludeCmdRegex", thisObserverProcessOption.mIncludeCmdRegex, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    // ExcludeCmdRegex (Optional)
    if (!GetOptionalListParam(probeConfig, "ExcludeCmdRegex", thisObserverProcessOption.mExcludeCmdRegex, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    return true;
}

bool InitObserverNetworkOption(const Json::Value& probeConfig,
                               ObserverNetworkOption& thisObserverNetworkOption,
                               const PipelineContext* mContext,
                               const string& sName) {
    string errorMsg;
    // EnableProtocols (Optional)
    if (!GetOptionalListParam(probeConfig, "EnableProtocols", thisObserverNetworkOption.mEnableProtocols, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    // EnableProtocols (Optional)
    if (!GetOptionalBoolParam(
            probeConfig, "DisableProtocolParse", thisObserverNetworkOption.mDisableProtocolParse, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    // DisableConnStats (Optional)
    if (!GetOptionalBoolParam(probeConfig, "DisableConnStats", thisObserverNetworkOption.mDisableConnStats, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    // EnableConnTrackerDump (Optional)
    if (!GetOptionalBoolParam(
            probeConfig, "EnableConnTrackerDump", thisObserverNetworkOption.mEnableConnTrackerDump, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    return true;
}

bool ObserverOptions::Init(ObserverType type,
                           const Json::Value& config,
                           const PipelineContext* mContext,
                           const string& sName) {
    string errorMsg;
    mType = type;
    // ProbeConfig (Mandatory)
    if (!IsValidMap(config, "ProbeConfig", errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    const Json::Value& probeConfig = config["ProbeConfig"];
    switch (type) {
        case ObserverType::NETWORK: {
            ObserverNetworkOption thisObserverNetworkOption;
            if (!InitObserverNetworkOption(probeConfig, thisObserverNetworkOption, mContext, sName)) {
                return false;
            }
            mObserverOption.emplace<ObserverNetworkOption>(thisObserverNetworkOption);
            break;
        }
        case ObserverType::FILE: {
            ObserverFileOption thisObserverFileOption;
            if (!InitObserverFileOption(probeConfig, thisObserverFileOption, mContext, sName)) {
                return false;
            }
            mObserverOption.emplace<ObserverFileOption>(thisObserverFileOption);
            break;
        }
        case ObserverType::PROCESS: {
            ObserverProcessOption thisObserverProcessOption;
            if (!InitObserverProcessOption(probeConfig, thisObserverProcessOption, mContext, sName)) {
                return false;
            }
            mObserverOption.emplace<ObserverProcessOption>(thisObserverProcessOption);
            break;
        }
        default:
            break;
    }
    return true;
}

} // namespace logtail
