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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "config/NewConfig.h"
#include "config/ConfigDiff.h"
#include "pipeline/Pipeline.h"

namespace logtail {
class PipelineManager {
public:
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;

    static PipelineManager* GetInstance() {
        static PipelineManager instance;
        return &instance;
    }

    void Init();
    bool LoadAllPipelines();
    bool RemoveAllPipelines();
    void UpdatePipelines(ConfigDiff& diff);
    std::shared_ptr<Pipeline> FindPipelineByName(const std::string& configName);
    std::vector<std::string> GetAllPipelineNames() const; // 过渡使用

private:
    PipelineManager() = default;
    ~PipelineManager() = default;

    std::shared_ptr<Pipeline> BuildPipeline(NewConfig&& config);
    void CheckIfInputUpdated(const NewConfig& config,
                          bool& isInputObserverChanged,
                          bool& isInputFileChanged,
                          bool& isInputStreamChanged);

    bool mIsFirstUpdate = true;
    std::unordered_map<std::string, std::shared_ptr<Pipeline> > mPipelineDict;
};
} // namespace logtail
