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

#include "BytedanceConfigService.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"
#include "logger/Logger.h"
#include "config_server_pb/agent.pb.h"
#include "ConfigManager.h"
#include "profiler/LogFileProfiler.h"
#include "common/version.h"
#include <json/json.h>

using namespace std;

namespace logtail {
	void BytedanceConfigServiceClient::initClient() {
		flushCredential();
		this->signV4.service = AppConfig::GetInstance()->GetGatewayService();
		this->signV4.region = AppConfig::GetInstance()->GetRegion();
	}

	bool BytedanceConfigServiceClient::flushCredential() { 
		sdk::HttpMessage httpResponse;
        sdk::CurlClient client;
		map<string, string> httpHeader;
		try {
			client.Send(sdk::HTTP_GET, AppConfig::GetInstance()->GetMetaServiceHost(), 80, AppConfig::GetInstance()->GetServiceRoleUri(), "", httpHeader, "", 6, httpResponse, "", false);
			Json::Value jsValue;
			std::string jsErrors;
			Json::CharReaderBuilder jcrBuilder;
			std::unique_ptr<Json::CharReader>jcReader(jcrBuilder.newCharReader());
			if (!jcReader->parse(httpResponse.content.c_str(), httpResponse.content.c_str() + httpResponse.content.length(), &jsValue, &jsErrors)) {
				LOG_WARNING(sLogger, ("parse response failed, response:", httpResponse.content));
				return false;
			}
			this->signV4.accessKeyId = jsValue["AccessKeyId"].asString();
			this->signV4.secretAccessKey = jsValue["SecretAccessKey"].asString();
			this->signV4.securityToken = jsValue["SessionToken"].asString();
		} catch (const sdk::LOGException& e) {
			LOG_WARNING(sLogger, ("flushCredential", "fail")("errCode", e.GetErrorCode()));
			return false;
		}
		return true;
	}

	void BytedanceConfigServiceClient::signHeader(sdk::AsynRequest& request) {
		signV4.signHeader(request);
	}

	void BytedanceConfigServiceClient::SendMetadata() {
		configserver::proto::MetadataRequest metadataReq;
		std::string requestID = sdk::Base64Enconde(string("metadata").append(to_string(time(NULL))));
		metadataReq.set_request_id(requestID);
		metadataReq.set_agent_id(ConfigManager::GetInstance()->GetInstanceId());
		metadataReq.set_agent_machine_id(AppConfig::GetInstance()->GetAgentMachineId());
		metadataReq.set_agent_type("iLogtail");
    	metadataReq.set_startup_time(time(0));
    	metadataReq.set_interval(INT32_FLAG(config_update_interval));
		metadataReq.set_version(ILOGTAIL_VERSION);
		metadataReq.set_ip(LogFileProfiler::mIpAddr);
		metadataReq.set_region(AppConfig::GetInstance()->GetRegion());
    	metadataReq.set_avaliable_zone(AppConfig::GetInstance()->GetAvailableZone());
    	metadataReq.set_account_id(AppConfig::GetInstance()->GetAccountId());
    	metadataReq.set_os(LogFileProfiler::mOsDetail);
		std::string reqBody;
    	metadataReq.SerializeToString(&reqBody);

		AppConfig::ConfigServerAddress configServerAddress = AppConfig::GetInstance()->GetOneConfigServerAddress(false);
		map<string, string> httpHeader = { {sdk::CONTENT_TYPE, sdk::TYPE_LOG_PROTOBUF} };
		// sign request header
		sdk::AsynRequest request(sdk::HTTP_POST, configServerAddress.host, configServerAddress.port, "", "Action=SendMetadata&Version=2018-08-01", httpHeader, reqBody, INT32_FLAG(sls_client_send_timeout), "", false, NULL, NULL);
		this->signV4.signHeader(request);

    	sdk::HttpMessage httpResponse;
    	sdk::CurlClient client;
    	try {
        	client.Send(request.mHTTPMethod, request.mHost, request.mPort, "", request.mQueryString, request.mHeader, reqBody, request.mTimeout, httpResponse, "", false);
			if (httpResponse.statusCode != 200) {
				LOG_WARNING(sLogger, ("SendMetadata", "fail")("reqBody", reqBody));
				return;
			}
		} catch (const sdk::LOGException& e) {
        	LOG_WARNING(sLogger, ("SendMetadata", "fail")("reqBody", reqBody)("errCode", e.GetErrorCode())("errMsg", e.GetMessage()));
			return;
    	}
		LOG_INFO(sLogger, ("send metadata to config server", "success"));
	}

	sdk::AsynRequest BytedanceConfigServiceClient::GenerateHeartBeatRequest(const AppConfig::ConfigServerAddress& configServerAddress, const std::string requestId) {
		configserver::proto::HeartBeatRequest heartBeatReq;
		heartBeatReq.set_request_id(requestId);
		heartBeatReq.set_agent_id(ConfigManager::GetInstance()->GetInstanceId());
		heartBeatReq.set_agent_machine_id(AppConfig::GetInstance()->GetAgentMachineId());
		heartBeatReq.set_running_status("");
		std::string reqBody;
		heartBeatReq.SerializeToString(&reqBody);
		map<string, string> httpHeader = { {sdk::CONTENT_TYPE, sdk::TYPE_LOG_PROTOBUF} };
		// sign request header
		sdk::AsynRequest request(sdk::HTTP_POST, configServerAddress.host, configServerAddress.port, "", "Action=HeartBeat&Version=2018-08-01", httpHeader, reqBody, INT32_FLAG(sls_client_send_timeout), "", false, NULL, NULL);
		return request;
	}
}