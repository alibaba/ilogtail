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
// See the License for the specific language governing permissions and
// limitations under the License.

#include "app_config/AppConfig.h"
#include "config_manager/ConfigManager.h"
#include "logger/Logger.h"
#include "ProfileSender.h"
#include "sender/Sender.h"
#include "sdk/Common.h"
#include "sdk/Client.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"
#include "common/CompressTools.h"
#include "common/LogtailCommonFlags.h"
#include "sls_control/SLSControl.h"

using namespace std;

DEFINE_FLAG_BOOL(send_running_status, "", true);

namespace logtail {

void ProfileSender::SendToProfileProject(const std::string& region, sls_logs::LogGroup& logGroup) {
    if (0 == logGroup.category().compare("logtail_status_profile")) {
        SendRunningStatus(logGroup);
    }

    // Opensource is not necessary to synchronize data with SLS
    Sender::Instance()->RestLastSenderTime();
    ConfigManager::GetInstance()->RestLastConfigTime();
    return;
}

void ProfileSender::SendRunningStatus(sls_logs::LogGroup& logGroup) {
    if (!BOOL_FLAG(send_running_status)) {
        return;
    }

    static int controlFeq = 0;

    // every 12 hours
    if (0 == logGroup.logs_size() || 0 != controlFeq++ % (60 * 12)) {
        return;
    }

    std::string region = "cn-shanghai";
    std::string project = "ilogtail-community-edition";
    std::string logstore = "ilogtail-online";
    std::string endpoint = region + ".log.aliyuncs.com";

    Json::Value logtailStatus;
    logtailStatus["__topic__"] = "logtail_status_profile";
    unordered_set<std::string> selectedFields(
        {"cpu", "mem", "version", "instance_key", "os", "os_detail", "load", "status", "metric_json", "plugin_stats"});
    Json::Value status;
    const sls_logs::Log& log = logGroup.logs(0);
    for (int32_t conIdx = 0; conIdx < log.contents_size(); ++conIdx) {
        const sls_logs::Log_Content& content = log.contents(conIdx);
        const string& key = content.key();
        const string& value = content.value();
        if (selectedFields.find(key) != selectedFields.end()) {
            status[key] = value;
        }
    }
    logtailStatus["__logs__"][0] = status;
    std::string logBody = logtailStatus.toStyledString();
    sdk::Client client(endpoint, "", "", INT32_FLAG(sls_client_send_timeout), "", true, "");
    SLSControl::Instance()->SetSlsSendClientCommonParam(&client);
    try {
        time_t curTime = time(NULL);
        LoggroupTimeValue* data = new LoggroupTimeValue(
            project, logstore, "", "", false, "", region, LOGGROUP_LZ4_COMPRESSED, 1, logBody.size(), curTime, "", 0);

        if (!CompressLz4(logBody, data->mLogData)) {
            LOG_ERROR(sLogger, ("lz4 compress data", "fail"));
            return;
        }

        sdk::PostLogStoreLogsResponse resp
            = client.PostLogUsingWebTracking(data->mProjectName, data->mLogstore, data->mLogData, data->mRawSize);

        LOG_DEBUG(sLogger,
                  ("SendToProfileProject",
                   "success")("logBody", logBody)("requestId", resp.requestId)("statusCode", resp.statusCode));
    } catch (const sdk::LOGException& e) {
        LOG_DEBUG(sLogger,
                  ("SendToProfileProject", "fail")("logBody", logBody)("errCode", e.GetErrorCode())("errMsg",
                                                                                                    e.GetMessage()));
    }
}

void ProfileSender::SendToConfigServer(sls_logs::LogGroup& logGroup) {
    if (0 == logGroup.category().compare("logtail_status_profile")) {
        SendHeartbeat(logGroup);
        SendRunningStatistics(logGroup);
    }

    // Opensource is not necessary to synchronize data with SLS
    Sender::Instance()->RestLastSenderTime();
    ConfigManager::GetInstance()->RestLastConfigTime();
    return;
}

void ProfileSender::SendHeartbeat(sls_logs::LogGroup& logGroup) {
    static int controlFeq = 0;

    // every 10 seconds
    if (0 == logGroup.logs_size() || 0 != controlFeq++ % (10)) {
        return;
    }

    configserver::proto::HeartBeatRequest heartBeatReq;
    std::string requestID = sdk::Base64Enconde(string("heartbeat").append(to_string(time(NULL))));
    heartBeatReq.set_request_id(requestID);
    heartBeatReq.set_agent_type("iLogtail");
    heartBeatReq.set_startup_time(0);

    Json::Value logtailStatus;
    logtailStatus["__topic__"] = "logtail_status_profile";
    unordered_set<std::string> selectedFields(
        {"version", "instance_key", "status", "ip"});
    const sls_logs::Log& log = logGroup.logs(0);
    for (int32_t conIdx = 0; conIdx < log.contents_size(); ++conIdx) {
        const sls_logs::Log_Content& content = log.contents(conIdx);
        const string& key = content.key();
        const string& value = content.value();
        if (selectedFields.find(key) != selectedFields.end()) {
            if (0 == key.compare("instance_key")) heartBeatReq.set_agent_id(value);
            if (0 == key.compare("version")) heartBeatReq.set_agent_version(value);
            if (0 == key.compare("ip")) heartBeatReq.set_ip(value);
            if (0 == key.compare("status")) heartBeatReq.set_running_status(value);
        }
    }

    AppConfig::ConfigServerAddress configServerAddress = AppConfig::GetInstance()->GetConfigServerAddress();
    string operation = sdk::CONFIGSERVERAGENT;
    operation.append("/").append("HeartBeat");
    map<string, string> httpHeader;
    httpHeader[sdk::CONTENT_TYPE] = sdk::TYPE_LOG_PROTOBUF;
    std::string reqBody;
    heartBeatReq.SerializeToString(&reqBody);
    sdk::HttpMessage httpResponse;

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
                    false
        );
        configserver::proto::HeartBeatResponse heartBeatResp;
        heartBeatResp.ParseFromString(httpResponse.content);

        if (0 != strcmp(heartBeatResp.response_id().c_str(), requestID.c_str())) return;

        LOG_DEBUG(sLogger, ("SendHeartBeat","success")("reqBody", reqBody)
                  ("requestId", heartBeatResp.response_id())("statusCode", heartBeatResp.code()));
    } catch (const sdk::LOGException& e) {
        LOG_ERROR(sLogger, ("SendHeartBeat", "fail")("reqBody", reqBody)("errCode", e.GetErrorCode())("errMsg", e.GetMessage()));
        LOG_DEBUG(sLogger, ("SendHeartBeat", "fail")("reqBody", reqBody)("errCode", e.GetErrorCode())("errMsg", e.GetMessage()));
    }
}

void ProfileSender::SendRunningStatistics(sls_logs::LogGroup& logGroup) {
    
}

bool ProfileSender::SendInstantly(sls_logs::LogGroup& logGroup,
                                  const std::string& aliuid,
                                  const std::string& region,
                                  const std::string& projectName,
                                  const std::string& logstore) {
    return true;
}

void ProfileSender::SendToLineCountProject(const std::string& region,
                                           const std::string& projectName,
                                           sls_logs::LogGroup& logGroup) {
    return;
}


} // namespace logtail