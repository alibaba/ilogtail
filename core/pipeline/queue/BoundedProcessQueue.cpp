// Copyright 2024 iLogtail Authors
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

#include "pipeline/queue/BoundedProcessQueue.h"

using namespace std;

namespace logtail {

BoundedProcessQueue::BoundedProcessQueue(
    size_t cap, size_t low, size_t high, int64_t key, uint32_t priority, const PipelineContext& ctx)
    : QueueInterface(key, cap, ctx),
      BoundedQueueInterface(key, cap, low, high, ctx),
      ProcessQueueInterface(key, cap, priority, ctx) {
    if (ctx.IsExactlyOnceEnabled()) {
        mMetricsRecordRef.AddLabels({{METRIC_LABEL_KEY_EXACTLY_ONCE_FLAG, "true"}});
    }
    WriteMetrics::GetInstance()->CommitMetricsRecordRef(mMetricsRecordRef);
}

bool BoundedProcessQueue::Push(unique_ptr<ProcessQueueItem>&& item) {
    if (!IsValidToPush()) {
        return false;
    }
    item->mEnqueTime = chrono::system_clock::now();
    auto size = item->mEventGroup.DataSize();
    mQueue.push(std::move(item));
    ChangeStateIfNeededAfterPush();

    mInItemsCnt->Add(1);
    mInItemDataSizeBytes->Add(size);
    mQueueSize->Set(Size());
    mQueueDataSizeByte->Add(size);
    mValidToPushFlag->Set(IsValidToPush());
    return true;
}

bool BoundedProcessQueue::Pop(unique_ptr<ProcessQueueItem>& item) {
    if (!IsValidToPop()) {
        return false;
    }
    item = std::move(mQueue.front());
    mQueue.pop();
    item->AddPipelineInProcessingCnt(GetConfigName());
    if (ChangeStateIfNeededAfterPop()) {
        GiveFeedback();
    }

    mOutItemsCnt->Add(1);
    mTotalDelayMs->Add(
        chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - item->mEnqueTime).count());
    mQueueSize->Set(Size());
    mQueueDataSizeByte->Sub(item->mEventGroup.DataSize());
    mValidToPushFlag->Set(IsValidToPush());
    return true;
}

void BoundedProcessQueue::InvalidatePop() {
    ProcessQueueInterface::InvalidatePop();
    std::size_t size = mQueue.size();
    auto pipeline = PipelineManager::GetInstance()->FindConfigByName(GetConfigName());
    if (pipeline) {
        for (std::size_t i = 0; i < size; ++i) {
            mQueue.front()->mPipeline = pipeline;
            mQueue.push(std::move(mQueue.front()));
            mQueue.pop();
        }
    }
}

void BoundedProcessQueue::SetUpStreamFeedbacks(vector<FeedbackInterface*>&& feedbacks) {
    mUpStreamFeedbacks.clear();
    for (auto& item : feedbacks) {
        if (item == nullptr) {
            // should not happen
            continue;
        }
        mUpStreamFeedbacks.emplace_back(item);
    }
}

void BoundedProcessQueue::GiveFeedback() const {
    for (auto& item : mUpStreamFeedbacks) {
        item->Feedback(mKey);
    }
}

} // namespace logtail
