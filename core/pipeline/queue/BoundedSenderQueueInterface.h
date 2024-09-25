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

#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include "common/FeedbackInterface.h"
#include "pipeline/limiter/ConcurrencyLimiter.h"
#include "pipeline/limiter/RateLimiter.h"
#include "pipeline/queue/BoundedQueueInterface.h"
#include "pipeline/queue/QueueKey.h"
#include "pipeline/queue/SenderQueueItem.h"

namespace logtail {

class Flusher;

// not thread-safe, should be protected explicitly by queue manager
class BoundedSenderQueueInterface : public BoundedQueueInterface<std::unique_ptr<SenderQueueItem>> {
public:
    static void SetFeedback(FeedbackInterface* feedback);

    BoundedSenderQueueInterface(
        size_t cap, size_t low, size_t high, QueueKey key, const std::string& flusherId, const PipelineContext& ctx);

    bool Pop(std::unique_ptr<SenderQueueItem>& item) override { return false; }

    virtual bool Remove(SenderQueueItem* item) = 0;
    
    virtual void GetAllAvailableItems(std::vector<SenderQueueItem*>& items, bool withLimits = true) = 0;

    void DecreaseSendingCnt();
    void OnSendingSuccess();
    void SetRateLimiter(uint32_t maxRate);
    void SetConcurrencyLimiters(std::vector<std::shared_ptr<ConcurrencyLimiter>>&& limiters);

#ifdef APSARA_UNIT_TEST_MAIN
    std::optional<RateLimiter>& GetRateLimiter() { return mRateLimiter; }
    std::vector<std::shared_ptr<ConcurrencyLimiter>>& GetConcurrencyLimiters() { return mConcurrencyLimiters; }
#endif

protected:
    static FeedbackInterface* sFeedback;

    void GiveFeedback() const override;
    void Reset(size_t cap, size_t low, size_t high);

    std::optional<RateLimiter> mRateLimiter;
    std::vector<std::shared_ptr<ConcurrencyLimiter>> mConcurrencyLimiters;

    std::queue<std::unique_ptr<SenderQueueItem>> mExtraBuffer;

    IntGaugePtr mExtraBufferSize;
    IntGaugePtr mExtraBufferDataSizeBytes;
};

} // namespace logtail
