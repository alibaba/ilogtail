// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific l

#pragma once

#include <string>
#include <json/json.h>
#include <variant>
#include <vector>

#include "pipeline/PipelineContext.h"
#include "ebpf/include/export.h"


namespace logtail {
namespace ebpf {

/////////////////////  /////////////////////

enum class ObserverType { PROCESS, FILE, NETWORK };
bool InitObserverFileOption(const Json::Value& config,
                            nami::ObserverFileOption& thisObserverFileOption,
                            const PipelineContext* mContext,
                            const std::string& sName);
bool InitObserverProcessOption(const Json::Value& config, 
                               nami::ObserverProcessOption& thisObserverProcessOption,
                               const PipelineContext* mContext,
                               const std::string& sName);
bool InitObserverNetworkOption(const Json::Value& config, 
                               nami::ObserverNetworkOption& thisObserverNetworkOption,
                               const PipelineContext* mContext,
                               const std::string& sName);

class ObserverOptions {
public:
    bool Init(ObserverType type, const Json::Value& config, const PipelineContext* mContext, const std::string& sName);

    std::variant<nami::ObserverProcessOption, nami::ObserverFileOption, nami::ObserverNetworkOption> mObserverOption;
    ObserverType mType;
};
/////////////////////  /////////////////////

enum class SecurityFilterType { PROCESS, FILE, NETWORK };

class SecurityOptions {
public:
    bool Init(SecurityFilterType filterType,
              const Json::Value& config,
              const PipelineContext* mContext,
              const std::string& sName);

    std::vector<nami::SecurityOption> mOptionList;
    SecurityFilterType mFilterType;
};

///////////////////// Process Level Config /////////////////////

const int32_t DEFUALT_RECEIVE_EVENT_CHAN_CAP = 4096;
const bool DEFUALT_ADMIN_DEBUG_MODE = false;
const std::string DEFUALT_ADMIN_LOG_LEVEL = "warn";
const bool DEFUALT_ADMIN_PUSH_ALL_SPAN = false;
const int32_t DEFUALT_AGGREGATION_WINDOW_SECOND = 15;
const std::string DEFUALT_CONVERAGE_STRATEGY = "combine";
const std::string DEFUALT_SAMPLE_STRATEGY = "fixedRate";
const double DEFUALT_SAMPLE_RATE = 0.01;
const int32_t DEFUALT_SOCKET_SLOW_REQUEST_THRESHOLD_MS = 500;
const int32_t DEFUALT_SOCKET_MAX_CONN_TRACKDERS = 10000;
const int32_t DEFUALT_SOCKET_MAX_BAND_WITH_MB_PER_SEC = 30;
const int32_t DEFUALT_SOCKET_MAX_RAW_RECORD_PER_SEC = 100000;
const int32_t DEFUALT_PROFILE_SAMPLE_RATE = 10;
const int32_t DEFUALT_PROFILE_UPLOAD_DURATION = 10;
const bool DEFUALT_PROCESS_ENABLE_OOM_DETECT = false;

struct AdminConfig {
    bool mDebugMode = DEFUALT_ADMIN_DEBUG_MODE;
    std::string mLogLevel = DEFUALT_ADMIN_LOG_LEVEL;
    bool mPushAllSpan = DEFUALT_ADMIN_PUSH_ALL_SPAN;
};

struct AggregationConfig {
    int32_t mAggWindowSecond = DEFUALT_AGGREGATION_WINDOW_SECOND;
};

struct ConverageConfig {
    std::string mStrategy = DEFUALT_CONVERAGE_STRATEGY;
};

struct SampleConfig {
    std::string mStrategy = DEFUALT_SAMPLE_STRATEGY;
    struct Config {
        double mRate = DEFUALT_SAMPLE_RATE;
    } mConfig;
};

struct SocketProbeConfig {
    int32_t mSlowRequestThresholdMs = DEFUALT_SOCKET_SLOW_REQUEST_THRESHOLD_MS;
    int32_t mMaxConnTrackers = DEFUALT_SOCKET_MAX_CONN_TRACKDERS;
    int32_t mMaxBandWidthMbPerSec = DEFUALT_SOCKET_MAX_BAND_WITH_MB_PER_SEC;
    int32_t mMaxRawRecordPerSec = DEFUALT_SOCKET_MAX_RAW_RECORD_PER_SEC;
};

struct ProfileProbeConfig {
    int32_t mProfileSampleRate = DEFUALT_PROFILE_SAMPLE_RATE;
    int32_t mProfileUploadDuration = DEFUALT_PROFILE_UPLOAD_DURATION;
};

struct ProcessProbeConfig {
    bool mEnableOOMDetect = DEFUALT_PROCESS_ENABLE_OOM_DETECT;
};

class eBPFAdminConfig {
public:
    eBPFAdminConfig() {}
    ~eBPFAdminConfig() {}

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
    int32_t mReceiveEventChanCap = DEFUALT_RECEIVE_EVENT_CHAN_CAP;
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

} // ebpf
} // logtail
