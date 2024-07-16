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
#include "config/feedbacker/ConfigFeedbackReceiver.h"

using namespace std;

namespace logtail {

ProcessConfigManager::ProcessConfigManager() {
}

void ProcessConfigManager::UpdateProcessConfigs(ProcessConfigDiff& diff) {
    bool changed = false;
    for (auto& config : diff.mAdded) {
        std::shared_ptr<ProcessConfig> configTmp(new ProcessConfig(config.mName, std::move(config.mDetail)));
        mProcessConfigMap[config.mName] = configTmp;
        ConfigFeedbackReceiver::GetInstance().FeedbackProcessConfigStatus(config.mName, ConfigFeedbackStatus::APPLIED);
        changed = true;
    }
    for (auto& config : diff.mModified) {
        std::shared_ptr<ProcessConfig> configTmp(new ProcessConfig(config.mName, std::move(config.mDetail)));
        mProcessConfigMap[config.mName] = configTmp;
        ConfigFeedbackReceiver::GetInstance().FeedbackProcessConfigStatus(config.mName, ConfigFeedbackStatus::APPLIED);
        changed = true;
    }
    for (auto& configName : diff.mRemoved) {
        mProcessConfigMap.erase(configName);
        ConfigFeedbackReceiver::GetInstance().FeedbackProcessConfigStatus(configName, ConfigFeedbackStatus::DELETED);
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
            mConfig[it.name()] = *it;
        }
    }
    for (const auto& callback : callbacks) {
        callback();
    }
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

bool ProcessConfigManager::GetProcessConfigBoolValue(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isBool()) {
        isExist = true;
        return mConfig[key].asBool();
    }
    isExist = false;
    return false;
}

int ProcessConfigManager::GetProcessConfigIntValue(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isInt()) {
        isExist = true;
        return mConfig[key].asInt();
    }
    isExist = false;
    return 0;
}

int64_t ProcessConfigManager::GetProcessConfigInt64Value(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isInt64()) {
        isExist = true;
        return mConfig[key].asInt64();
    }
    isExist = false;
    return 0;
}

unsigned int ProcessConfigManager::GetProcessConfigUIntValue(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isUInt()) {
        isExist = true;
        return mConfig[key].asUInt();
    }
    isExist = false;
    return 0;
}

uint64_t ProcessConfigManager::GetProcessConfigUInt64Value(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isUInt64()) {
        isExist = true;
        return mConfig[key].asUInt64();
    }
    isExist = false;
    return 0;
}

double ProcessConfigManager::GetProcessConfigRealValue(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isDouble()) {
        isExist = true;
        return mConfig[key].asDouble();
    }
    isExist = false;
    return 0.0;
}

std::string ProcessConfigManager::GetProcessConfigStringValue(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isString()) {
        isExist = true;
        return mConfig[key].asString();
    }
    isExist = false;
    return "";
}

Json::Value ProcessConfigManager::GetProcessConfigArrayValue(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isArray()) {
        isExist = true;
        return mConfig[key];
    }
    isExist = false;
    return Json::Value(Json::arrayValue);
}

Json::Value ProcessConfigManager::GetProcessConfigObjectValue(const std::string key, bool& isExist) {
    if (mConfig.isMember(key) && mConfig[key].isObject()) {
        isExist = true;
        return mConfig[key];
    }
    isExist = false;
    return Json::Value(Json::objectValue);
}

} // namespace logtail
