/*
 * Copyright 2023 iLogtail Authors
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

#include "pipeline/ProcessConfigManager.h"
#include "app_config/AppConfig.h"

using namespace std;

namespace logtail {

ProcessConfigManager::ProcessConfigManager() {

}

void ProcessConfigManager::UpdateProcessConfigs(ProcessConfigDiff& diff) {
    bool changed = false;
    for (auto & config : diff.mAdded) {
        std::shared_ptr<ProcessConfig> configTmp(new ProcessConfig(config.mName, std::move(config.mDetail)));
        mProcessConfigMap[config.mName] = configTmp;
        changed = true;
    }
    for (auto & config : diff.mModified) {
        std::shared_ptr<ProcessConfig> configTmp(new ProcessConfig(config.mName, std::move(config.mDetail)));
        mProcessConfigMap[config.mName] = configTmp;
        changed = true;
    }
    for (auto & configName : diff.mRemoved) {
        mProcessConfigMap.erase(configName);
        changed = true;
    }
    if (changed) {
        Update();
    }
}

void ProcessConfigManager::Update() {
    for (const auto& config : mProcessConfigMap) {
        for (auto it = config.second.get()->mDetail.get()->begin(); it != config.second.get()->mDetail.get()->end();
             ++it) {
            if (it.name() == "max_bytes_per_sec" && it->isInt64()) {
                AppConfig::GetInstance()->SetMaxBytePerSec(it->asInt64());
            } else if (it.name() == "mem_usage_limit" && it->isDouble()) {
                AppConfig::GetInstance()->SetMemUsageUpLimit(it->asDouble());
            } else if (it.name() == "cpu_usage_limit" && it->isDouble()) {
                AppConfig::GetInstance()->SetCpuUsageUpLimit(it->asDouble());
            }
        }
    }
    AppConfig::GetInstance()->CheckAndAdjustParameters();
}

std::shared_ptr<ProcessConfig> ProcessConfigManager::FindConfigByName(const string& configName) const {
    auto it = mProcessConfigMap.find(configName);
    if (it != mProcessConfigMap.end()) {
        return it->second;
    }
    return nullptr;
}

vector<string> ProcessConfigManager::GetAllConfigNames() const {
    vector<string> res;
    for (const auto& item : mProcessConfigMap) {
        res.push_back(item.first);
    }
    return res;
}


} // namespace logtail
