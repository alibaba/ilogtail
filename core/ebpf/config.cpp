#include <string>
#include <unordered_set>

#include "logger/Logger.h"
#include "ebpf/config.h"
#include "common/ParamExtractor.h"

namespace logtail {
namespace ebpf {
//////
bool IsProcessNamespaceFilterTypeValid(const std::string& type);

bool InitObserverFileOptionInner(const Json::Value& probeConfig,
                            nami::ObserverFileOption& thisObserverFileOption,
                            const PipelineContext* mContext,
                            const std::string& sName) {
    std::string errorMsg;
    // ProfileRemoteServer (Optional)
    if (!GetOptionalStringParam(
            probeConfig, "ProfileRemoteServer", thisObserverFileOption.mProfileRemoteServer, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
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
    // MemSkipUpload (Optional)
    if (!GetOptionalBoolParam(probeConfig, "MemSkipUpload", thisObserverFileOption.mMemSkipUpload, errorMsg)) {
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

bool InitObserverProcessOptionInner(const Json::Value& probeConfig,
                               nami::ObserverProcessOption& thisObserverProcessOption,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    std::string errorMsg;
    // IncludeCmdRegex (Optional)
    if (!GetOptionalListParam(probeConfig, "IncludeCmdRegex", thisObserverProcessOption.mIncludeCmdRegex, errorMsg)) {
        PARAM_WARNING_IGNORE(mContext->GetLogger(),
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

bool InitObserverNetworkOptionInner(const Json::Value& probeConfig,
                               nami::ObserverNetworkOption& thisObserverNetworkOption,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    std::string errorMsg;
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

bool InitObserverFileOption(const Json::Value& config,
                            nami::ObserverFileOption& thisObserverFileOption,
                            const PipelineContext* mContext,
                            const std::string& sName) {
    Json::Value probeConfig;
    if (!ExtractProbeConfig(config, mContext, sName, probeConfig)) {
        return false;
    }

    return InitObserverFileOptionInner(probeConfig, thisObserverFileOption, mContext, sName);

}
bool InitObserverProcessOption(const Json::Value& config, 
                               nami::ObserverProcessOption& thisObserverProcessOption,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    Json::Value probeConfig;
    if (!ExtractProbeConfig(config, mContext, sName, probeConfig)) {
        return false;
    }

    return InitObserverProcessOptionInner(probeConfig, thisObserverProcessOption, mContext, sName);
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

bool ObserverOptions::Init(ObserverType type,
                           const Json::Value& config,
                           const PipelineContext* mContext,
                           const std::string& sName) {
    std::string errorMsg;
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
            nami::ObserverNetworkOption thisObserverNetworkOption;
            if (!InitObserverNetworkOptionInner(probeConfig, thisObserverNetworkOption, mContext, sName)) {
                return false;
            }
            mObserverOption.emplace<nami::ObserverNetworkOption>(thisObserverNetworkOption);
            break;
        }
        case ObserverType::FILE: {
            nami::ObserverFileOption thisObserverFileOption;
            if (!InitObserverFileOptionInner(probeConfig, thisObserverFileOption, mContext, sName)) {
                return false;
            }
            mObserverOption.emplace<nami::ObserverFileOption>(thisObserverFileOption);
            break;
        }
        case ObserverType::PROCESS: {
            nami::ObserverProcessOption thisObserverProcessOption;
            if (!InitObserverProcessOptionInner(probeConfig, thisObserverProcessOption, mContext, sName)) {
                return false;
            }
            mObserverOption.emplace<nami::ObserverProcessOption>(thisObserverProcessOption);
            break;
        }
        default:
            break;
    }
    return true;
}

//////
// 之后考虑将这几个函数拆分到input中的各个插件里面
bool InitSecurityFileFilter(const Json::Value& config,
                            nami::SecurityFileFilter& thisFileFilter,
                            const PipelineContext* mContext,
                            const std::string& sName) {
    std::string errorMsg;
    for (auto& fileFilterItem : config["Filter"]) {
        nami::SecurityFileFilterItem thisFileFilterItem;
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
                               nami::SecurityProcessFilter& thisProcessFilter,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    std::string errorMsg;
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
                nami::SecurityProcessNamespaceFilter thisProcessNamespaceFilter;
                // NamespaceType (Mandatory)
                if (!GetMandatoryStringParam(
                        namespaceFilterConfig, "NamespaceType", thisProcessNamespaceFilter.mNamespaceType, errorMsg)
                    || !IsProcessNamespaceFilterTypeValid(thisProcessNamespaceFilter.mNamespaceType)) {
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
                if (!GetMandatoryListParam<std::string>(
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
                nami::SecurityProcessNamespaceFilter thisProcessNamespaceFilter;
                // NamespaceType (Mandatory)
                if (!GetMandatoryStringParam(namespaceBlackFilterConfig,
                                             "NamespaceType",
                                             thisProcessNamespaceFilter.mNamespaceType,
                                             errorMsg)
                    || !IsProcessNamespaceFilterTypeValid(thisProcessNamespaceFilter.mNamespaceType)) {
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
                if (!GetMandatoryListParam<std::string>(
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
                               nami::SecurityNetworkFilter& thisNetworkFilter,
                               const PipelineContext* mContext,
                               const std::string& sName) {
    std::string errorMsg;
    // DestAddrList (Optional)
    if (!GetOptionalListParam<std::string>(config, "DestAddrList", thisNetworkFilter.mDestAddrList, errorMsg)) {
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
    if (!GetOptionalListParam<std::string>(config, "DestAddrBlackList", thisNetworkFilter.mDestAddrBlackList, errorMsg)) {
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
    if (!GetOptionalListParam<std::string>(config, "SourceAddrList", thisNetworkFilter.mSourceAddrList, errorMsg)) {
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
    if (!GetOptionalListParam<std::string>(
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

bool IsProcessNamespaceFilterTypeValid(const std::string& type) {
    const std::unordered_set<std::string> dic
        = {"Uts", "Ipc", "Mnt", "Pid", "PidForChildren", "Net", "Cgroup", "User", "Time", "TimeForChildren"};
    return dic.find(type) != dic.end();
}


bool SecurityOptions::Init(SecurityFilterType filterType,
                           const Json::Value& config,
                           const PipelineContext* mContext,
                           const std::string& sName) {
    std::string errorMsg;
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
        nami::SecurityOption thisSecurityOption;

        std::string errorMsg;
        // CallName (Optional)
        if (!GetOptionalListParam<std::string>(innerConfig, "CallName", thisSecurityOption.call_names_, errorMsg)) {
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
                nami::SecurityFileFilter thisFileFilter;
                if (!IsValidList(innerConfig, "Filter", errorMsg)) {
                    PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                        mContext->GetAlarm(),
                                        errorMsg,
                                        sName,
                                        mContext->GetConfigName(),
                                        mContext->GetProjectName(),
                                        mContext->GetLogstoreName(),
                                        mContext->GetRegion());
                } else {
                    if (!InitSecurityFileFilter(innerConfig, thisFileFilter, mContext, sName)) {
                        return false;
                    }
                }
                thisSecurityOption.filter_.emplace<nami::SecurityFileFilter>(thisFileFilter);
                break;
            }
            case SecurityFilterType::PROCESS: {
                nami::SecurityProcessFilter thisProcessFilter;
                if (!IsValidMap(innerConfig, "Filter", errorMsg)) {
                    PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                        mContext->GetAlarm(),
                                        errorMsg,
                                        sName,
                                        mContext->GetConfigName(),
                                        mContext->GetProjectName(),
                                        mContext->GetLogstoreName(),
                                        mContext->GetRegion());
                } else {
                    const Json::Value& filterConfig = innerConfig["Filter"];
                    if (!InitSecurityProcessFilter(filterConfig, thisProcessFilter, mContext, sName)) {
                        return false;
                    }
                }
                thisSecurityOption.filter_.emplace<nami::SecurityProcessFilter>(thisProcessFilter);
                break;
            }
            case SecurityFilterType::NETWORK: {
                nami::SecurityNetworkFilter thisNetworkFilter;
                if (!IsValidMap(innerConfig, "Filter", errorMsg)) {
                    PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                        mContext->GetAlarm(),
                                        errorMsg,
                                        sName,
                                        mContext->GetConfigName(),
                                        mContext->GetProjectName(),
                                        mContext->GetLogstoreName(),
                                        mContext->GetRegion());
                } else {
                    const Json::Value& filterConfig = innerConfig["Filter"];
                    if (!InitSecurityNetworkFilter(filterConfig, thisNetworkFilter, mContext, sName)) {
                        return false;
                    }
                }
                thisSecurityOption.filter_.emplace<nami::SecurityNetworkFilter>(thisNetworkFilter);
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
                return false;
        }


        // if (!thisSecurityOption.Init(filterType, innerConfig, mContext, sName)) {
        //     return false;
        // }
        mOptionList.emplace_back(thisSecurityOption);
    }
    mFilterType = filterType;
    return true;
}


//////
void eBPFAdminConfig::LoadEbpfConfig(const Json::Value& confJson) {
    std::string errorMsg;
    // ReceiveEventChanCap (Optional)
    if (!GetOptionalIntParam(confJson, "ReceiveEventChanCap", mReceiveEventChanCap, errorMsg)) {
        LOG_ERROR(sLogger, ("load ReceiveEventChanCap fail", errorMsg));
        return;
    }
    // AdminConfig (Optional)
    if (confJson.isMember("AdminConfig")) {
        if (!confJson["AdminConfig"].isObject()) {
            LOG_ERROR(sLogger, ("AdminConfig", " is not a map"));
            return;
        }
        const Json::Value& thisAdminConfig = confJson["AdminConfig"];
        // AdminConfig.DebugMode (Optional)
        if (!GetOptionalBoolParam(thisAdminConfig, "DebugMode", mAdminConfig.mDebugMode, errorMsg)) {
            LOG_ERROR(sLogger, ("load AdminConfig.DebugMode fail", errorMsg));
            return;
        }
        // AdminConfig.LogLevel (Optional)
        if (!GetOptionalStringParam(thisAdminConfig, "LogLevel", mAdminConfig.mLogLevel, errorMsg)) {
            LOG_ERROR(sLogger, ("load AdminConfig.LogLevel fail", errorMsg));
            return;
        }
        // AdminConfig.PushAllSpan (Optional)
        if (!GetOptionalBoolParam(thisAdminConfig, "PushAllSpan", mAdminConfig.mPushAllSpan, errorMsg)) {
            LOG_ERROR(sLogger, ("load AdminConfig.PushAllSpan fail", errorMsg));
            return;
        }
    }
    // AggregationConfig (Optional)
    if (confJson.isMember("AggregationConfig")) {
        if (!confJson["AggregationConfig"].isObject()) {
            LOG_ERROR(sLogger, ("AggregationConfig", " is not a map"));
            return;
        }
        const Json::Value& thisAggregationConfig = confJson["AggregationConfig"];
        // AggregationConfig.AggWindowSecond (Optional)
        if (!GetOptionalIntParam(
                thisAggregationConfig, "AggWindowSecond", mAggregationConfig.mAggWindowSecond, errorMsg)) {
            LOG_ERROR(sLogger, ("load AggregationConfig.AggWindowSecond fail", errorMsg));
            return;
        }
    }
    // ConverageConfig (Optional)
    if (confJson.isMember("ConverageConfig")) {
        if (!confJson["ConverageConfig"].isObject()) {
            LOG_ERROR(sLogger, ("ConverageConfig", " is not a map"));
            return;
        }
        const Json::Value& thisConverageConfig = confJson["ConverageConfig"];
        // ConverageConfig.Strategy (Optional)
        if (!GetOptionalStringParam(thisConverageConfig, "Strategy", mConverageConfig.mStrategy, errorMsg)) {
            LOG_ERROR(sLogger, ("load ConverageConfig.Strategy fail", errorMsg));
            return;
        }
    }
    // SampleConfig (Optional)
    if (confJson.isMember("SampleConfig")) {
        if (!confJson["SampleConfig"].isObject()) {
            LOG_ERROR(sLogger, ("SampleConfig", " is not a map"));
            return;
        }
        const Json::Value& thisSampleConfig = confJson["SampleConfig"];
        // SampleConfig.Strategy (Optional)
        if (!GetOptionalStringParam(thisSampleConfig, "Strategy", mSampleConfig.mStrategy, errorMsg)) {
            LOG_ERROR(sLogger, ("load SampleConfig.Strategy fail", errorMsg));
            return;
        }
        // SampleConfig.Config (Optional)
        if (thisSampleConfig.isMember("Config")) {
            if (!thisSampleConfig["Config"].isObject()) {
                LOG_ERROR(sLogger, ("SampleConfig.Config", " is not a map"));
                return;
            }
            const Json::Value& thisSampleConfigConfig = thisSampleConfig["Config"];
            // SampleConfig.Config.Rate (Optional)
            if (!GetOptionalDoubleParam(thisSampleConfigConfig, "Rate", mSampleConfig.mConfig.mRate, errorMsg)) {
                LOG_ERROR(sLogger, ("load SampleConfig.Config.Rate fail", errorMsg));
                return;
            }
        }
    }
    // for Observer
    // SocketProbeConfig (Optional)
    if (confJson.isMember("SocketProbeConfig")) {
        if (!confJson["SocketProbeConfig"].isObject()) {
            LOG_ERROR(sLogger, ("SocketProbeConfig", " is not a map"));
            return;
        }
        const Json::Value& thisSocketProbeConfig = confJson["SocketProbeConfig"];
        // SocketProbeConfig.SlowRequestThresholdMs (Optional)
        if (!GetOptionalIntParam(thisSocketProbeConfig,
                                 "SlowRequestThresholdMs",
                                 mSocketProbeConfig.mSlowRequestThresholdMs,
                                 errorMsg)) {
            LOG_ERROR(sLogger, ("load SocketProbeConfig.SlowRequestThresholdMs fail", errorMsg));
            return;
        }
        // SocketProbeConfig.MaxConnTrackers (Optional)
        if (!GetOptionalIntParam(
                thisSocketProbeConfig, "MaxConnTrackers", mSocketProbeConfig.mMaxConnTrackers, errorMsg)) {
            LOG_ERROR(sLogger, ("load SocketProbeConfig.MaxConnTrackers fail", errorMsg));
            return;
        }
        // SocketProbeConfig.MaxBandWidthMbPerSec (Optional)
        if (!GetOptionalIntParam(
                thisSocketProbeConfig, "MaxBandWidthMbPerSec", mSocketProbeConfig.mMaxBandWidthMbPerSec, errorMsg)) {
            LOG_ERROR(sLogger, ("load SocketProbeConfig.MaxBandWidthMbPerSec fail", errorMsg));
            return;
        }
        // SocketProbeConfig.MaxRawRecordPerSec (Optional)
        if (!GetOptionalIntParam(
                thisSocketProbeConfig, "MaxRawRecordPerSec", mSocketProbeConfig.mMaxRawRecordPerSec, errorMsg)) {
            LOG_ERROR(sLogger, ("load SocketProbeConfig.MaxRawRecordPerSec fail", errorMsg));
            return;
        }
    }
    // ProfileProbeConfig (Optional)
    if (confJson.isMember("ProfileProbeConfig")) {
        if (!confJson["ProfileProbeConfig"].isObject()) {
            LOG_ERROR(sLogger, ("ProfileProbeConfig", " is not a map"));
            return;
        }
        const Json::Value& thisProfileProbeConfig = confJson["ProfileProbeConfig"];
        // ProfileProbeConfig.ProfileSampleRate (Optional)
        if (!GetOptionalIntParam(
                thisProfileProbeConfig, "ProfileSampleRate", mProfileProbeConfig.mProfileSampleRate, errorMsg)) {
            LOG_ERROR(sLogger, ("load ProfileProbeConfig.ProfileSampleRate fail", errorMsg));
            return;
        }
        // ProfileProbeConfig.ProfileUploadDuration (Optional)
        if (!GetOptionalIntParam(thisProfileProbeConfig,
                                 "ProfileUploadDuration",
                                 mProfileProbeConfig.mProfileUploadDuration,
                                 errorMsg)) {
            LOG_ERROR(sLogger, ("load ProfileProbeConfig.ProfileUploadDuration fail", errorMsg));
            return;
        }
    }
    // ProcessProbeConfig (Optional)
    if (confJson.isMember("ProcessProbeConfig")) {
        if (!confJson["ProcessProbeConfig"].isObject()) {
            LOG_ERROR(sLogger, ("ProcessProbeConfig", " is not a map"));
            return;
        }
        const Json::Value& thisProcessProbeConfig = confJson["ProcessProbeConfig"];
        // ProcessProbeConfig.EnableOOMDetect (Optional)
        if (!GetOptionalBoolParam(
                thisProcessProbeConfig, "EnableOOMDetect", mProcessProbeConfig.mEnableOOMDetect, errorMsg)) {
            LOG_ERROR(sLogger, ("load ProcessProbeConfig.EnableOOMDetect fail", errorMsg));
            return;
        }
    }
}

}

}