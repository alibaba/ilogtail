/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

#include "config/provider/ConfigProvider.h"
#include "config_server_pb/v2/agent.pb.h"

namespace logtail {

enum ConfigStatus {
    UNSET = 0,
    APPLYING = 1,
    APPLIED = 2,
    FAILED = 3,
};

struct ConfigInfo {
    std::string name;
    int64_t version;
    ConfigStatus status;
    std::string message;
    std::string detail;
};

struct CommandInfo {
    std::string type;
    std::string name;
    ConfigStatus status;
    std::string message;
};

class CommonConfigProvider : public ConfigProvider {
public:
    std::string sName;

    std::filesystem::path mSourceDir;
    mutable std::mutex mMux;
    std::unordered_map<std::string, ConfigInfo> mPipelineConfigInfoMap;
    std::unordered_map<std::string, ConfigInfo> mProcessConfigInfoMap;

    std::unordered_map<std::string, CommandInfo> mCommandInfoMap;
    int64_t mSequenceNum;

    CommonConfigProvider(const CommonConfigProvider&) = delete;
    CommonConfigProvider& operator=(const CommonConfigProvider&) = delete;

    static CommonConfigProvider* GetInstance() {
        static CommonConfigProvider instance;
        return &instance;
    }

    void Init(const std::string& dir) override;
    void Stop() override;

    // virtual void FeedbackProcessConfigStatus(std::string name, ConfigInfo status);
    // virtual void FeedbackPipelineConfigStatus(std::string name, ConfigInfo status);
    // virtual void FeedbackCommandStatus(std::string type, std::string name, CommandInfo status);

    CommonConfigProvider() = default;
    ~CommonConfigProvider() = default;

protected:
    virtual configserver::proto::v2::HeartBeatRequest PrepareHeartbeat();
    virtual configserver::proto::v2::HeartBeatResponse SendHeartBeat(configserver::proto::v2::HeartBeatRequest);

    virtual ::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
    FetchProcessConfig(::configserver::proto::v2::HeartBeatResponse&);

    virtual ::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
    FetchPipelineConfig(::configserver::proto::v2::HeartBeatResponse&);

    virtual std::string GetInstanceId();
    virtual void FillAttributes(::configserver::proto::v2::AgentAttributes& attributes);
    virtual void UpdateRemoteConfig(const std::string& fetchConfigResponse);
    virtual void
    UpdateRemoteConfig(const google::protobuf::RepeatedPtrField<configserver::proto::v2::ConfigDetail>& configs);

    virtual ::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
    FetchProcessConfigFromServer(::configserver::proto::v2::HeartBeatResponse&);
    virtual ::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
    FetchPipelineConfigFromServer(::configserver::proto::v2::HeartBeatResponse&);

    void CheckUpdateThread();
    void GetConfigUpdate();
    bool GetConfigServerAvailable() { return mConfigServerAvailable; }
    void StopUsingConfigServer() { mConfigServerAvailable = false; }

    int32_t mStartTime;
    std::future<void> mThreadRes;
    mutable std::mutex mThreadRunningMux;
    bool mIsThreadRunning = true;
    mutable std::condition_variable mStopCV;
    std::unordered_map<std::string, int64_t> mConfigNameVersionMap;
    bool mConfigServerAvailable = false;

private:
    struct ConfigServerAddress {
        ConfigServerAddress() = default;
        ConfigServerAddress(const std::string& config_server_host, const std::int32_t& config_server_port)
            : host(config_server_host), port(config_server_port) {}

        std::string host;
        std::int32_t port;
    };

    ConfigServerAddress GetOneConfigServerAddress(bool changeConfigServer);
    const std::unordered_map<std::string, std::string>& GetConfigServerTags() const { return mConfigServerTags; }

    std::string SendHttpRequest(const std::string& operation,
                           const std::string& reqBody,
                           const std::string& emptyResultString,
                           const std::string& configType);

    std::vector<ConfigServerAddress> mConfigServerAddresses;
    int mConfigServerAddressId = 0;
    std::unordered_map<std::string, std::string> mConfigServerTags;

};

} // namespace logtail
