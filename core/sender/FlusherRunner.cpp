// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sender/FlusherRunner.h"

#include "app_config/AppConfig.h"
#include "application/Application.h"
#include "common/LogtailCommonFlags.h"
#include "common/StringTools.h"
#include "flusher/sls/DiskBufferWriter.h"
#include "logger/Logger.h"
#include "monitor/LogtailAlarm.h"
#include "queue/SenderQueueItem.h"
#include "queue/SenderQueueManager.h"
#include "sink/http/HttpRequest.h"
#include "sink/http/HttpSink.h"
// TODO: temporarily used here
#include "flusher/sls/PackIdManager.h"
#include "flusher/sls/SLSClientManager.h"

using namespace std;

DEFINE_FLAG_INT32(check_send_client_timeout_interval, "", 600);

static const int SEND_BLOCK_COST_TIME_ALARM_INTERVAL_SECOND = 3;
static const int LOG_GROUP_WAIT_IN_QUEUE_ALARM_INTERVAL_SECOND = 6;

namespace logtail {

bool FlusherRunner::Init() {
    srand(time(nullptr));
    mLastDaemonRunTime = time(nullptr);
    mLastSendDataTime = time(nullptr);
    mLastCheckSendClientTime = time(nullptr);

    mThreadRes = async(launch::async, &FlusherRunner::Run, this);
    return true;
}

void FlusherRunner::Stop() {
    mIsFlush = true;
    future_status s = mThreadRes.wait_for(chrono::seconds(10));
    if (s == future_status::ready) {
        LOG_INFO(sLogger, ("flusher runner", "stopped successfully"));
    } else {
        LOG_WARNING(sLogger, ("flusher runner", "forced to stopped"));
    }
}

void FlusherRunner::PushToHttpSink(SenderQueueItem* item) {
    if (!BOOL_FLAG(enable_full_drain_mode) && item->mFlusher->Name() == "flusher_sls"
        && Application::GetInstance()->IsExiting()) {
        DiskBufferWriter::GetInstance()->PushToDiskBuffer(item, 3);
        SubSendingBufferCount();
        SenderQueueManager::GetInstance()->RemoveItem(item->mFlusher->GetQueueKey(), item);
        return;
    }
    unique_ptr<HttpRequest> req = item->mFlusher->BuildRequest(item);
    item->mLastSendTime = time(nullptr);
    HttpSink::GetInstance()->AddRequest(std::move(req));
}

void FlusherRunner::RegisterSink(const std::string& flusher, SinkType type) {
    mFlusherSinkMap[flusher] = type;
}

void FlusherRunner::Run() {
    LOG_INFO(sLogger, ("flusher runner", "started"));
    while (true) {
        int32_t curTime = time(NULL);
        SenderQueueManager::GetInstance()->Wait(1000);
        vector<SenderQueueItem*> items;
        if (Application::GetInstance()->IsExiting()) {
            SenderQueueManager::GetInstance()->GetAllAvailableItems(items, false);
        } else {
            SenderQueueManager::GetInstance()->GetAllAvailableItems(items);
        }
        mLastDaemonRunTime = curTime;

        // smoothing send tps, walk around webserver load burst
        uint32_t bufferPackageCount = items.size();
        if (!Application::GetInstance()->IsExiting() && AppConfig::GetInstance()->IsSendRandomSleep()) {
            int64_t sleepMicroseconds = 0;
            if (bufferPackageCount < 20)
                sleepMicroseconds = (rand() % 30) * 10000; // 0ms ~ 300ms
            else if (bufferPackageCount < 30)
                sleepMicroseconds = (rand() % 20) * 10000; // 0ms ~ 200ms
            else if (bufferPackageCount < 40)
                sleepMicroseconds = (rand() % 10) * 10000; // 0ms ~ 100ms
            if (sleepMicroseconds > 0)
                usleep(sleepMicroseconds);
        }

        for (auto itr = items.begin(); itr != items.end(); ++itr) {
            int32_t logGroupWaitTime = curTime - (*itr)->mEnqueTime;
            if (logGroupWaitTime > LOG_GROUP_WAIT_IN_QUEUE_ALARM_INTERVAL_SECOND) {
                LOG_WARNING(sLogger,
                            ("log group wait in queue for too long, may blocked by concurrency or quota",
                             "")("log group wait time", logGroupWaitTime));
                LogtailAlarm::GetInstance()->SendAlarm(LOG_GROUP_WAIT_TOO_LONG_ALARM,
                                                       "log group wait in queue for too long, log group wait time "
                                                           + ToString(logGroupWaitTime));
            }

            mLastSendDataTime = curTime;
            if (!Application::GetInstance()->IsExiting() && AppConfig::GetInstance()->IsSendFlowControl()) {
                RateLimiter::FlowControl((*itr)->mRawSize, mSendLastTime, mSendLastByte, true);
            }
            int32_t beforeSleepTime = time(NULL);
            while (!Application::GetInstance()->IsExiting()
                   && GetSendingBufferCount() >= AppConfig::GetInstance()->GetSendRequestConcurrency()) {
                usleep(10 * 1000);
            }
            int32_t afterSleepTime = time(NULL);
            int32_t blockCostTime = afterSleepTime - beforeSleepTime;
            if (blockCostTime > SEND_BLOCK_COST_TIME_ALARM_INTERVAL_SECOND) {
                LOG_WARNING(
                    sLogger,
                    ("sending log group blocked too long because send concurrency reached limit. current "
                     "concurrency used",
                     GetSendingBufferCount())("max concurrency", AppConfig::GetInstance()->GetSendRequestConcurrency())(
                        "blocked time", blockCostTime));
                LogtailAlarm::GetInstance()->SendAlarm(SENDING_COSTS_TOO_MUCH_TIME_ALARM,
                                                       "sending log group blocked for too much time, cost "
                                                           + ToString(blockCostTime));
            }

            ++mSendingBufferCount;
            Dispatch(*itr);
        }

        // TODO: move the following logic to scheduler
        if ((time(NULL) - mLastCheckSendClientTime) > INT32_FLAG(check_send_client_timeout_interval)) {
            SLSClientManager::GetInstance()->CleanTimeoutClient();
            PackIdManager::GetInstance()->CleanTimeoutEntry();
            mLastCheckSendClientTime = time(NULL);
        }

        if (mIsFlush && SenderQueueManager::GetInstance()->IsAllQueueEmpty()) {
            break;
        }
    }
}

void FlusherRunner::Dispatch(SenderQueueItem* item) {
    auto iter = mFlusherSinkMap.find(item->mFlusher->Name());
    if (iter == mFlusherSinkMap.end()) {
        LOG_ERROR(sLogger, ("unexpected error", "no sink type found for flusher")("flusher", item->mFlusher->Name()));
        SenderQueueManager::GetInstance()->RemoveItem(item->mFlusher->GetQueueKey(), item);
        return;
    }

    switch (iter->second) {
        case SinkType::HTTP:
            PushToHttpSink(item);
            break;
        default:
            SubSendingBufferCount();
            SenderQueueManager::GetInstance()->RemoveItem(item->mFlusher->GetQueueKey(), item);
            break;
    }
}

} // namespace logtail
