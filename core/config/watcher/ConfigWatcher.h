/*
 * Copyright 2023 iLogtail Authors
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

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "config/Config.h"
#include "config/ConfigDiff.h"

namespace logtail {

class PipelineManager;

class ConfigWatcher {
public:
    ConfigWatcher(const ConfigWatcher&) = delete;
    ConfigWatcher& operator=(const ConfigWatcher&) = delete;

    static ConfigWatcher* GetInstance() {
        static ConfigWatcher instance;
        return &instance;
    }

    ConfigDiff CheckConfigDiff();
    void AddSource(const std::string& dir);
    // for ut
    void SetPipelineManager(const PipelineManager* pm) { mPipelineManager = pm; }
    void ClearEnvironment();

private:
    ConfigWatcher();
    ~ConfigWatcher() = default;

    bool LoadConfigDetailFromFile(const std::filesystem::path& filePath, Json::Value& detail) const;
    bool IsConfigEnabled(const std::string& name, const Json::Value& detail) const;

    std::vector<std::filesystem::path> mSourceDir;
    std::map<std::string, std::pair<uintmax_t, std::filesystem::file_time_type>> mFileInfoMap;
    const PipelineManager* mPipelineManager = nullptr;
};

} // namespace logtail
