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

#include "Constants.h"
#include "app_config/AppConfig.h"
#include "application/Application.h"
#include "common/LogtailCommonFlags.h"
#include "common/StringTools.h"
#include "common/UUIDUtil.h"
#include "common/version.h"
#include "logger/Logger.h"
#include "monitor/LogFileProfiler.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

DEFINE_FLAG_INT32(heartBeat_update_interval, "second", 10);

namespace logtail {


void CommonConfigProvider::Init(const string& dir) {
    string sName = "CommonConfigProvider";

    ConfigProvider::Init(dir);

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

void CommonConfigProvider::CheckUpdateThread() {
    LOG_INFO(sLogger, (sName, "started"));
    usleep((rand() % 10) * 100 * 1000);
    int32_t lastCheckTime = 0;
    unique_lock<mutex> lock(mThreadRunningMux);
    while (mIsThreadRunning) {
        int32_t curTime = time(NULL);
        if (curTime - lastCheckTime >= INT32_FLAG(heartBeat_update_interval)) {
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
    switch (configInfo.second.status) {
        case UNSET:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::UNSET);
            break;
        case APPLYING:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::APPLYING);
            break;
        case APPLIED:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::APPLIED);
            break;
        case FAILED:
            reqConfig->set_status(configserver::proto::v2::ConfigStatus::FAILED);
            break;
    }
    reqConfig->set_version(configInfo.second.version);
}

void CommonConfigProvider::GetConfigUpdate() {
    if (!GetConfigServerAvailable()) {
        return;
    }
    auto heartBeatRequest = PrepareHeartbeat();
    auto HeartBeatResponse = SendHeartBeat(heartBeatRequest);
    auto processConfig = FetchProcessConfig(HeartBeatResponse);
    auto pipelineConfig = FetchPipelineConfig(HeartBeatResponse);
    if (!pipelineConfig.empty()) {
        LOG_DEBUG(sLogger, ("fetch pipelineConfig, config file number", pipelineConfig.size()));
        UpdateRemoteConfig(pipelineConfig);
    }
    if (!processConfig.empty()) {
        LOG_DEBUG(sLogger, ("fetch processConfig config, config file number", processConfig.size()));
        UpdateRemoteConfig(processConfig);
    }
}

configserver::proto::v2::HeartBeatRequest CommonConfigProvider::PrepareHeartbeat() {
    configserver::proto::v2::HeartBeatRequest heartBeatReq;
    string requestID = sdk::Base64Enconde(string("HeartBeat").append(to_string(time(NULL))));
    heartBeatReq.set_request_id(requestID);
    heartBeatReq.set_sequence_num(mSequenceNum);
    heartBeatReq.set_capabilities(configserver::proto::v2::AcceptsProcessConfig
                                  | configserver::proto::v2::AcceptsPipelineConfig);
    heartBeatReq.set_instance_id(GetInstanceId());
    heartBeatReq.set_agent_type("LoongCollector");
    FillAttributes(*heartBeatReq.mutable_attributes());

    for (auto tag : GetConfigServerTags()) {
        configserver::proto::v2::AgentGroupTag* agentGroupTag = heartBeatReq.add_tags();
        agentGroupTag->set_name(tag.first);
        agentGroupTag->set_value(tag.second);
    }
    heartBeatReq.set_running_status("running");
    heartBeatReq.set_startup_time(mStartTime);

    for (const auto& configInfo : mPipelineConfigInfoMap) {
        addConfigInfoToRequest(configInfo, heartBeatReq.add_pipeline_configs());
    }
    for (const auto& configInfo : mProcessConfigInfoMap) {
        addConfigInfoToRequest(configInfo, heartBeatReq.add_process_configs());
    }

    for (auto& configInfo : mCommandInfoMap) {
        configserver::proto::v2::CommandInfo* command = heartBeatReq.add_custom_commands();
        command->set_type(configInfo.second.type);
        command->set_name(configInfo.second.name);
        switch (configInfo.second.status) {
            case UNSET:
                command->set_status(configserver::proto::v2::ConfigStatus::UNSET);
            case APPLYING:
                command->set_status(configserver::proto::v2::ConfigStatus::APPLYING);
            case APPLIED:
                command->set_status(configserver::proto::v2::ConfigStatus::APPLIED);
            case FAILED:
                command->set_status(configserver::proto::v2::ConfigStatus::FAILED);
        }
        command->set_message(configInfo.second.message);
    }
    return heartBeatReq;
}

configserver::proto::v2::HeartBeatResponse
CommonConfigProvider::SendHeartBeat(configserver::proto::v2::HeartBeatRequest heartBeatReq) {
    string operation = sdk::CONFIGSERVERAGENT;
    operation.append("/").append("HeartBeat");
    string reqBody;
    heartBeatReq.SerializeToString(&reqBody);
    configserver::proto::v2::HeartBeatResponse emptyResult;
    string emptyResultString;
    emptyResult.SerializeToString(&emptyResultString);
    auto heartBeatResp = SendHttpRequest(operation, reqBody, emptyResultString, "SendHeartBeat");
    configserver::proto::v2::HeartBeatResponse heartBeatRespPb;
    heartBeatRespPb.ParseFromString(heartBeatResp);
    return heartBeatRespPb;
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
CommonConfigProvider::FetchPipelineConfig(configserver::proto::v2::HeartBeatResponse& heartBeatResponse) {
    if (heartBeatResponse.flags() & ::configserver::proto::v2::FetchPipelineConfigDetail) {
        return FetchPipelineConfigFromServer(heartBeatResponse);
    } else {
        return std::move(heartBeatResponse.pipeline_config_updates());
    }
}

::google::protobuf::RepeatedPtrField< ::configserver::proto::v2::ConfigDetail>
CommonConfigProvider::FetchProcessConfig(configserver::proto::v2::HeartBeatResponse& heartBeatResponse) {
    if (heartBeatResponse.flags() & ::configserver::proto::v2::FetchPipelineConfigDetail) {
        return FetchProcessConfigFromServer(heartBeatResponse);
    } else {
        return std::move(heartBeatResponse.process_config_updates());
    }
}

void CommonConfigProvider::UpdateRemoteConfig(
    const google::protobuf::RepeatedPtrField<configserver::proto::v2::ConfigDetail>& configs) {
    error_code ec;
    filesystem::create_directories(mSourceDir, ec);
    if (ec) {
        StopUsingConfigServer();
        LOG_ERROR(sLogger,
                  ("failed to create dir for common configs", "stop receiving config from common config server")(
                      "dir", mSourceDir.string())("error code", ec.value())("error msg", ec.message()));
        return;
    }

    lock_guard<mutex> lock(mMux);
    for (const auto& config : configs) {
        filesystem::path filePath = mSourceDir / (config.name() + ".yaml");
        filesystem::path tmpFilePath = mSourceDir / (config.name() + ".yaml.new");
        if (config.version() == -1) {
            mConfigNameVersionMap.erase(config.name());
            filesystem::remove(filePath, ec);
        } else {
            string configDetail = config.detail();
            mConfigNameVersionMap[config.name()] = config.version();
            ofstream fout(tmpFilePath);
            if (!fout) {
                LOG_WARNING(sLogger, ("failed to open config file", filePath.string()));
                continue;
            }
            fout << configDetail;

            error_code ec;
            filesystem::rename(tmpFilePath, filePath, ec);
            if (ec) {
                LOG_WARNING(sLogger,
                            ("failed to dump config file", filePath.string())("error code", ec.value())("error msg",
                                                                                                        ec.message()));
                filesystem::remove(tmpFilePath, ec);
            }
        }
    }
}

} // namespace logtail
