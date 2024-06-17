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

bool ObserverOptions::Init(ObserverType type,
                           const Json::Value& config,
                           const PipelineContext* mContext,
                           const string& sName) {
    string errorMsg;
    mType = type;
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
            ObserverNetworkOption thisObserverNetwork;
            // EnableProtocols (Mandatory)
            if (!GetOptionalListParam(probeConfig, "EnableProtocols", thisObserverNetwork.mEnableProtocols, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // EnableProtocols (Mandatory)
            if (!GetOptionalBoolParam(
                    probeConfig, "DisableProtocolParse", thisObserverNetwork.mDisableProtocolParse, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // DisableConnStats (Mandatory)
            if (!GetOptionalBoolParam(
                    probeConfig, "DisableConnStats", thisObserverNetwork.mDisableConnStats, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // EnableConnTrackerDump (Mandatory)
            if (!GetOptionalBoolParam(
                    probeConfig, "EnableConnTrackerDump", thisObserverNetwork.mEnableConnTrackerDump, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            mObserverOption.emplace<ObserverNetworkOption>(thisObserverNetwork);
            break;
        }
        case ObserverType::FILE: {
            ObserverFileOption thisObserverFile;
            // ProfileRemoteServer (Mandatory)
            if (!GetOptionalStringParam(
                    probeConfig, "ProfileRemoteServer", thisObserverFile.mProfileRemoteServer, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // CpuSkipUpload (Mandatory)
            if (!GetOptionalBoolParam(probeConfig, "CpuSkipUpload", thisObserverFile.mCpuSkipUpload, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // MemSkipUpload (Mandatory)
            if (!GetOptionalBoolParam(probeConfig, "MemSkipUpload", thisObserverFile.mMemSkipUpload, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            mObserverOption.emplace<ObserverFileOption>(thisObserverFile);
            break;
        }
        case ObserverType::PROCESS: {
            ObserverProcessOption thisObserverProcess;
            // IncludeCmdRegex (Mandatory)
            if (!GetOptionalListParam(probeConfig, "IncludeCmdRegex", thisObserverProcess.mIncludeCmdRegex, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // ExcludeCmdRegex (Mandatory)
            if (!GetOptionalListParam(probeConfig, "ExcludeCmdRegex", thisObserverProcess.mExcludeCmdRegex, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            mObserverOption.emplace<ObserverProcessOption>(thisObserverProcess);
            break;
        }
        default:
            break;
    }
    return true;
}

} // namespace logtail
