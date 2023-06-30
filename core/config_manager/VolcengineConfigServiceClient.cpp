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

#include "VolcengineConfigServiceClient.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"
#include "logger/Logger.h"
#include "config_server_pb/agent.pb.h"
#include "ConfigManager.h"
#include "profiler/LogFileProfiler.h"
#include "common/version.h"
#include "common/FileSystemUtil.h"
#include <json/json.h>

using namespace std;

namespace logtail {
	void VolcengineConfigServiceClient::InitClient() {
		this->mMetaServiceHost = AppConfig::GetInstance()->GetStringParameter("meta_service_host", "META_SERVICE_HOST");
		FlushCredential();
		this->signV4.service = AppConfig::GetInstance()->GetStringParameter("gateway_service", "GATEWAY_SERVICE");
		this->signV4.region = getUrlContent(this->mMetaServiceHost, AppConfig::GetInstance()->GetStringParameter("region_uri", "REGION_URI"));
		this->mRegion = this->signV4.region;
		this->mMachineInstancetId = getUrlContent(this->mMetaServiceHost, AppConfig::GetInstance()->GetStringParameter("instance_id_uri", "INSTANCE_ID_URI"));
		this->mAvailableZone = getUrlContent(this->mMetaServiceHost, AppConfig::GetInstance()->GetStringParameter("available_zone_uri", "AVALIABLE_ZONE_URI"));
		this->mAccountId = getUrlContent(this->mMetaServiceHost, AppConfig::GetInstance()->GetStringParameter("account_id_uri", "ACCOUNT_ID_URI"));
	}

	bool VolcengineConfigServiceClient::FlushCredential() { 
		sdk::HttpMessage httpResponse;
        sdk::CurlClient client;
		map<string, string> httpHeader;
		try {
			client.Send(sdk::HTTP_GET, this->mMetaServiceHost, 80, AppConfig::GetInstance()->GetStringParameter("service_role_uri", "SERVICE_ROLE_URI"), "", httpHeader, "", 6, httpResponse, "", false);
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
			LOG_WARNING(sLogger, ("FlushCredential", "fail")("errCode", e.GetErrorCode()));
			return false;
		}
		return true;
	}

	void VolcengineConfigServiceClient::SignHeader(sdk::AsynRequest& request) {
		signV4.SignHeader(request);
	}

	void VolcengineConfigServiceClient::SendMetadata() {
		configserver::proto::MetadataRequest metadataReq;
		std::string requestID = sdk::Base64Enconde(string("metadata").append(to_string(time(NULL))));
		metadataReq.set_request_id(requestID);
		metadataReq.set_agent_id(ConfigManager::GetInstance()->GetInstanceId());
		metadataReq.set_agent_session_id(ConfigManager::GetInstance()->GetSessionId());
		metadataReq.set_agent_type("iLogtail");
    	metadataReq.set_startup_time(time(0));
    	metadataReq.set_interval(INT32_FLAG(config_update_interval));
		configserver::proto::AgentAttributes attributes;
		attributes.set_version(ILOGTAIL_VERSION);
		attributes.set_ip(LogFileProfiler::mIpAddr);
		attributes.set_region(this->mRegion);
		attributes.set_zone(this->mAvailableZone);
    	attributes.set_account_id(this->mAccountId);
    	attributes.set_os(LogFileProfiler::mOsDetail);
    	attributes.set_instance_id(this->mMachineInstancetId);
		metadataReq.mutable_attributes()->MergeFrom(attributes);
		std::string reqBody;
    	metadataReq.SerializeToString(&reqBody);

		AppConfig::ConfigServerAddress configServerAddress = AppConfig::GetInstance()->GetOneConfigServerAddress(false);
		map<string, string> httpHeader = { {sdk::CONTENT_TYPE, sdk::TYPE_LOG_PROTOBUF} };
		// sign request header
		sdk::AsynRequest request(sdk::HTTP_POST, configServerAddress.host, configServerAddress.port, "", "Action=SendMetadata&Version=2018-08-01", httpHeader, reqBody, INT32_FLAG(sls_client_send_timeout), "", false, NULL, NULL);
		this->signV4.SignHeader(request);

    	sdk::HttpMessage httpResponse;
    	sdk::CurlClient client;
    	try {
        	client.Send(request.mHTTPMethod, request.mHost, request.mPort, "", request.mQueryString, request.mHeader, reqBody, request.mTimeout, httpResponse, "", false);
			if (httpResponse.statusCode != 200) {
				LOG_WARNING(sLogger, ("SendMetadata", "fail")("response code", httpResponse.statusCode)("resopnse", httpResponse.content)("reqBody", reqBody));
				return;
			}
		} catch (const sdk::LOGException& e) {
        	LOG_WARNING(sLogger, ("SendMetadata", "fail")("reqBody", reqBody)("errCode", e.GetErrorCode())("errMsg", e.GetMessage()));
			return;
    	}
		LOG_INFO(sLogger, ("send metadata to config server", "success"));
	}

	sdk::AsynRequest VolcengineConfigServiceClient::GenerateHeartBeatRequest(const AppConfig::ConfigServerAddress& configServerAddress, const std::string requestId) {
		configserver::proto::HeartBeatRequest heartBeatReq;
		heartBeatReq.set_request_id(requestId);
		heartBeatReq.set_agent_id(ConfigManager::GetInstance()->GetInstanceId());
		heartBeatReq.set_agent_session_id(ConfigManager::GetInstance()->GetSessionId());
		heartBeatReq.set_running_status("");
		std::string reqBody;
		heartBeatReq.SerializeToString(&reqBody);
		map<string, string> httpHeader = { {sdk::CONTENT_TYPE, sdk::TYPE_LOG_PROTOBUF} };
		// sign request header
		sdk::AsynRequest request(sdk::HTTP_POST, configServerAddress.host, configServerAddress.port, "", "Action=HeartBeat&Version=2018-08-01", httpHeader, reqBody, INT32_FLAG(sls_client_send_timeout), "", false, NULL, NULL);
		return request;
	}

	const std::string VolcengineConfigServiceClient::getUrlContent(const std::string host, const std::string uri) {
		sdk::HttpMessage httpResponse;
        sdk::CurlClient client;
		map<string, string> httpHeader;
		try {
			client.Send(sdk::HTTP_GET, host, 80, uri, "", httpHeader, "", 6, httpResponse, "", false);
			if (httpResponse.statusCode != 200) {
				LOG_WARNING(sLogger, ("getUrlContent", "fail")("uri", uri));
				return "";
			}
			return httpResponse.content;
		} catch (const sdk::LOGException& e) {
			LOG_WARNING(sLogger, ("getUrlContent", "fail")("errCode", e.GetErrorCode()));
			return "";
		}
	}
}