/*
 * Copyright 2024 iLogtail Authors
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

#include "HostMonitorInputRunner.h"

#include <memory>
#include <mutex>

#include "CollectorManager.h"
#include "Lock.h"
#include "LogEvent.h"
#include "Logger.h"
#include "ProcessQueueItem.h"
#include "ProcessQueueManager.h"
#include "ThreadPool.h"
#include "timer/HostMonitorTimerEvent.h"
#include "timer/TimerEvent.h"


namespace logtail {

HostMonitorInputRunner::HostMonitorInputRunner() {
    mTimer = std::make_shared<Timer>();
    mThreadPool = std::make_shared<ThreadPool>(3);
}

void HostMonitorInputRunner::UpdateCollector(const std::string& configName,
                                             const std::vector<std::string>& collectorNames,
                                             QueueKey processQueueKey) {
    WriteLock lock(mCollectorMapRWLock);
    mCollectorMap[configName] = collectorNames;
    for (const auto& collectorName : collectorNames) {
        LOG_INFO(sLogger, ("add new host monitor collector", configName)("collector", collectorName));
        mTimer->PushEvent(BuildTimerEvent(configName, collectorName, processQueueKey));
    }
}

void HostMonitorInputRunner::RemoveCollector(const std::string& configName) {
    WriteLock lock(mCollectorMapRWLock);
    mCollectorMap.erase(configName);
}

void HostMonitorInputRunner::Init() {
    std::lock_guard<std::mutex> lock(mStartMutex);
    if (mIsStarted) {
        return;
    }
    LOG_INFO(sLogger, ("HostMonitorInputRunner", "Start"));
    mIsStarted = true;

#ifndef APSARA_UNIT_TEST_MAIN
    mThreadPool->Start();
    mTimer->Init();
#endif
}

void HostMonitorInputRunner::Stop() {
    std::lock_guard<std::mutex> lock(mStartMutex);
    if (!mIsStarted) {
        return;
    }

    mIsStarted = false;
#ifndef APSARA_UNIT_TEST_MAIN
    mTimer->Stop();
    mThreadPool->Stop();
#endif
    LOG_INFO(sLogger, ("HostMonitorInputRunner", "Stop"));
}

bool HostMonitorInputRunner::HasRegisteredPlugins() const {
    ReadLock lock(mCollectorMapRWLock);
    return !mCollectorMap.empty();
}

bool HostMonitorInputRunner::IsCollectTaskValid(const std::string& configName, const std::string& collectorName) const {
    ReadLock lock(mCollectorMapRWLock);
    auto collectors = mCollectorMap.find(configName);
    if (collectors == mCollectorMap.end()) {
        return false;
    }
    for (const auto& collectorName : collectors->second) {
        if (collectorName == collectorName) {
            return true;
        }
    }
    return false;
}

void HostMonitorInputRunner::ScheduleOnce(HostMonitorTimerEvent* event) {
    // TODO: reuse event
    HostMonitorTimerEvent eventCopy = *event;
    mThreadPool->Add([this, eventCopy]() mutable {
        auto configName = eventCopy.GetConfigName();
        auto collectorName = eventCopy.GetCollectorName();
        auto processQueueKey = eventCopy.GetProcessQueueKey();
        PipelineEventGroup group(std::make_shared<SourceBuffer>());
        auto collector = CollectorManager::GetInstance()->GetCollector(collectorName);
        collector->Collect(group);

        std::unique_ptr<ProcessQueueItem> item
            = std::make_unique<ProcessQueueItem>(std::move(group), eventCopy.GetInputIndex());
        if (ProcessQueueManager::GetInstance()->IsValidToPush(processQueueKey)) {
            ProcessQueueManager::GetInstance()->PushQueue(processQueueKey, std::move(item));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // try again
            if (ProcessQueueManager::GetInstance()->IsValidToPush(processQueueKey)) {
                ProcessQueueManager::GetInstance()->PushQueue(processQueueKey, std::move(item));
            } else {
                LOG_WARNING(sLogger, ("process queue is full", "discard data")("config", configName));
            }
        }

        LOG_DEBUG(sLogger, ("schedule host monitor collector again", configName)("collector", collectorName));

        eventCopy.ResetForNextExec();
        mTimer->PushEvent(std::make_unique<HostMonitorTimerEvent>(eventCopy));
    });
}

std::unique_ptr<TimerEvent> HostMonitorInputRunner::BuildTimerEvent(const std::string& configName,
                                                                    const std::string& collectorName,
                                                                    QueueKey processQueueKey) {
    auto now = std::chrono::steady_clock::now();
    auto event = std::make_unique<HostMonitorTimerEvent>(
        now, DEFAULT_SCHEDULE_INTERVAL, configName, collectorName, processQueueKey);
    return event;
}


} // namespace logtail
