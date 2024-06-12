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

#include "ebpf/security/SecurityOptions.h"

#include "common/ParamExtractor.h"

using namespace std;
namespace logtail {

bool SecurityOption::Init(SecurityFilterType filterType,
                          const Json::Value& config,
                          const PipelineContext* mContext,
                          const string& sName) {
    string errorMsg;
    // CallName (Mandatory)
    if (!GetOptionalListParam<string>(config, "CallName", mCallName, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }

    // Filter
    switch (filterType) {
        case SecurityFilterType::FILE: {
            if (!IsListValid(config, "Filter", errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            SecurityFileFilter* thisFileFilter = new SecurityFileFilter();
            mFilter = thisFileFilter;
            // FilterType
            mFilter->mFilterType = filterType;
            for (auto& fileFilterItem : config["Filter"]) {
                SecurityFileFilterItem* thisFileFilterItem = new SecurityFileFilterItem();
                // FilePath (Mandatory)
                if (!GetMandatoryStringParam(fileFilterItem, "FilePath", thisFileFilterItem->mFilePath, errorMsg)) {
                    PARAM_ERROR_RETURN(mContext->GetLogger(),
                                       mContext->GetAlarm(),
                                       errorMsg,
                                       sName,
                                       mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
                }
                // FileName (Optional)
                if (!GetOptionalStringParam(fileFilterItem, "FileName", thisFileFilterItem->mFileName, errorMsg)) {
                    PARAM_ERROR_RETURN(mContext->GetLogger(),
                                       mContext->GetAlarm(),
                                       errorMsg,
                                       sName,
                                       mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
                }
                thisFileFilter->mFileFilterItem.emplace_back(thisFileFilterItem);
            }
            break;
        }
        case SecurityFilterType::PROCESS: {
            SecurityProcessFilter* thisProcessFilter = new SecurityProcessFilter();
            mFilter = thisProcessFilter;
            // FilterType
            mFilter->mFilterType = filterType;
            if (!IsMapValid(config, "Filter", errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            const Json::Value& filterConfig = config["Filter"];
            // NamespaceFilter (Optional)
            if (filterConfig.isMember("NamespaceFilter")) {
                if (!filterConfig["NamespaceFilter"].isArray()) {
                    PARAM_ERROR_RETURN(mContext->GetLogger(),
                                       mContext->GetAlarm(),
                                       "NamespaceFilter is not of type list",
                                       sName,
                                       mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
                }
                for (auto& namespaceFilterConfig : filterConfig["NamespaceFilter"]) {
                    SecurityProcessNamespaceFilter* thisProcessNamespaceFilter = new SecurityProcessNamespaceFilter();
                    // Type (Mandatory)
                    if (!GetMandatoryStringParam(
                            namespaceFilterConfig, "Type", thisProcessNamespaceFilter->mType, errorMsg)
                        || !IsProcessNamespaceFilterTypeValid(thisProcessNamespaceFilter->mType)) {
                        PARAM_ERROR_RETURN(mContext->GetLogger(),
                                           mContext->GetAlarm(),
                                           errorMsg,
                                           sName,
                                           mContext->GetConfigName(),
                                           mContext->GetProjectName(),
                                           mContext->GetLogstoreName(),
                                           mContext->GetRegion());
                    }
                    // ValueList (Mandatory)
                    if (!GetMandatoryListParam<string>(
                            namespaceFilterConfig, "ValueList", thisProcessNamespaceFilter->mValueList, errorMsg)) {
                        PARAM_ERROR_RETURN(mContext->GetLogger(),
                                           mContext->GetAlarm(),
                                           errorMsg,
                                           sName,
                                           mContext->GetConfigName(),
                                           mContext->GetProjectName(),
                                           mContext->GetLogstoreName(),
                                           mContext->GetRegion());
                    }
                    thisProcessFilter->mNamespaceFilter.emplace_back(thisProcessNamespaceFilter);
                }
                if (filterConfig.isMember("NamespaceBlackFilter")) {
                    PARAM_ERROR_RETURN(mContext->GetLogger(),
                                       mContext->GetAlarm(),
                                       "NamespaceFilter and NamespaceBlackFilter cannot be set at the same time",
                                       sName,
                                       mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
                }
            }

            // NamespaceBlackFilter (Optional)
            if (filterConfig.isMember("NamespaceBlackFilter")) {
                if (!filterConfig["NamespaceBlackFilter"].isArray()) {
                    PARAM_ERROR_RETURN(mContext->GetLogger(),
                                       mContext->GetAlarm(),
                                       "NamespaceBlackFilter is not of type list",
                                       sName,
                                       mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
                }
                for (auto& namespaceBlackFilterConfig : filterConfig["NamespaceBlackFilter"]) {
                    SecurityProcessNamespaceFilter* thisProcessNamespaceFilter = new SecurityProcessNamespaceFilter();
                    // Type (Mandatory)
                    if (!GetMandatoryStringParam(
                            namespaceBlackFilterConfig, "Type", thisProcessNamespaceFilter->mType, errorMsg)
                        || !IsProcessNamespaceFilterTypeValid(thisProcessNamespaceFilter->mType)) {
                        PARAM_ERROR_RETURN(mContext->GetLogger(),
                                           mContext->GetAlarm(),
                                           errorMsg,
                                           sName,
                                           mContext->GetConfigName(),
                                           mContext->GetProjectName(),
                                           mContext->GetLogstoreName(),
                                           mContext->GetRegion());
                    }
                    // ValueList (Mandatory)
                    if (!GetMandatoryListParam<string>(namespaceBlackFilterConfig,
                                                       "ValueList",
                                                       thisProcessNamespaceFilter->mValueList,
                                                       errorMsg)) {
                        PARAM_ERROR_RETURN(mContext->GetLogger(),
                                           mContext->GetAlarm(),
                                           errorMsg,
                                           sName,
                                           mContext->GetConfigName(),
                                           mContext->GetProjectName(),
                                           mContext->GetLogstoreName(),
                                           mContext->GetRegion());
                    }
                    thisProcessFilter->mNamespaceBlackFilter.emplace_back(thisProcessNamespaceFilter);
                }
            }
            break;
        }
        case SecurityFilterType::NETWORK: {
            SecurityNetworkFilter* thisNetWorkFilter = new SecurityNetworkFilter();
            mFilter = thisNetWorkFilter;
            // FilterType
            mFilter->mFilterType = filterType;

            if (!IsMapValid(config, "Filter", errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            const Json::Value& filterConfig = config["Filter"];
            // DestAddrList (Optional)
            if (!GetOptionalListParam<string>(
                    filterConfig, "DestAddrList", thisNetWorkFilter->mDestAddrList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // DestPortList (Optional)
            if (!GetOptionalListParam<uint32_t>(
                    filterConfig, "DestPortList", thisNetWorkFilter->mDestPortList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // DestAddrBlackList (Optional)
            if (!GetOptionalListParam<string>(
                    filterConfig, "DestAddrBlackList", thisNetWorkFilter->mDestAddrBlackList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // DestPortBlackList (Optional)
            if (!GetOptionalListParam<uint32_t>(
                    filterConfig, "DestPortBlackList", thisNetWorkFilter->mDestPortBlackList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // SourceAddrList (Optional)
            if (!GetOptionalListParam<string>(
                    filterConfig, "SourceAddrList", thisNetWorkFilter->mSourceAddrList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // SourcePortList (Optional)
            if (!GetOptionalListParam<uint32_t>(
                    filterConfig, "SourcePortList", thisNetWorkFilter->mSourcePortList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // SourceAddrBlackList (Optional)
            if (!GetOptionalListParam<string>(
                    filterConfig, "SourceAddrBlackList", thisNetWorkFilter->mSourceAddrBlackList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            // SourcePortBlackList (Optional)
            if (!GetOptionalListParam<uint32_t>(
                    filterConfig, "SourcePortBlackList", thisNetWorkFilter->mSourcePortBlackList, errorMsg)) {
                PARAM_ERROR_RETURN(mContext->GetLogger(),
                                   mContext->GetAlarm(),
                                   errorMsg,
                                   sName,
                                   mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
            }
            break;
        }
        default:
            PARAM_ERROR_RETURN(mContext->GetLogger(),
                               mContext->GetAlarm(),
                               "Unknown filter type",
                               sName,
                               mContext->GetConfigName(),
                               mContext->GetProjectName(),
                               mContext->GetLogstoreName(),
                               mContext->GetRegion());
    }

    return true;
}

bool SecurityOption::IsProcessNamespaceFilterTypeValid(string type) {
    unordered_set<string> dic
        = {"Uts", "Ipc", "Mnt", "Pid", "PidForChildren", "Net", "Cgroup", "User", "Time", "TimeForChildren"};
    return dic.find(type) != dic.end();
}


bool SecurityOptions::Init(SecurityFilterType filterType,
                           const Json::Value& config,
                           const PipelineContext* mContext,
                           const string& sName) {
    string errorMsg;
    // ConfigList (Mandatory)
    if (!IsListValid(config, "ConfigList", errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    for (auto& innerConfig : config["ConfigList"]) {
        SecurityOption* thisSecurityOption = new SecurityOption();
        if (!thisSecurityOption->Init(filterType, innerConfig, mContext, sName)) {
            return false;
        }
        mOptionList.emplace_back(thisSecurityOption);
    }
    return true;
}

// todo app_config中定义的进程级别配置获取


} // namespace logtail
