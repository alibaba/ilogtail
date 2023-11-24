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

#include "file_server/FileServer.h"
#include "go_pipeline/LogtailPlugin.h"
#ifdef __linux__
#include "observer/ObserverManager.h"
#endif
#include "processor/daemon/LogProcess.h"
#include "sender/Sender.h"
#if defined(__ENTERPRISE__) && defined(__linux__)
#include "streamlog/StreamLogManager.h"
#endif

DEFINE_FLAG_INT32(exit_flushout_duration, "exit process flushout duration", 20 * 1000);

using namespace std;

namespace logtail {

void logtail::PipelineManager::UpdatePipelines(ConfigDiff& diff) {
#ifndef APSARA_UNIT_TEST_MAIN
    // 过渡使用
    bool isInputObserverChanged = false, isInputFileChanged = false, isInputStreamChanged = false;
    if (!mIsFirstUpdate) {
        for (const auto& name : diff.mRemoved) {
            CheckIfInputUpdated(mPipelineNameEntityMap[name]->GetConfig()["inputs"][0],
                                isInputObserverChanged,
                                isInputFileChanged,
                                isInputStreamChanged);
        }
        for (const auto& config : diff.mModified) {
            CheckIfInputUpdated(*config.mInputs[0], isInputObserverChanged, isInputFileChanged, isInputStreamChanged);
        }
        for (const auto& config : diff.mAdded) {
            CheckIfInputUpdated(*config.mInputs[0], isInputObserverChanged, isInputFileChanged, isInputStreamChanged);
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
        LogProcess::GetInstance()->HoldOn();
        LogtailPlugin::GetInstance()->HoldOn(false);
    }
#endif

    for (const auto& name : diff.mRemoved) {
        mPipelineNameEntityMap[name]->Stop(true);
        DecreasePluginUsageCnt(mPipelineNameEntityMap[name]->GetPluginStatistics());
        mPipelineNameEntityMap.erase(name);
    }
    for (auto& config : diff.mModified) {
        auto p = BuildPipeline(std::move(config));
        if (!p) {
            continue;
        }
        mPipelineNameEntityMap[config.mName]->Stop(false);
        DecreasePluginUsageCnt(mPipelineNameEntityMap[config.mName]->GetPluginStatistics());
        mPipelineNameEntityMap[config.mName] = p;
        p->Start();
    }
    for (auto& config : diff.mAdded) {
        auto p = BuildPipeline(std::move(config));
        if (!p) {
            continue;
        }
        mPipelineNameEntityMap[config.mName] = p;
        p->Start();
    }

#ifndef APSARA_UNIT_TEST_MAIN
    // 过渡使用
    for (auto& name : diff.mUnchanged) {
        mPipelineNameEntityMap[name]->LoadGoPipelines();
    }
    // 在Flusher改造完成前，先不执行如下步骤，不会造成太大影响
    // Sender::CleanUnusedAk();

    // 过渡使用
    LogtailPlugin::GetInstance()->Resume();
    if (mIsFirstUpdate) {
        LogProcess::GetInstance()->Start();
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
        LogProcess::GetInstance()->Resume();
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
        ConfigManager::GetInstance()->DeleteHandlers();
    }
#endif
}

shared_ptr<Pipeline> PipelineManager::FindPipelineByName(const string& configName) const {
    auto it = mPipelineNameEntityMap.find(configName);
    if (it != mPipelineNameEntityMap.end()) {
        return it->second;
    }
    return nullptr;
}

vector<string> PipelineManager::GetAllPipelineNames() const {
    vector<string> res;
    for (const auto& item : mPipelineNameEntityMap) {
        res.push_back(item.first);
    }
    return res;
}

string PipelineManager::GetPluginStatistics() const {
    Json::Value root;
    ScopedSpinLock lock(mPluginCntMapLock);
    for (const auto& item : mPluginCntMap) {
        for (const auto& plugin : item.second) {
            root[item.first][plugin.first] = Json::Value(plugin.second);
        }
    }
    return root.toStyledString();
}

void PipelineManager::StopAllPipelines() {
#if defined(__ENTERPRISE__) && defined(__linux__)
    StreamLogManager::GetInstance()->Shutdown();
#endif
#ifdef __linux__
    ObserverManager::GetInstance()->HoldOn(true);
#endif
    FileServer::GetInstance()->Stop();

    Sender::Instance()->SetQueueUrgent();
    bool logProcessFlushFlag = false;
    for (int i = 0; !logProcessFlushFlag && i < 500; ++i) {
        // deamon send thread may reset flush, so we should set flush every time
        Sender::Instance()->SetFlush();
        logProcessFlushFlag = LogProcess::GetInstance()->FlushOut(10);
    }
    if (!logProcessFlushFlag) {
        LOG_WARNING(sLogger, ("flush log process buffer", "fail"));
    } else {
        LOG_INFO(sLogger, ("flush log process buffer", "success"));
    }
    LogProcess::GetInstance()->HoldOn();

    LogtailPlugin::GetInstance()->HoldOn(true);

    if (!(Sender::Instance()->FlushOut(INT32_FLAG(exit_flushout_duration)))) {
        LOG_WARNING(sLogger, ("flush out sender data", "fail"));
    } else {
        LOG_INFO(sLogger, ("flush out sender data", "success"));
    }
}

shared_ptr<Pipeline> PipelineManager::BuildPipeline(Config&& config) {
    shared_ptr<Pipeline> p = make_shared<Pipeline>();
    // only config.mDetail is removed, other members can be safely used later
    if (!p->Init(std::move(config))) {
        return nullptr;
    }
    IncreasePluginUsageCnt(p->GetPluginStatistics());
    return p;
}

void PipelineManager::IncreasePluginUsageCnt(const unordered_map<string, unordered_map<string, uint32_t>>& statistics) {
    for (const auto& item : statistics) {
        for (const auto& plugin : item.second) {
            mPluginCntMap[item.first][plugin.first] += plugin.second;
        }
    }
}

void PipelineManager::DecreasePluginUsageCnt(const unordered_map<string, unordered_map<string, uint32_t>>& statistics) {
    for (const auto& item : statistics) {
        for (const auto& plugin : item.second) {
            mPluginCntMap[item.first][plugin.first] -= plugin.second;
        }
    }
}

void PipelineManager::CheckIfInputUpdated(const Json::Value& config,
                                          bool& isInputObserverChanged,
                                          bool& isInputFileChanged,
                                          bool& isInputStreamChanged) {
    string inputType = config["Type"].asString();
    if (inputType == "input_observer_network") {
        isInputObserverChanged = true;
    } else if (inputType == "input_file") {
        isInputFileChanged = true;
    } else if (inputType == "input_stream") {
        isInputStreamChanged = true;
    }
}

} // namespace logtail
