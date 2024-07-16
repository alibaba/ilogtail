// Copyright 2023 iLogtail Authors
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
// See the License for the specific language governing permissions and
// limitations under the License.

#include "config/provider/CommonConfigProvider.h"

#include <json/json.h>

#include <filesystem>
#include <iostream>
#include <random>

#include "app_config/AppConfig.h"
#include "application/Application.h"
#include "common/Constants.h"
#include "common/LogtailCommonFlags.h"
#include "common/StringTools.h"
#include "common/UUIDUtil.h"
#include "common/YamlUtil.h"
#include "common/version.h"
#include "config/PipelineConfig.h"
#include "config/feedbacker/ConfigFeedbackReceiver.h"
#include "logger/Logger.h"
#include "monitor/LogFileProfiler.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

DEFINE_FLAG_INT32(heartbeat_update_interval, "second", 10);

namespace logtail {

void CommonConfigProvider::Init(const string& dir) {
    string sName = "CommonConfigProvider";

    ConfigProvider::Init(dir);
    LoadConfigFile();

    mSequenceNum = 0;

    const Json::Value& confJson = AppConfig::GetInstance()->GetConfig();

    // configserver path
    if (confJson.isMember("ilogtail_configserver_address") && confJson["ilogtail_configserver_address"].isArray()) {
        for (Json::Value::ArrayIndex i = 0; i < confJson["ilogtail_configserver_address"].size(); ++i) {
            vector<string> configServerAddress
                = SplitString(TrimString(confJson["ilogtail_configserver_address"][i].asString()), ":");

            if (configServerAddress.size() != 2) {
                LOG_WARNING(sLogger,
                            ("ilogtail_configserver_address", "format error")(
                                "wrong address", TrimString(confJson["ilogtail_configserver_address"][i].asString())));
                continue;
            }

            string host = configServerAddress[0];
            int32_t port = atoi(configServerAddress[1].c_str());

            if (port < 1 || port > 65535)
                LOG_WARNING(sLogger, ("ilogtail_configserver_address", "illegal port")("port", port));
            else
                mConfigServerAddresses.push_back(ConfigServerAddress(host, port));
        }

        mConfigServerAvailable = true;
        LOG_INFO(sLogger,
                 ("ilogtail_configserver_address", confJson["ilogtail_configserver_address"].toStyledString()));
    }

    // tags for configserver
    if (confJson.isMember("ilogtail_tags") && confJson["ilogtail_tags"].isObject()) {
        Json::Value::Members members = confJson["ilogtail_tags"].getMemberNames();
        for (Json::Value::Members::iterator it = members.begin(); it != members.end(); it++) {
            mConfigServerTags[*it] = confJson["ilogtail_tags"][*it].asString();
        }
        LOG_INFO(sLogger, ("ilogtail_configserver_tags", confJson["ilogtail_tags"].toStyledString()));
    }

    GetConfigUpdate();

    mThreadRes = async(launch::async, &CommonConfigProvider::CheckUpdateThread, this);
}

void CommonConfigProvider::Stop() {
    {
        lock_guard<mutex> lock(mThreadRunningMux);
        mIsThreadRunning = false;
    }
    mStopCV.notify_one();
    future_status s = mThreadRes.wait_for(chrono::seconds(1));
    if (s == future_status::ready) {
        LOG_INFO(sLogger, (sName, "stopped successfully"));
    } else {
        LOG_WARNING(sLogger, (sName, "forced to stopped"));
    }
}

void CommonConfigProvider::LoadConfigFile() {
    error_code ec;
    for (auto const& entry : filesystem::directory_iterator(mPipelineSourceDir, ec)) {
        Json::Value detail;
        if (LoadConfigDetailFromFile(entry, detail)) {
            ConfigInfo info;
            info.name = entry.path().stem();
            if (detail.isMember("version") && detail["version"].isInt64()) {
                info.version = detail["version"].asInt64();
            }
            info.status = ConfigFeedbackStatus::APPLYING;
            info.detail = detail.toStyledString();
            lock_guard<mutex> infomaplock(mInfoMapMux);
            mPipelineConfigInfoMap[info.name] = info;
        }
    }
    for (auto const& entry : filesystem::directory_iterator(mProcessSourceDir, ec)) {
        Json::Value detail;
        if (LoadConfigDetailFromFile(entry, detail)) {
            ConfigInfo info;
            info.name = entry.path().stem();
            if (detail.isMember("version") && detail["version"].isInt64()) {
                info.version = detail["version"].asInt64();
            }
            info.status = ConfigFeedbackStatus::APPLYING;
            info.detail = detail.toStyledString();
            lock_guard<mutex> infomaplock(mInfoMapMux);
            mProcessConfigInfoMap[info.name] = info;
        }
    }
}

void CommonConfigProvider::CheckUpdateThread() {
    LOG_INFO(sLogger, (sName, "started"));
    usleep((rand() % 10) * 100 * 1000);
    int32_t lastCheckTime = 0;
    unique_lock<mutex> lock(mThreadRunningMux);
    while (mIsThreadRunning) {
        int32_t curTime = time(NULL);
        if (curTime - lastCheckTime >= INT32_FLAG(heartbeat_update_interval)) {
            GetConfigUpdate();
            lastCheckTime = curTime;
        }
        if (mStopCV.wait_for(lock, chrono::seconds(3), [this]() { return !mIsThreadRunning; })) {
            break;
        }
    }
}

CommonConfigProvider::ConfigServerAddress CommonConfigProvider::GetOneConfigServerAddress(bool changeConfigServer) {
    if (0 == mConfigServerAddresses.size()) {
        return ConfigServerAddress("", -1); // No address available
    }

    // Return a random address
    if (changeConfigServer) {
        random_device rd;
        int tmpId = rd() % mConfigServerAddresses.size();
        while (mConfigServerAddresses.size() > 1 && tmpId == mConfigServerAddressId) {
            tmpId = rd() % mConfigServerAddresses.size();
        }
        mConfigServerAddressId = tmpId;
    }
    return ConfigServerAddress(mConfigServerAddresses[mConfigServerAddressId].host,
                               mConfigServerAddresses[mConfigServerAddressId].port);
}

string CommonConfigProvider::GetInstanceId() {
    return CalculateRandomUUID() + "_" + LogFileProfiler::mIpAddr + "_" + ToString(mStartTime);
}

void CommonConfigProvider::FillAttributes(configserver::proto::v2::AgentAttributes& attributes) {
    attributes.set_hostname(LogFileProfiler::mHostname);
    attributes.set_ip(LogFileProfiler::mIpAddr);
    attributes.set_version(ILOGTAIL_VERSION);
    google::protobuf::Map<string, string>* extras = attributes.mutable_extras();
    extras->insert({"osDetail", LogFileProfiler::mOsDetail});
}

void addConfigInfoToRequest(const std::pair<const string, logtail::ConfigInfo>& configInfo,
                            configserver::proto::v2::ConfigInfo* reqConfig) {
    reqConfig->set_name(configInfo.second.name);
    reqConfig->set_message(configInfo.second.message);
    reqConfig->set_version(configInfo.second.version);
    switch (configInfo.second.status) {
        case ConfigFeedbackStatus::UNSET:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::UNSET);
            break;
        case ConfigFeedbackStatus::APPLYING:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::APPLYING);
            break;
        case ConfigFeedbackStatus::APPLIED:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::APPLIED);
            break;
        case ConfigFeedbackStatus::FAILED:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::FAILED);
            break;
        case ConfigFeedbackStatus::DELETED:
            reqConfig->set_version(-1);
            break;
    }
    reqConfig->set_version(configInfo.second.version);
}

void CommonConfigProvider::GetConfigUpdate() {
    if (!GetConfigServerAvailable()) {
        return;
    }
    auto heartbeatRequest = PrepareHeartbeat();
    auto HeartbeatResponse = SendHeartbeat(heartbeatRequest);
    auto processConfig = FetchProcessConfig(HeartbeatResponse);
    auto pipelineConfig = FetchPipelineConfig(HeartbeatResponse);
    if (!pipelineConfig.empty()) {
        LOG_DEBUG(sLogger, ("fetch pipelineConfig, config file number", pipelineConfig.size()));
        UpdateRemotePipelineConfig(pipelineConfig);
    }
    if (!processConfig.empty()) {
        LOG_DEBUG(sLogger, ("fetch processConfig config, config file number", processConfig.size()));
        UpdateRemoteProcessConfig(processConfig);
    }
}

configserver::proto::v2::HeartbeatRequest CommonConfigProvider::PrepareHeartbeat() {
    configserver::proto::v2::HeartbeatRequest heartbeatReq;
    string requestID = sdk::Base64Enconde(string("Heartbeat").append(to_string(time(NULL))));
    heartbeatReq.set_request_id(requestID);
    heartbeatReq.set_sequence_num(mSequenceNum);
    heartbeatReq.set_capabilities(configserver::proto::v2::AcceptsProcessConfig
                                  | configserver::proto::v2::AcceptsPipelineConfig);
    heartbeatReq.set_instance_id(GetInstanceId());
    heartbeatReq.set_agent_type("LoongCollector");
    FillAttributes(*heartbeatReq.mutable_attributes());

    for (auto tag : GetConfigServerTags()) {
        configserver::proto::v2::AgentGroupTag* agentGroupTag = heartbeatReq.add_tags();
        agentGroupTag->set_name(tag.first);
        agentGroupTag->set_value(tag.second);
    }
    heartbeatReq.set_running_status("running");
    heartbeatReq.set_startup_time(mStartTime);

    lock_guard<mutex> infomaplock(mInfoMapMux);
    for (const auto& configInfo : mPipelineConfigInfoMap) {
        addConfigInfoToRequest(configInfo, heartbeatReq.add_pipeline_configs());
    }
    for (const auto& configInfo : mProcessConfigInfoMap) {
        addConfigInfoToRequest(configInfo, heartbeatReq.add_process_configs());
    }

    for (auto& configInfo : mCommandInfoMap) {
        configserver::proto::v2::CommandInfo* command = heartbeatReq.add_custom_commands();
        command->set_type(configInfo.second.type);
        command->set_name(configInfo.second.name);
        command->set_message(configInfo.second.message);
        switch (configInfo.second.status) {
            case ConfigFeedbackStatus::UNSET:
                command->set_status(configserver::proto::v2::ConfigStatus::UNSET);
                break;
            case ConfigFeedbackStatus::APPLYING:
                command->set_status(configserver::proto::v2::ConfigStatus::APPLYING);
                break;
            case ConfigFeedbackStatus::APPLIED:
                command->set_status(configserver::proto::v2::ConfigStatus::APPLIED);
                break;
            case ConfigFeedbackStatus::FAILED:
                command->set_status(configserver::proto::v2::ConfigStatus::FAILED);
                break;
            case ConfigFeedbackStatus::DELETED:
                break;
        }
        command->set_message(configInfo.second.message);
    }
    return heartbeatReq;
}

configserver::proto::v2::HeartbeatResponse
CommonConfigProvider::SendHeartbeat(configserver::proto::v2::HeartbeatRequest heartbeatReq) {
    string operation = sdk::CONFIGSERVERAGENT;
    operation.append("/").append("Heartbeat");
    string reqBody;
    heartbeatReq.SerializeToString(&reqBody);
    configserver::proto::v2::HeartbeatResponse emptyResult;
    string emptyResultString;
    emptyResult.SerializeToString(&emptyResultString);
    auto heartbeatResp = SendHttpRequest(operation, reqBody, emptyResultString, "SendHeartbeat");
    configserver::proto::v2::HeartbeatResponse heartbeatRespPb;
    heartbeatRespPb.ParseFromString(heartbeatResp);
    return heartbeatRespPb;
}

string CommonConfigProvider::SendHttpRequest(const string& operation,
                                             const string& reqBody,
                                             const string& emptyResultString,
                                             const string& configType) {
    ConfigServerAddress configServerAddress = GetOneConfigServerAddress(false);
    map<string, string> httpHeader;
    httpHeader[sdk::CONTENT_TYPE] = sdk::TYPE_LOG_PROTOBUF;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = "ConfigServer";
    sdk::CurlClient client;

    try {
        client.Send(sdk::HTTP_POST,
                    configServerAddress.host,
                    configServerAddress.port,
                    operation,
                    "",
                    httpHeader,
                    reqBody,
                    INT32_FLAG(sls_client_send_timeout),
                    httpResponse,
                    "",
                    false);
        return httpResponse.content;
    } catch (const sdk::LOGException& e) {
        LOG_WARNING(sLogger,
                    (configType, "fail")("reqBody", reqBody)("errCode", e.GetErrorCode())("errMsg", e.GetMessage())(
                        "host", configServerAddress.host)("port", configServerAddress.port));
        return emptyResultString;
    }
}

::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
CommonConfigProvider::FetchPipelineConfig(configserver::proto::v2::HeartbeatResponse& heartbeatResponse) {
    if (heartbeatResponse.flags() & ::configserver::proto::v2::FetchPipelineConfigDetail) {
        return FetchPipelineConfigFromServer(heartbeatResponse);
    } else {
        ::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail> result;
        result.Swap(heartbeatResponse.mutable_pipeline_config_updates());
        return result;
    }
}

::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
CommonConfigProvider::FetchProcessConfig(configserver::proto::v2::HeartbeatResponse& heartbeatResponse) {
    if (heartbeatResponse.flags() & ::configserver::proto::v2::FetchPipelineConfigDetail) {
        return FetchProcessConfigFromServer(heartbeatResponse);
    } else {
        ::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail> result;
        result.Swap(heartbeatResponse.mutable_process_config_updates());
        return result;
    }
}

bool CommonConfigProvider::DumpConfigFile(const configserver::proto::v2::ConfigDetail& config,
                                          const filesystem::path& sourceDir) {
    filesystem::path filePath = sourceDir / (config.name() + ".json");
    filesystem::path tmpFilePath = sourceDir / (config.name() + ".json.new");
    Json::Value detail;
    std::string errorMsg;
    if (!ParseConfigDetail(config.detail(), ".yaml", detail, errorMsg)) {
        LOG_WARNING(sLogger, ("failed to parse config detail", config.detail()));
        return false;
    }
    detail["version"] = config.version();
    string configDetail = detail.toStyledString();
    ofstream fout(tmpFilePath);
    if (!fout) {
        LOG_WARNING(sLogger, ("failed to open config file", filePath.string()));
        return false;
    }
    fout << configDetail;

    error_code ec;
    filesystem::rename(tmpFilePath, filePath, ec);
    if (ec) {
        LOG_WARNING(
            sLogger,
            ("failed to dump config file", filePath.string())("error code", ec.value())("error msg", ec.message()));
        filesystem::remove(tmpFilePath, ec);
    }
    return true;
}

void CommonConfigProvider::UpdateRemotePipelineConfig(
    const google::protobuf::RepeatedPtrField<configserver::proto::v2::ConfigDetail>& configs) {
    error_code ec;
    const std::filesystem::path& sourceDir = mPipelineSourceDir;
    filesystem::create_directories(sourceDir, ec);
    if (ec) {
        StopUsingConfigServer();
        LOG_ERROR(sLogger,
                  ("failed to create dir for common configs", "stop receiving config from common config server")(
                      "dir", sourceDir.string())("error code", ec.value())("error msg", ec.message()));
        return;
    }

    lock_guard<mutex> lock(mPipelineMux);
    lock_guard<mutex> infomaplock(mInfoMapMux);
    for (const auto& config : configs) {
        filesystem::path filePath = sourceDir / (config.name() + ".json");
        if (config.version() == -1) {
            mPipelineConfigInfoMap.erase(config.name());
            filesystem::remove(filePath, ec);
            ConfigFeedbackReceiver::GetInstance().UnregisterPipelineConfig(config.name());
        } else {
            if (!DumpConfigFile(config, sourceDir)) {
                mPipelineConfigInfoMap[config.name()] = ConfigInfo{.name = config.name(),
                                                                   .version = config.version(),
                                                                   .status = ConfigFeedbackStatus::FAILED,
                                                                   .detail = config.detail()};
                continue;
            }
            mPipelineConfigInfoMap[config.name()] = ConfigInfo{.name = config.name(),
                                                               .version = config.version(),
                                                               .status = ConfigFeedbackStatus::APPLYING,
                                                               .detail = config.detail()};
            ConfigFeedbackReceiver::GetInstance().RegisterPipelineConfig(config.name(), this);
        }
    }
}

void CommonConfigProvider::UpdateRemoteProcessConfig(
    const google::protobuf::RepeatedPtrField<configserver::proto::v2::ConfigDetail>& configs) {
    error_code ec;
    const std::filesystem::path& sourceDir = mProcessSourceDir;
    filesystem::create_directories(sourceDir, ec);
    if (ec) {
        StopUsingConfigServer();
        LOG_ERROR(sLogger,
                  ("failed to create dir for common configs", "stop receiving config from common config server")(
                      "dir", sourceDir.string())("error code", ec.value())("error msg", ec.message()));
        return;
    }

    lock_guard<mutex> lock(mProcessMux);
    lock_guard<mutex> infomaplock(mInfoMapMux);
    for (const auto& config : configs) {
        filesystem::path filePath = sourceDir / (config.name() + ".yaml");
        if (config.version() == -1) {
            mProcessConfigInfoMap.erase(config.name());
            filesystem::remove(filePath, ec);
            ConfigFeedbackReceiver::GetInstance().UnregisterProcessConfig(config.name());
        } else {
            filesystem::path filePath = sourceDir / (config.name() + ".json");
            if (config.version() == -1) {
                mProcessConfigInfoMap.erase(config.name());
                filesystem::remove(filePath, ec);
                ConfigFeedbackReceiver::GetInstance().UnregisterProcessConfig(config.name());
            } else {
                if (!DumpConfigFile(config, sourceDir)) {
                    mProcessConfigInfoMap[config.name()] = ConfigInfo{.name = config.name(),
                                                                      .version = config.version(),
                                                                      .status = ConfigFeedbackStatus::FAILED,
                                                                      .detail = config.detail()};
                    continue;
                }
                mProcessConfigInfoMap[config.name()] = ConfigInfo{.name = config.name(),
                                                                  .version = config.version(),
                                                                  .status = ConfigFeedbackStatus::APPLYING,
                                                                  .detail = config.detail()};
                ConfigFeedbackReceiver::GetInstance().RegisterProcessConfig(config.name(), this);
            }
        }
    }
}

::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
CommonConfigProvider::FetchProcessConfigFromServer(::configserver::proto::v2::HeartbeatResponse& heartbeatResponse) {
    configserver::proto::v2::FetchConfigRequest fetchConfigRequest;
    string requestID = sdk::Base64Enconde(string("FetchProcessConfig").append(to_string(time(NULL))));
    fetchConfigRequest.set_request_id(requestID);
    fetchConfigRequest.set_instance_id(GetInstanceId());
    for (const auto& config : heartbeatResponse.process_config_updates()) {
        auto reqConfig = fetchConfigRequest.add_req_configs();
        reqConfig->set_name(config.name());
        reqConfig->set_version(config.version());
    }
    string operation = sdk::CONFIGSERVERAGENT;
    operation.append("/FetchProcessConfig");
    string reqBody;
    fetchConfigRequest.SerializeToString(&reqBody);
    configserver::proto::v2::FetchConfigResponse emptyResult;
    string emptyResultString;
    emptyResult.SerializeToString(&emptyResultString);
    string fetchConfigResponse = SendHttpRequest(operation, reqBody, emptyResultString, "FetchProcessConfig");
    configserver::proto::v2::FetchConfigResponse fetchConfigResponsePb;
    fetchConfigResponsePb.ParseFromString(fetchConfigResponse);
    return std::move(fetchConfigResponsePb.config_details());
}

::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
CommonConfigProvider::FetchPipelineConfigFromServer(::configserver::proto::v2::HeartbeatResponse& heartbeatResponse) {
    configserver::proto::v2::FetchConfigRequest fetchConfigRequest;
    string requestID = sdk::Base64Enconde(string("FetchPipelineConfig").append(to_string(time(NULL))));
    fetchConfigRequest.set_request_id(requestID);
    fetchConfigRequest.set_instance_id(GetInstanceId());
    for (const auto& config : heartbeatResponse.pipeline_config_updates()) {
        auto reqConfig = fetchConfigRequest.add_req_configs();
        reqConfig->set_name(config.name());
        reqConfig->set_version(config.version());
    }
    string operation = sdk::CONFIGSERVERAGENT;
    operation.append("/FetchPipelineConfig");
    string reqBody;
    fetchConfigRequest.SerializeToString(&reqBody);
    configserver::proto::v2::FetchConfigResponse emptyResult;
    string emptyResultString;
    emptyResult.SerializeToString(&emptyResultString);
    string fetchConfigResponse = SendHttpRequest(operation, reqBody, emptyResultString, "FetchPipelineConfig");
    configserver::proto::v2::FetchConfigResponse fetchConfigResponsePb;
    fetchConfigResponsePb.ParseFromString(fetchConfigResponse);
    return std::move(fetchConfigResponsePb.config_details());
}

void CommonConfigProvider::FeedbackPipelineConfigStatus(const std::string& name, ConfigFeedbackStatus status) {
    lock_guard<mutex> infomaplock(mInfoMapMux);
    auto info = mPipelineConfigInfoMap.find(name);
    if (info != mPipelineConfigInfoMap.end()) {
        info->second.status = status;
    }
    LOG_DEBUG(sLogger,
              ("CommonConfigProvider", "FeedbackPipelineConfigStatus")("name", name)("status", ToStringView(status)));
}
void CommonConfigProvider::FeedbackProcessConfigStatus(const std::string& name, ConfigFeedbackStatus status) {
    lock_guard<mutex> infomaplock(mInfoMapMux);
    auto info = mProcessConfigInfoMap.find(name);
    if (info != mProcessConfigInfoMap.end()) {
        info->second.status = status;
    }
    LOG_DEBUG(sLogger,
              ("CommonConfigProvider", "FeedbackProcessConfigStatus")("name", name)("status", ToStringView(status)));
}
void CommonConfigProvider::FeedbackCommandConfigStatus(const std::string& type,
                                                       const std::string& name,
                                                       ConfigFeedbackStatus status) {
    lock_guard<mutex> infomaplock(mInfoMapMux);
    auto info = mCommandInfoMap.find(type + '\1' + name);
    if (info != mCommandInfoMap.end()) {
        info->second.status = status;
    }
    LOG_DEBUG(sLogger,
              ("CommonConfigProvider",
               "FeedbackCommandConfigStatus")("type", type)("name", name)("status", ToStringView(status)));
}

} // namespace logtail
