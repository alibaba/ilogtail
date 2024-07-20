#pragma once

#include <string>
#include <json/json.h>


namespace logtail {
namespace ebpf {

const int32_t defaultReceiveEventChanCap = 4096;
const bool defaultAdminDebugMode = false;
const std::string defaultAdminLogLevel = "warn";
const bool defaultAdminPushAllSpan = false;
const int32_t defaultAggregationAggWindowSecond = 15;
const std::string defaultConverageStrategy = "combine";
const std::string defaultSampleStrategy = "fixedRate";
const double defaultSampleRate = 0.01;
const int32_t defaultSocketSlowRequestThresholdMs = 500;
const int32_t defaultSocketMaxConnTrackers = 10000;
const int32_t defaultSocketMaxBandWidthMbPerSec = 30;
const int32_t defaultSocketMaxRawRecordPerSec = 100000;
const int32_t defaultProfileSampleRate = 10;
const int32_t defaultProfileUploadDuration = 10;
const bool defaultProcessEnableOOMDetect = false;

struct AdminConfig {
    bool mDebugMode = defaultAdminDebugMode;
    std::string mLogLevel = defaultAdminLogLevel;
    bool mPushAllSpan = defaultAdminPushAllSpan;
};

struct AggregationConfig {
    int32_t mAggWindowSecond = defaultAggregationAggWindowSecond;
};

struct ConverageConfig {
    std::string mStrategy = defaultConverageStrategy;
};

struct SampleConfig {
    std::string mStrategy = defaultSampleStrategy;
    struct Config {
        double mRate = defaultSampleRate;
    } mConfig;
};

struct SocketProbeConfig {
    int32_t mSlowRequestThresholdMs = defaultSocketSlowRequestThresholdMs;
    int32_t mMaxConnTrackers = defaultSocketMaxConnTrackers;
    int32_t mMaxBandWidthMbPerSec = defaultSocketMaxBandWidthMbPerSec;
    int32_t mMaxRawRecordPerSec = defaultSocketMaxRawRecordPerSec;
};

struct ProfileProbeConfig {
    int32_t mProfileSampleRate = defaultProfileSampleRate;
    int32_t mProfileUploadDuration = defaultProfileUploadDuration;
};

struct ProcessProbeConfig {
    bool mEnableOOMDetect = defaultProcessEnableOOMDetect;
};

class eBPFConfig {
public:
    eBPFConfig() {}
    ~eBPFConfig() {}

    void LoadEbpfConfig(const Json::Value& confJson);

    int32_t GetReceiveEventChanCap() const { return mReceiveEventChanCap; }

    const AdminConfig& GetAdminConfig() const { return mAdminConfig; }

    const AggregationConfig& GetAggregationConfig() const { return mAggregationConfig; }

    const ConverageConfig& GetConverageConfig() const { return mConverageConfig; }

    const SampleConfig& GetSampleConfig() const { return mSampleConfig; }

    const SocketProbeConfig& GetSocketProbeConfig() const { return mSocketProbeConfig; }

    const ProfileProbeConfig& GetProfileProbeConfig() const { return mProfileProbeConfig; }

    const ProcessProbeConfig& GetProcessProbeConfig() const { return mProcessProbeConfig; }

private:
    int32_t mReceiveEventChanCap = defaultReceiveEventChanCap;
    AdminConfig mAdminConfig;

    AggregationConfig mAggregationConfig;

    ConverageConfig mConverageConfig;

    SampleConfig mSampleConfig;

    SocketProbeConfig mSocketProbeConfig;

    ProfileProbeConfig mProfileProbeConfig;

    ProcessProbeConfig mProcessProbeConfig;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif

};

}
}