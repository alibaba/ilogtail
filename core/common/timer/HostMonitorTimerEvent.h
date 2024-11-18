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

#pragma once

#include <chrono>
#include <string>

#include "QueueKey.h"
#include "timer/TimerEvent.h"

namespace logtail {

class HostMonitorTimerEvent : public TimerEvent {
public:
    HostMonitorTimerEvent(std::chrono::steady_clock::time_point execTime,
                          size_t interval,
                          std::string configName,
                          std::string collectorName,
                          QueueKey processQueueKey)
        : TimerEvent(execTime),
          mConfigName(std::move(configName)),
          mCollectorName(collectorName),
          mProcessQueueKey(processQueueKey),
          mInputIdx(0) {
        mInterval = std::chrono::seconds(interval);
    }

    bool IsValid() const override;
    bool Execute() override;

    const std::string GetConfigName() const { return mConfigName; }
    const std::string GetCollectorName() const { return mCollectorName; }
    const QueueKey GetProcessQueueKey() const { return mProcessQueueKey; }
    int GetInputIndex() const { return mInputIdx; }
    const std::chrono::seconds GetInterval() const { return mInterval; }
    void ResetForNextExec() { SetExecTime(GetExecTime() + mInterval); }

private:
    std::string mConfigName;
    std::string mCollectorName;
    QueueKey mProcessQueueKey;
    int mInputIdx;
    std::chrono::seconds mInterval;
};

} // namespace logtail
