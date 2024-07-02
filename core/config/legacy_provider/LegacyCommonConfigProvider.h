/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/YamlUtil.h"
#include "config/provider/ConfigProvider.h"
#include "config_server_pb/agent.pb.h"

namespace logtail {

void ConvertLegacyYamlAndStore(const std::string& inputFilePath, const std::string& outputFilePath);

// LegacyCommonConfigProvider is used to convert yaml config for ilogtail 1.0.
// It detects file changes in user_yaml_config.d/ and copy changed files to config/local/.
class LegacyCommonConfigProvider : public ConfigProvider {
public:
    LegacyCommonConfigProvider(const LegacyCommonConfigProvider&) = delete;
    LegacyCommonConfigProvider& operator=(const LegacyCommonConfigProvider&) = delete;

    static LegacyCommonConfigProvider* GetInstance() {
        static LegacyCommonConfigProvider instance;
        return &instance;
    }

    void Init(const std::string& dir) override;
    void Stop() override;

private:
    LegacyCommonConfigProvider() = default;
    ~LegacyCommonConfigProvider() = default;


    void CheckUpdateThread();
    void GetConfigUpdate();
    bool IsLegacyConfigDirExisted() { return std::filesystem::exists(mLegacySourceDir); }

    std::future<void> mThreadRes;
    mutable std::mutex mThreadRunningMux;
    bool mIsThreadRunning = true;
    mutable std::condition_variable mStopCV;
    // key is config name, value is modification timestamp.
    std::unordered_map<std::string, int64_t> mConfigNameTimestampMap;
    std::filesystem::path mLegacySourceDir; 
};

} // namespace logtail
