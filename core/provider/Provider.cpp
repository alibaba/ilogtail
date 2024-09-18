/*
 * Copyright 2024 iLogtail Authors
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

#include "Provider.h"

#include "config/common_provider/CommonConfigProvider.h"
#include "config/common_provider/LegacyCommonConfigProvider.h"

namespace logtail {


std::vector<std::pair<std::string, ConfigProvider*>> GetRemoteConfigProviders() {
    std::vector<std::pair<std::string, ConfigProvider*>> providers;
    providers.push_back(std::make_pair("common", LegacyCommonConfigProvider::GetInstance()));
    providers.push_back(std::make_pair("common_v2", CommonConfigProvider::GetInstance()));
    return providers;
}

void InitRemoteConfigProviders(const std::vector<std::pair<std::string, ConfigProvider*>>& providers) {
    for (const auto& configProvider : providers) {
        configProvider.second->Init(configProvider.first);
    }
}

ProfileSender* GetProfileSender() {
    return ProfileSender::GetInstance();
}

} // namespace logtail