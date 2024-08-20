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

#include <string>
#include <unordered_set>

#include "logger/Logger.h"
#include "ebpf/config.h"
#include "common/ParamExtractor.h"
#include "common/Flags.h"

DEFINE_FLAG_INT32(ebpf_receive_event_chan_cap, "ebpf receive kernel event queue size", 4096);
DEFINE_FLAG_BOOL(ebpf_admin_config_debug_mode, "ebpf admin config debug mode", false);
DEFINE_FLAG_STRING(ebpf_admin_config_log_level, "ebpf admin config log level", "warn");
DEFINE_FLAG_BOOL(ebpf_admin_config_push_all_span, "if admin config push all span", false);
DEFINE_FLAG_INT32(ebpf_aggregation_config_agg_window_second, "ebpf data aggregation window time", 15);
DEFINE_FLAG_STRING(ebpf_converage_config_strategy, "ebpf converage strategy", "combine");
DEFINE_FLAG_STRING(ebpf_sample_config_strategy, "ebpf sample strategy", "fixedRate");
DEFINE_FLAG_DOUBLE(ebpf_sample_config_config_rate, "ebpf sample rate", 0.01);
DEFINE_FLAG_INT32(ebpf_socket_probe_config_slow_request_threshold_ms, "ebpf socket probe slow request threshold", 500);
DEFINE_FLAG_INT32(ebpf_socket_probe_config_max_conn_trackers, "ebpf socket probe max connect trackers", 10000);
DEFINE_FLAG_INT32(ebpf_socket_probe_config_max_band_width_mb_per_sec, "ebpf socket probe max bandwidth per sec", 30);
DEFINE_FLAG_INT32(ebpf_socket_probe_config_max_raw_record_per_sec, "ebpf socket probe max raw record per sec", 100000);
DEFINE_FLAG_INT32(ebpf_profile_probe_config_profile_sample_rate, "ebpf profile probe profile sample rate", 10);
DEFINE_FLAG_INT32(ebpf_profile_probe_config_profile_upload_duration, "ebpf profile probe profile upload duration", 10);
DEFINE_FLAG_BOOL(ebpf_process_probe_config_enable_oom_detect, "if ebpf process probe enable oom detect", false);

namespace logtail {
namespace ebpf {

static const std::unordered_map<SecurityProbeType, std::unordered_set<std::string>> callNameDict
    = {{SecurityProbeType::PROCESS,
        {"sys_enter_execve", "sys_enter_clone", "disassociate_ctty", "acct_process", "wake_up_new_task"}},
       {SecurityProbeType::FILE, {"security_file_permission", "security_file_mmap", "security_path_truncate"}},
       {SecurityProbeType::NETWORK, {"tcp_connect", "tcp_close", "tcp_sendmsg"}}};

bool InitObserverNetworkOptionInner(const Json::Value& probeConfig,
                               nami::ObserverNetworkOption& thisObserverNetworkOption,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    std::string errorMsg;
    // MeterHandlerType (Optional)
    if (!GetOptionalStringParam(probeConfig, "MeterHandlerType", thisObserverNetworkOption.mMeterHandlerType, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
    }
    // SpanHandlerType (Optional)
    if (!GetOptionalStringParam(probeConfig, "SpanHandlerType", thisObserverNetworkOption.mSpanHandlerType, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
    }

    // EnableProtocols (Optional)
    if (!GetOptionalListParam(probeConfig, "EnableProtocols", thisObserverNetworkOption.mEnableProtocols, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
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
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              false,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    // DisableConnStats (Optional)
    if (!GetOptionalBoolParam(probeConfig, "DisableConnStats", thisObserverNetworkOption.mDisableConnStats, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              false,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    // EnableConnTrackerDump (Optional)
    if (!GetOptionalBoolParam(
            probeConfig, "EnableConnTrackerDump", thisObserverNetworkOption.mEnableConnTrackerDump, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              false,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    return true;
}

bool ExtractProbeConfig(const Json::Value& config, const PipelineContext* mContext, const std::string& sName, Json::Value& probeConfig) {
    std::string errorMsg;
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
    probeConfig = config["ProbeConfig"];
    return true;
}

bool InitObserverNetworkOption(const Json::Value& config, 
                               nami::ObserverNetworkOption& thisObserverNetworkOption,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    Json::Value probeConfig;
    if (!ExtractProbeConfig(config, mContext, sName, probeConfig)) {
        return false;
    }

    return InitObserverNetworkOptionInner(probeConfig, thisObserverNetworkOption, mContext, sName);
}

bool InitSecurityFileFilter(const Json::Value& config,
                            nami::SecurityFileFilter& thisFileFilter,
                            const PipelineContext* mContext,
                            const std::string& sName) {
    std::string errorMsg;
    // FilePathFilter (Optional)
    if (!config.isMember("FilePathFilter")) {
        return false;
    } else if (!config["FilePathFilter"].isArray()) {
        errorMsg = "FilePathFilter is not of type list";
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             errorMsg,
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
        return false;
    } else {
        if (!GetOptionalListFilterParam<std::string>(
                config, "FilePathFilter", thisFileFilter.mFilePathList, errorMsg)) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 errorMsg,
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        }
    }
    return true;
}

bool InitSecurityNetworkFilter(const Json::Value& config,
                               nami::SecurityNetworkFilter& thisNetworkFilter,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    std::string errorMsg;
    // AddrFilter (Optional)
    if (!config.isMember("AddrFilter")) {
        return false;
    } else if (!config["AddrFilter"].isObject()) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             "AddrFilter is not of type map",
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
        return false;
    } else {
        auto addrFilterConfig = config["AddrFilter"];
        // DestAddrList (Optional)
        if (!GetOptionalListFilterParam<std::string>(
                addrFilterConfig, "DestAddrList", thisNetworkFilter.mDestAddrList, errorMsg)) {
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
        if (!GetOptionalListFilterParam<uint32_t>(
                addrFilterConfig, "DestPortList", thisNetworkFilter.mDestPortList, errorMsg)) {
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
        if (!GetOptionalListFilterParam<std::string>(
                addrFilterConfig, "DestAddrBlackList", thisNetworkFilter.mDestAddrBlackList, errorMsg)) {
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
        if (!GetOptionalListFilterParam<uint32_t>(
                addrFilterConfig, "DestPortBlackList", thisNetworkFilter.mDestPortBlackList, errorMsg)) {
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
        if (!GetOptionalListFilterParam<std::string>(
                addrFilterConfig, "SourceAddrList", thisNetworkFilter.mSourceAddrList, errorMsg)) {
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
        if (!GetOptionalListFilterParam<uint32_t>(
                addrFilterConfig, "SourcePortList", thisNetworkFilter.mSourcePortList, errorMsg)) {
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
        if (!GetOptionalListFilterParam<std::string>(
                addrFilterConfig, "SourceAddrBlackList", thisNetworkFilter.mSourceAddrBlackList, errorMsg)) {
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
        if (!GetOptionalListFilterParam<uint32_t>(
                addrFilterConfig, "SourcePortBlackList", thisNetworkFilter.mSourcePortBlackList, errorMsg)) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 errorMsg,
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        }
    }
    return true;
}

bool IsProcessNamespaceFilterTypeValid(const std::string& type) {
    const std::unordered_set<std::string> dic
        = {"Uts", "Ipc", "Mnt", "Pid", "PidForChildren", "Net", "Cgroup", "User", "Time", "TimeForChildren"};
    return dic.find(type) != dic.end();
}

bool GetValidSecurityProbeCallName(SecurityProbeType type, std::vector<std::string>& callNames, std::string& errorMsg) {
    if (type >= SecurityProbeType::MAX) {
        errorMsg = "Invalid security eBPF probe type";
        return false;
    }
    std::vector<std::string> survivedCallNames;
    bool res = true;
    for (auto& callName : callNames) {
        if (callNameDict.at(type).find(callName) == callNameDict.at(type).end()) {
            if (!res) {
                errorMsg += ", " + callName;
            } else {
                errorMsg = "Invalid callnames for security eBPF probe: " + callName;
                res = false;
            }
        } else {
            survivedCallNames.emplace_back(callName);
        }
    }
    callNames.swap(survivedCallNames);
    return res;
}

void SetSecurityProbeDefaultCallName(SecurityProbeType type, std::vector<std::string>& callNames) {
    callNames.assign(callNameDict.at(type).begin(), callNameDict.at(type).end());
}

bool InitCallNameFilter(const Json::Value& config,
                        std::vector<std::string>& callNames,
                        const PipelineContext* mContext,
                        const std::string& sName,
                        SecurityProbeType probeType) {
    std::string errorMsg;
    // CallNameFilter (Optional)
    if (!config.isMember("CallNameFilter")) {
        SetSecurityProbeDefaultCallName(probeType, callNames);
    } else if (!config["CallNameFilter"].isArray()) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
                             mContext->GetAlarm(),
                             "CallNameFilter is not of type list",
                             sName,
                             mContext->GetConfigName(),
                             mContext->GetProjectName(),
                             mContext->GetLogstoreName(),
                             mContext->GetRegion());
        SetSecurityProbeDefaultCallName(probeType, callNames);
    } else {
        if (!GetOptionalListFilterParam<std::string>(config, "CallNameFilter", callNames, errorMsg)) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 errorMsg,
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        }
        if (!GetValidSecurityProbeCallName(probeType, callNames, errorMsg)) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 errorMsg,
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        }
        if (callNames.empty()) {
            SetSecurityProbeDefaultCallName(probeType, callNames);
        }
    }
    return true;
}

// Merge the same callname in different options of probeconfig
void Merge(SecurityProbeType type,
           nami::SecurityOption& option,
           std::variant<std::monostate, nami::SecurityFileFilter, nami::SecurityNetworkFilter> filter) {
    if (std::holds_alternative<std::monostate>(filter)) {
        return;
    }
    if (std::holds_alternative<std::monostate>(option.filter_)) {
        option.filter_.swap(filter);
        return;
    }
    switch (type) {
        case SecurityProbeType::FILE: {
            auto thisFileFilter = std::get_if<nami::SecurityFileFilter>(&option.filter_);
            auto newFileFilter = std::get_if<nami::SecurityFileFilter>(&filter);
            thisFileFilter->mFilePathList.insert(thisFileFilter->mFilePathList.end(),
                                                 newFileFilter->mFilePathList.begin(),
                                                 newFileFilter->mFilePathList.end());
        }
        case SecurityProbeType::NETWORK: {
            auto thisNetworkFilter = std::get_if<nami::SecurityNetworkFilter>(&option.filter_);
            auto newNetworkFilter = std::get_if<nami::SecurityNetworkFilter>(&filter);
            thisNetworkFilter->mDestAddrList.insert(thisNetworkFilter->mDestAddrList.end(),
                                                    newNetworkFilter->mDestAddrList.begin(),
                                                    newNetworkFilter->mDestAddrList.end());
            thisNetworkFilter->mDestPortList.insert(thisNetworkFilter->mDestPortList.end(),
                                                    newNetworkFilter->mDestPortList.begin(),
                                                    newNetworkFilter->mDestPortList.end());
            thisNetworkFilter->mDestAddrBlackList.insert(thisNetworkFilter->mDestAddrBlackList.end(),
                                                         newNetworkFilter->mDestAddrBlackList.begin(),
                                                         newNetworkFilter->mDestAddrBlackList.end());
            thisNetworkFilter->mDestPortBlackList.insert(thisNetworkFilter->mDestPortBlackList.end(),
                                                         newNetworkFilter->mDestPortBlackList.begin(),
                                                         newNetworkFilter->mDestPortBlackList.end());
            thisNetworkFilter->mSourceAddrList.insert(thisNetworkFilter->mSourceAddrList.end(),
                                                      newNetworkFilter->mSourceAddrList.begin(),
                                                      newNetworkFilter->mSourceAddrList.end());
            thisNetworkFilter->mSourcePortList.insert(thisNetworkFilter->mSourcePortList.end(),
                                                      newNetworkFilter->mSourcePortList.begin(),
                                                      newNetworkFilter->mSourcePortList.end());
            thisNetworkFilter->mSourceAddrBlackList.insert(thisNetworkFilter->mSourceAddrBlackList.end(),
                                                           newNetworkFilter->mSourceAddrBlackList.begin(),
                                                           newNetworkFilter->mSourceAddrBlackList.end());
            thisNetworkFilter->mSourcePortBlackList.insert(thisNetworkFilter->mSourcePortBlackList.end(),
                                                           newNetworkFilter->mSourcePortBlackList.begin(),
                                                           newNetworkFilter->mSourcePortBlackList.end());
            break;
        }
        case SecurityProbeType::PROCESS: {
            break;
        }
        default:
            break;
    }
}

bool SecurityOptions::Init(SecurityProbeType probeType,
                           const Json::Value& config,
                           const PipelineContext* mContext,
                           const std::string& sName) {
    std::string errorMsg;

    // ProbeConfig (Mandatory)
    if (!IsValidList(config, "ProbeConfig", errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    std::unordered_map<std::string, int> thisOptionMap;
    for (auto& innerConfig : config["ProbeConfig"]) {
        // Genral Filter (Optional)
        std::variant<std::monostate, nami::SecurityFileFilter, nami::SecurityNetworkFilter> thisFilter;
        switch (probeType) {
            case SecurityProbeType::FILE: {
                nami::SecurityFileFilter thisFileFilter;
                if (InitSecurityFileFilter(innerConfig, thisFileFilter, mContext, sName)) {
                    thisFilter.emplace<nami::SecurityFileFilter>(thisFileFilter);
                }
                break;
            }
            case SecurityProbeType::NETWORK: {
                nami::SecurityNetworkFilter thisNetworkFilter;
                if (InitSecurityNetworkFilter(innerConfig, thisNetworkFilter, mContext, sName)) {
                    thisFilter.emplace<nami::SecurityNetworkFilter>(thisNetworkFilter);
                }
                break;
            }
            case SecurityProbeType::PROCESS: {
                break;
            }
            default:
                PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                     mContext->GetAlarm(),
                                     "Unknown security eBPF probe type",
                                     sName,
                                     mContext->GetConfigName(),
                                     mContext->GetProjectName(),
                                     mContext->GetLogstoreName(),
                                     mContext->GetRegion());
        }
        // CallNameFilter (Optional)
        std::vector<std::string> callNames;
        InitCallNameFilter(innerConfig, callNames, mContext, sName, probeType);
        // Merge the same callname in different options of probeconfig
        for (auto& callName : callNames) {
            if (thisOptionMap.find(callName) == thisOptionMap.end()) {
                nami::SecurityOption thisSecurityOption;
                thisSecurityOption.call_names_.emplace_back(callName);
                thisSecurityOption.filter_ = thisFilter;
                mOptionList.emplace_back(thisSecurityOption);
                thisOptionMap[callName] = mOptionList.size() - 1;
            } else {
                Merge(probeType, mOptionList[thisOptionMap[callName]], thisFilter);
            }
        }
    }
    mProbeType = probeType;
    return true;
}

//////
void eBPFAdminConfig::LoadEbpfConfig(const Json::Value& confJson) {
    // receive_event_chan_cap (Optional)
    mReceiveEventChanCap = INT32_FLAG(ebpf_receive_event_chan_cap);
    // admin_config (Optional)
    mAdminConfig = AdminConfig{BOOL_FLAG(ebpf_admin_config_debug_mode), STRING_FLAG(ebpf_admin_config_log_level), BOOL_FLAG(ebpf_admin_config_push_all_span)};
    // aggregation_config (Optional)
    mAggregationConfig = AggregationConfig{INT32_FLAG(ebpf_aggregation_config_agg_window_second)};
    // converage_config (Optional)
    mConverageConfig = ConverageConfig{STRING_FLAG(ebpf_converage_config_strategy)};
    // sample_config (Optional)
    mSampleConfig = SampleConfig{STRING_FLAG(ebpf_sample_config_strategy), {DOUBLE_FLAG(ebpf_sample_config_config_rate)}};
    // socket_probe_config (Optional)
    mSocketProbeConfig = SocketProbeConfig{INT32_FLAG(ebpf_socket_probe_config_slow_request_threshold_ms), INT32_FLAG(ebpf_socket_probe_config_max_conn_trackers), INT32_FLAG(ebpf_socket_probe_config_max_band_width_mb_per_sec), INT32_FLAG(ebpf_socket_probe_config_max_raw_record_per_sec)};
    // profile_probe_config (Optional)
    mProfileProbeConfig = ProfileProbeConfig{INT32_FLAG(ebpf_profile_probe_config_profile_sample_rate), INT32_FLAG(ebpf_profile_probe_config_profile_upload_duration)};
    // process_probe_config (Optional)
    mProcessProbeConfig = ProcessProbeConfig{BOOL_FLAG(ebpf_process_probe_config_enable_oom_detect)};
}

} // ebpf
} // logtail
