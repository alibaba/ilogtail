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

namespace logtail {

class ConfigServiceClient: public ConfigServiceClientBase {
public:
    void initClient(){};
    bool flushCredential(){ return true; };
    void signHeader(sdk::AsynRequest& request){};
	void SendMetadata(){};
	sdk::AsynRequest GenerateHeartBeatRequest(const AppConfig::ConfigServerAddress& configServerAddress, const std::string requestId);
};

} // namespace logtail
