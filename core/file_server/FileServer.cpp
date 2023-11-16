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
#include "checkpoint/CheckPointManager.h"
#include "common/Flags.h"
#include "config_manager/ConfigManager.h"
#include "controller/EventDispatcher.h"
#include "event_handler/LogInput.h"
#include "polling/PollingDirFile.h"
#include "polling/PollingModify.h"

DEFINE_FLAG_BOOL(enable_polling_discovery, "", true);

using namespace std;

namespace logtail {

void FileServer::Start() {
    ConfigManager::GetInstance()->LoadDockerConfig();
    CheckPointManager::Instance()->LoadCheckPoint();
    ConfigManager::GetInstance()->RegisterHandlers();
    EventDispatcher::GetInstance()->AddExistedCheckPointFileEvents();
    if (BOOL_FLAG(enable_polling_discovery)) {
        PollingModify::GetInstance()->Start();
        PollingDirFile::GetInstance()->Start();
    }
    LogInput::GetInstance()->Start();
}

void FileServer::Pause() {
    LogInput::GetInstance()->HoldOn();
    EventDispatcher::GetInstance()->ClearBrokenLinkSet();
    PollingDirFile::GetInstance()->ClearCache();
    ConfigManager::GetInstance()->ClearFilePipelineMatchCache();
    EventDispatcher::GetInstance()->DumpAllHandlersMeta(true);
    CheckPointManager::Instance()->DumpCheckPointToLocal();
    CheckPointManager::Instance()->ResetLastDumpTime();
}

void FileServer::Resume() {
    ClearContainerInfo();
    ConfigManager::GetInstance()->DoUpdateContainerPaths();
    ConfigManager::GetInstance()->SaveDockerConfig();
    LogInput::GetInstance()->Resume(true);
}

void FileServer::Stop() {
    LogInput::GetInstance()->HoldOn();
    EventDispatcher::GetInstance()->DumpAllHandlersMeta(false);
    CheckPointManager::Instance()->DumpCheckPointToLocal();
}

FileDiscoveryConfig FileServer::GetFileDiscoveryConfig(const string& name) const {
    auto itr = mPipelineNameFileDiscoveryConfigsMap.find(name);
    if (itr != mPipelineNameFileDiscoveryConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

void FileServer::AddFileDiscoveryConfig(const string& name, FileDiscoveryOptions* opts, const PipelineContext* ctx) {
    mPipelineNameFileDiscoveryConfigsMap[name] = make_pair(opts, ctx);
}

void FileServer::RemoveFileDiscoveryConfig(const string& name) {
    mPipelineNameFileDiscoveryConfigsMap.erase(name);
}

FileReaderConfig FileServer::GetFileReaderConfig(const string& name) const {
    auto itr = mPipelineNameFileReaderConfigsMap.find(name);
    if (itr != mPipelineNameFileReaderConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

void FileServer::AddFileReaderConfig(const string& name, const FileReaderOptions* opts, const PipelineContext* ctx) {
    mPipelineNameFileReaderConfigsMap[name] = make_pair(opts, ctx);
}

void FileServer::RemoveFileReaderConfig(const string& name) {
    mPipelineNameFileReaderConfigsMap.erase(name);
}

MultilineConfig FileServer::GetMultilineConfig(const std::string& name) const {
    auto itr = mPipelineNameMultilineConfigsMap.find(name);
    if (itr != mPipelineNameMultilineConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

void FileServer::AddMultilineConfig(const string& name, const MultilineOptions* opts, const PipelineContext* ctx) {
    mPipelineNameMultilineConfigsMap[name] = make_pair(opts, ctx);
}

void FileServer::RemoveMultilineConfig(const string& name) {
    mPipelineNameMultilineConfigsMap.erase(name);
}

void FileServer::SaveContainerInfo(const std::string& pipeline,
                                   const std::shared_ptr<std::vector<DockerContainerPath>>& info) {
    mAllDockerContainerPathMap[pipeline] = info;
}

std::shared_ptr<std::vector<DockerContainerPath>> FileServer::GetAndRemoveContainerInfo(const std::string& pipeline) {
    auto iter = mAllDockerContainerPathMap.find(pipeline);
    if (iter == mAllDockerContainerPathMap.end()) {
        return std::make_shared<std::vector<DockerContainerPath>>();
    }
    mAllDockerContainerPathMap.erase(iter);
    return iter->second;
}

void FileServer::ClearContainerInfo() {
    mAllDockerContainerPathMap.clear();
}

uint32_t FileServer::GetExactlyOnceConcurrency(const std::string& name) const {
    auto itr = mPipelineNameEOConcurrencyMap.find(name);
    if (itr != mPipelineNameEOConcurrencyMap.end()) {
        return itr->second;
    }
    return 0;
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

void FileServer::AddExactlyOnceConcurrency(const std::string& name, uint32_t concurrency) {
    mPipelineNameEOConcurrencyMap[name] = concurrency;
}

void FileServer::RemoveExactlyOnceConcurrency(const string& name) {
    mPipelineNameEOConcurrencyMap.erase(name);
}

} // namespace logtail
