#include <string>

#include "logger/Logger.h"
#include "ebpf/config.h"
#include "common/ParamExtractor.h"

namespace logtail {
namespace ebpf {

void eBPFConfig::LoadEbpfConfig(const Json::Value& confJson) {
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