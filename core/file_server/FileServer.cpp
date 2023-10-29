// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "file_server/FileServer.h"
#include "input/InputFile.h"

using namespace std;

namespace logtail {

FileDiscoveryConfig FileServer::GetFileDiscoveryConfig(const string& name) const {
    auto itr = mPipelineNameFileDiscoveryConfigsMap.find(name);
    if (itr != mPipelineNameFileDiscoveryConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

FileReaderConfig FileServer::GetFileReaderConfig(const string& name) const {
    auto itr = mPipelineNameFileReaderConfigsMap.find(name);
    if (itr != mPipelineNameFileReaderConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

MultilineConfig FileServer::GetMultilineConfig(const std::string& name) const {
    auto itr = mPipelineNameMultilineConfigsMap.find(name);
    if (itr != mPipelineNameMultilineConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

void FileServer::AddFileDiscoveryConfig(const string& name, FileDiscoveryOptions* opts, const PipelineContext* ctx) {
    mPipelineNameFileDiscoveryConfigsMap[name] = make_pair(opts, ctx);
}

void FileServer::AddFileReaderConfig(const string& name, const FileReaderOptions* opts, const PipelineContext* ctx) {
    mPipelineNameFileReaderConfigsMap[name] = make_pair(opts, ctx);
}

void FileServer::AddMultilineConfig(const string& name, const MultilineOptions* opts, const PipelineContext* ctx) {
    mPipelineNameMultilineConfigsMap[name] = make_pair(opts, ctx);
}

uint32_t FileServer::GetExactlyOnceConcurrency(const std::string& name) const {
    auto itr = mPipelineNameEOConcurrencyMap.find(name);
    if (itr != mPipelineNameEOConcurrencyMap.end()) {
        return itr->second;
    }
    return 0;
}

void FileServer::AddExactlyOnceConcurrency(const std::string& name, uint32_t concurrency) {
    mPipelineNameEOConcurrencyMap[name] = concurrency;
}

vector<string> FileServer::GetExactlyOnceConfigs() const {
    vector<string> res;
    for (const auto& item : mPipelineNameEOConcurrencyMap) {
        if (item.second > 0) {
            res.push_back(item.first);
        }
    }
    return res;
}

} // namespace logtail
