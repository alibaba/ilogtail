/*
 * Copyright 2022 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <cstdint>
#include <string>
#include "app_config/AppConfig.h"
#include "config_manager/ConfigManagerBase.h"
#include "config_server_pb/agent.pb.h"
#include "ConfigManager.h"
#include "ConfigServiceClient.h"
#include "profiler/LogFileProfiler.h"
#include "common/version.h"

using namespace std;

namespace logtail {

sdk::AsynRequest ConfigServiceClient::GenerateHeartBeatRequest(const AppConfig::ConfigServerAddress& configServerAddress, const std::string requestId) {
	configserver::proto::HeartBeatRequest heartBeatReq;
    configserver::proto::AgentAttributes attributes;
    heartBeatReq.set_request_id(requestId);
    heartBeatReq.set_agent_id(ConfigManager::GetInstance()->GetInstanceId());
    heartBeatReq.set_agent_type("iLogtail");
    attributes.set_version(ILOGTAIL_VERSION);
    attributes.set_ip(LogFileProfiler::mIpAddr);
    heartBeatReq.mutable_attributes()->MergeFrom(attributes);
    heartBeatReq.mutable_tags()->MergeFrom({AppConfig::GetInstance()->GetConfigServerTags().begin(),
                                            AppConfig::GetInstance()->GetConfigServerTags().end()});
    heartBeatReq.set_running_status("");
    heartBeatReq.set_startup_time(0);
    heartBeatReq.set_interval(INT32_FLAG(config_update_interval));

    google::protobuf::RepeatedPtrField<configserver::proto::ConfigInfo> pipelineConfigs;
    for (std::unordered_map<std::string, int64_t>::iterator it = ConfigManager::GetInstance()->GetMServerYamlConfigVersionMap().begin();
         it != ConfigManager::GetInstance()->GetMServerYamlConfigVersionMap().end();
         it++) {
        configserver::proto::ConfigInfo* info = pipelineConfigs.Add();
        info->set_type(configserver::proto::PIPELINE_CONFIG);
        info->set_name(it->first);
        info->set_version(it->second);
    }
    heartBeatReq.mutable_pipeline_configs()->MergeFrom(pipelineConfigs);

    string operation = sdk::CONFIGSERVERAGENT;
    operation.append("/").append("HeartBeat");
    map<string, string> httpHeader;
    httpHeader[sdk::CONTENT_TYPE] = sdk::TYPE_LOG_PROTOBUF;
    std::string reqBody;
    heartBeatReq.SerializeToString(&reqBody);
	sdk::AsynRequest request(sdk::HTTP_POST, configServerAddress.host, configServerAddress.port, operation, "", 
		httpHeader, reqBody, INT32_FLAG(sls_client_send_timeout), "", false, NULL, NULL);
	return request;
}

} // namespace logtail
