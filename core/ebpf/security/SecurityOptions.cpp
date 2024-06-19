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

// 之后考虑将这几个函数拆分到input中的各个插件里面
bool InitSecurityFileFilter(const Json::Value& config,
                            SecurityFileFilter& thisFileFilter,
                            const PipelineContext* mContext,
                            const string& sName) {
    string errorMsg;
    for (auto& fileFilterItem : config["Filter"]) {
        SecurityFileFilterItem thisFileFilterItem;
        // FilePath (Mandatory)
        if (!GetMandatoryStringParam(fileFilterItem, "FilePath", thisFileFilterItem.mFilePath, errorMsg)) {
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
        if (!GetOptionalStringParam(fileFilterItem, "FileName", thisFileFilterItem.mFileName, errorMsg)) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 errorMsg,
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        }
        thisFileFilter.mFileFilterItem.emplace_back(thisFileFilterItem);
    }
    return true;
}

bool InitSecurityProcessFilter(const Json::Value& config,
                               SecurityProcessFilter& thisProcessFilter,
                               const PipelineContext* mContext,
                               const string& sName) {
    string errorMsg;
    // NamespaceFilter (Optional)
    if (config.isMember("NamespaceFilter")) {
        if (!config["NamespaceFilter"].isArray()) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 "NamespaceFilter is not of type list",
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        } else {
            for (auto& namespaceFilterConfig : config["NamespaceFilter"]) {
                SecurityProcessNamespaceFilter thisProcessNamespaceFilter;
                // NamespaceType (Mandatory)
                if (!GetMandatoryStringParam(
                        namespaceFilterConfig, "NamespaceType", thisProcessNamespaceFilter.mNamespaceType, errorMsg)
                    || !SecurityOption::IsProcessNamespaceFilterTypeValid(thisProcessNamespaceFilter.mNamespaceType)) {
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
                        namespaceFilterConfig, "ValueList", thisProcessNamespaceFilter.mValueList, errorMsg)) {
                    PARAM_ERROR_RETURN(mContext->GetLogger(),
                                       mContext->GetAlarm(),
                                       errorMsg,
                                       sName,
                                       mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
                }
                thisProcessFilter.mNamespaceFilter.emplace_back(thisProcessNamespaceFilter);
            }
        }
    }

    // NamespaceBlackFilter (Optional)
    if (config.isMember("NamespaceBlackFilter")) {
        // 如果用户两个filter都配置了，不去显式阻塞流水线，但是会打印警告并只执行白名单
        if (config.isMember("NamespaceFilter")) {
            PARAM_WARNING_IGNORE(
                mContext->GetLogger(),
                mContext->GetAlarm(),
                "Both NamespaceFilter and NamespaceBlackFilter are configured, only NamespaceFilter will be executed",
                sName,
                mContext->GetConfigName(),
                mContext->GetProjectName(),
                mContext->GetLogstoreName(),
                mContext->GetRegion());
        } else if (!config["NamespaceBlackFilter"].isArray()) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 "NamespaceBlackFilter is not of type list",
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        } else {
            for (auto& namespaceBlackFilterConfig : config["NamespaceBlackFilter"]) {
                SecurityProcessNamespaceFilter thisProcessNamespaceFilter;
                // NamespaceType (Mandatory)
                if (!GetMandatoryStringParam(namespaceBlackFilterConfig,
                                             "NamespaceType",
                                             thisProcessNamespaceFilter.mNamespaceType,
                                             errorMsg)
                    || !SecurityOption::IsProcessNamespaceFilterTypeValid(thisProcessNamespaceFilter.mNamespaceType)) {
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
                        namespaceBlackFilterConfig, "ValueList", thisProcessNamespaceFilter.mValueList, errorMsg)) {
                    PARAM_ERROR_RETURN(mContext->GetLogger(),
                                       mContext->GetAlarm(),
                                       errorMsg,
                                       sName,
                                       mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
                }
                thisProcessFilter.mNamespaceBlackFilter.emplace_back(thisProcessNamespaceFilter);
            }
        }
    }
    return true;
}

bool InitSecurityNetworkFilter(const Json::Value& config,
                               SecurityNetworkFilter& thisNetworkFilter,
                               const PipelineContext* mContext,
                               const string& sName) {
    string errorMsg;
    // DestAddrList (Optional)
    if (!GetOptionalListParam<string>(config, "DestAddrList", thisNetworkFilter.mDestAddrList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
    }
    // DestPortList (Optional)
    if (!GetOptionalListParam<uint32_t>(config, "DestPortList", thisNetworkFilter.mDestPortList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
    }
    // DestAddrBlackList (Optional)
    if (!GetOptionalListParam<string>(config, "DestAddrBlackList", thisNetworkFilter.mDestAddrBlackList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
    }
    // DestPortBlackList (Optional)
    if (!GetOptionalListParam<uint32_t>(config, "DestPortBlackList", thisNetworkFilter.mDestPortBlackList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
    }
    // SourceAddrList (Optional)
    if (!GetOptionalListParam<string>(config, "SourceAddrList", thisNetworkFilter.mSourceAddrList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
    }
    // SourcePortList (Optional)
    if (!GetOptionalListParam<uint32_t>(config, "SourcePortList", thisNetworkFilter.mSourcePortList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
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
            config, "SourceAddrBlackList", thisNetworkFilter.mSourceAddrBlackList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
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
            config, "SourcePortBlackList", thisNetworkFilter.mSourcePortBlackList, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
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

bool SecurityOption::Init(SecurityFilterType filterType,
                          const Json::Value& config,
                          const PipelineContext* mContext,
                          const string& sName) {
    string errorMsg;
    // CallName (Optional)
    if (!GetOptionalListParam<string>(config, "CallName", mCallName, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
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
            SecurityFileFilter thisFileFilter;
            if (!IsValidList(config, "Filter", errorMsg)) {
                PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                     mContext->GetAlarm(),
                                     errorMsg,
                                     sName,
                                     mContext->GetConfigName(),
                                     mContext->GetProjectName(),
                                     mContext->GetLogstoreName(),
                                     mContext->GetRegion());
            } else {
                if (!InitSecurityFileFilter(config, thisFileFilter, mContext, sName)) {
                    return false;
                }
            }
            mFilter.emplace<SecurityFileFilter>(thisFileFilter);
            break;
        }
        case SecurityFilterType::PROCESS: {
            SecurityProcessFilter thisProcessFilter;
            if (!IsValidMap(config, "Filter", errorMsg)) {
                PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                     mContext->GetAlarm(),
                                     errorMsg,
                                     sName,
                                     mContext->GetConfigName(),
                                     mContext->GetProjectName(),
                                     mContext->GetLogstoreName(),
                                     mContext->GetRegion());
            } else {
                const Json::Value& filterConfig = config["Filter"];
                if (!InitSecurityProcessFilter(filterConfig, thisProcessFilter, mContext, sName)) {
                    return false;
                }
            }
            mFilter.emplace<SecurityProcessFilter>(thisProcessFilter);
            break;
        }
        case SecurityFilterType::NETWORK: {
            SecurityNetworkFilter thisNetworkFilter;
            if (!IsValidMap(config, "Filter", errorMsg)) {
                PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                     mContext->GetAlarm(),
                                     errorMsg,
                                     sName,
                                     mContext->GetConfigName(),
                                     mContext->GetProjectName(),
                                     mContext->GetLogstoreName(),
                                     mContext->GetRegion());
            } else {
                const Json::Value& filterConfig = config["Filter"];
                if (!InitSecurityNetworkFilter(filterConfig, thisNetworkFilter, mContext, sName)) {
                    return false;
                }
            }
            mFilter.emplace<SecurityNetworkFilter>(thisNetworkFilter);
            break;
        }
        default:
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
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
    if (!IsValidList(config, "ConfigList", errorMsg)) {
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
        SecurityOption thisSecurityOption;
        if (!thisSecurityOption.Init(filterType, innerConfig, mContext, sName)) {
            return false;
        }
        mOptionList.emplace_back(thisSecurityOption);
    }
    mFilterType = filterType;
    return true;
}

// todo app_config中定义的进程级别配置获取


} // namespace logtail
