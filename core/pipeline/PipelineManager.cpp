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

#include "pipeline/PipelineManager.h"

#include "pipeline/PipelineValidator.h"

#include "file/FileServer.h"
#if defined(__ENTERPRISE__) && defined(__linux__)
#include "streamlog/StreamLogManager.h"
#endif
#ifdef __linux__
#include "observer/ObserverManager.h"
#endif
#include "Go_pipeline/LogtailPlugin.h"
#include "sender/Sender.h"
#include "processor/LogProcess.h"
#include "config_manager/ConfigManager.h"
#include "PipelineManager.h"

using namespace std;

namespace logtail {
string ExtractInputType(const NewConfig& config);

void PipelineManager::Init() {
    PipelineValidator::GetInstance()->Init();
}

bool PipelineManager::LoadAllPipelines() {
    auto& allConfig = ConfigManager::GetInstance()->GetAllConfig();
    for (auto& kv : allConfig) {
        auto p = make_shared<Pipeline>();
        if (p->Init(*kv.second)) {
            mPipelineDict.emplace(kv.first, p);
        } else {
            LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
                                                   "pipeline " + kv.second->mConfigName + " init failed",
                                                   kv.second->GetProjectName(),
                                                   kv.second->GetCategory(),
                                                   kv.second->mRegion);
            LOG_WARNING(sLogger,
                        ("pipeline init failed", kv.second->mConfigName)("project", kv.second->GetProjectName())(
                            "logstore", kv.second->GetCategory())("region", kv.second->mRegion));
        }
    }
    return true;
}

bool PipelineManager::RemoveAllPipelines() {
    mPipelineDict.clear();
    return true;
}

shared_ptr<Pipeline> PipelineManager::FindPipelineByName(const string& configName) {
    auto it = mPipelineDict.find(configName);
    if (it != mPipelineDict.end()) {
        return it->second;
    }
    return nullptr;
}

void logtail::PipelineManager::UpdatePipelines(ConfigDiff& diff) {
    bool isInputObserverChanged = false, isInputFileChanged = false, isInputStreamChanged = false;
    if (!mIsFirstUpdate) {
        for (const auto& name : diff.mRemoved) {
            CheckIfInputUpdated(mPipelineDict[name]->GetConfig(), isInputObserverChanged, isInputFileChanged, isInputStreamChanged);
        }
        for (const auto& config : diff.mModified) {
            CheckIfInputUpdated(config, isInputObserverChanged, isInputFileChanged, isInputStreamChanged);
        }
        for (const auto& config : diff.mAdded) {
            CheckIfInputUpdated(config, isInputObserverChanged, isInputFileChanged, isInputStreamChanged);
        }
        
#if defined(__ENTERPRISE__) && defined(__linux__)
        if (isInputStreamChanged) {
            StreamLogManager::GetInstance()->ShutdownConfigUsage();
        }
#endif
#ifdef __linux__
        if (isInputObserverChanged) {
            ObserverManager::GetInstance()->HoldOn(false);
        }
#endif
        if (isInputFileChanged) {
            FileServer::GetInstance()->Pause();
        }
        LogtailPlugin::GetInstance()->HoldOn(false);
    }

    for (const auto& name : diff.mRemoved) {
        mPipelineDict[name]->Stop(true);
        mPipelineDict.erase(name);
    }
    for (auto& config : diff.mModified) {
        auto p = BuildPipeline(std::move(config));
        if (!p) {
            mPipelineDict[config.mName]->Stop(true);
            mPipelineDict.erase(config.mName);
            continue;
        }
        mPipelineDict[config.mName]->Stop(false);
        mPipelineDict[config.mName] = p;
        p->Start();
    }
    for (auto& config : diff.mAdded) {
        auto p = BuildPipeline(std::move(config));
        if (!p) {
            continue;
        }
        mPipelineDict[config.mName] = p;
        p->Start();
    }
    for (auto& name : diff.mUnchanged) {
        mPipelineDict[name]->LoadGoPipelines();
    }
    // 在Flusher改造完成前，先不执行如下步骤，不会造成太大
    // Sender::CleanUnusedAk();

    if (mIsFirstUpdate) {
        LogProcess::GetInstance()->Start();
    }
    LogtailPlugin::GetInstance()->Resume();
    if (mIsFirstUpdate) {
        FileServer::GetInstance()->Start();
#ifdef __linux__
        ObserverManager::GetInstance()->Reload();
#endif
#if defined(__ENTERPRISE__) && defined(__linux__)
        if (AppConfig::GetInstance()->GetOpenStreamLog()) {
            StreamLogManager::GetInstance()->Init();
        }
#endif
    } else {
        if (isInputFileChanged) {
            FileServer::GetInstance()->Resume();
        }
#ifdef __linux__
        if (isInputObserverChanged) {
            ObserverManager::GetInstance()->Resume();
        }
#endif
#if defined(__ENTERPRISE__) && defined(__linux__)
        if (isInputStreamChanged) {
            StreamLogManager::GetInstance()->StartupConfigUsage();
        }
#endif
    }
}

std::shared_ptr<Pipeline> PipelineManager::BuildPipeline(NewConfig&& config) {
    std::shared_ptr<Pipeline> p = std::make_shared<Pipeline>();
    if (!p->Init(std::move(config))) {
        return std::make_shared<Pipeline>();
    }
    if (!PipelineValidator::GetInstance()->Validate(p)) {
        return std::make_shared<Pipeline>();
    }
    return p;
}

void PipelineManager::CheckIfInputUpdated(const NewConfig& config,
                                          bool& isInputObserverChanged,
                                          bool& isInputFileChanged,
                                          bool& isInputStreamChanged) {
    string inputType = ExtractInputType(config);
    if (inputType == "input_observer_network") {
        isInputObserverChanged = true;
    } else if (inputType == "input_file") {
        isInputFileChanged = true;
    } else if (inputType == "input_stream") {
        isInputStreamChanged = true;
    }
}

vector<string> PipelineManager::GetAllPipelineNames() const {
    vector<string> res;
    for (const auto& item : mPipelineDict) {
        res.push_back(item.first);
    }
    return res;
}

string ExtractInputType(const NewConfig& config) {
    return "";
}
} // namespace logtail
