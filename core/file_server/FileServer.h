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

#include <string>
#include <unordered_map>
#include <utility>

#include "file_server/FileDiscoveryOptions.h"
#include "file_server/MultilineOptions.h"
#include "pipeline/PipelineContext.h"
#include "reader/FileReaderOptions.h"

namespace logtail {

class FileServer {
public:
    FileServer(const FileServer&) = delete;
    FileServer& operator=(const FileServer&) = delete;

    static FileServer* GetInstance() {
        static FileServer instance;
        return &instance;
    }

    FileDiscoveryConfig GetFileDiscoveryConfig(const std::string& name) const;
    FileReaderConfig GetFileReaderConfig(const std::string& name) const;
    MultilineConfig GetMultilineConfig(const std::string& name) const;
    void AddFileDiscoveryConfig(const std::string& name, FileDiscoveryOptions* opts, const PipelineContext* ctx);
    void AddFileReaderConfig(const std::string& name, const FileReaderOptions* opts, const PipelineContext* ctx);
    void AddMultilineConfig(const std::string& name, const MultilineOptions* opts, const PipelineContext* ctx);
    void ClearFileConfigs() {
        mPipelineNameFileDiscoveryConfigsMap.clear();
        mPipelineNameFileReaderConfigsMap.clear();
        mPipelineNameMultilineConfigsMap.clear();
        mPipelineNameEOConcurrencyMap.clear();
    }
    const std::unordered_map<std::string, FileDiscoveryConfig>& GetAllFileDiscoveryConfigs() const {
        return mPipelineNameFileDiscoveryConfigsMap;
    }
    const std::unordered_map<std::string, FileReaderConfig>& GetAllFileReaderConfigs() const {
        return mPipelineNameFileReaderConfigsMap;
    }
    const std::unordered_map<std::string, MultilineConfig>& GetAllMultilineConfigs() const {
        return mPipelineNameMultilineConfigsMap;
    }
    // 过渡使用
    uint32_t GetExactlyOnceConcurrency(const std::string& name) const;
    void AddExactlyOnceConcurrency(const std::string& name, uint32_t concurrency);
    std::vector<std::string> GetExactlyOnceConfigs() const;

private:
    FileServer() = default;
    ~FileServer() = default;

    std::unordered_map<std::string, FileDiscoveryConfig> mPipelineNameFileDiscoveryConfigsMap;
    std::unordered_map<std::string, FileReaderConfig> mPipelineNameFileReaderConfigsMap;
    std::unordered_map<std::string, MultilineConfig> mPipelineNameMultilineConfigsMap;
    // 过渡使用
    std::unordered_map<std::string, uint32_t> mPipelineNameEOConcurrencyMap;
};

} // namespace logtail
