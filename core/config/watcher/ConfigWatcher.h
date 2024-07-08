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
    void AddPipelineSource(const std::string& dir, std::mutex* mux = nullptr);
    void AddProcessSource(const std::string& dir, std::mutex* mux = nullptr);
    void AddCommandSource(const std::string& dir, std::mutex* mux = nullptr);

    // for ut
    void SetPipelineManager(const PipelineManager* pm) { mPipelineManager = pm; }
    void ClearEnvironment();

private:
    ConfigWatcher();
    ~ConfigWatcher() = default;

    std::vector<std::filesystem::path> mPipelineConfigDir;
    std::unordered_map<std::string, std::mutex*> mPipelineConfigDirMutexMap;

    std::vector<std::filesystem::path> mProcessConfigDir;
    std::unordered_map<std::string, std::mutex*> mProcessConfigDirMutexMap;

    std::vector<std::filesystem::path> mCommandConfigDir;
    std::unordered_map<std::string, std::mutex*> mCommandConfigDirMutexMap;

    std::map<std::string, std::pair<uintmax_t, std::filesystem::file_time_type>> mPipelineFileInfoMap;
    const PipelineManager* mPipelineManager = nullptr;

    std::map<std::string, std::pair<uintmax_t, std::filesystem::file_time_type>> mProcessFileInfoMap;
    std::map<std::string, std::pair<uintmax_t, std::filesystem::file_time_type>> mCommandFileInfoMap;

    bool CheckDirectoryStatus(const std::filesystem::path& dir);
};

} // namespace logtail
