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

#include "pipeline/queue/SenderQueue.h"
#include "unittest/Unittest.h"
#include "unittest/queue/FeedbackInterfaceMock.h"

using namespace std;

namespace logtail {

class SenderQueueUnittest : public testing::Test {
public:
    void TestPush();
    void TestRemove();
    void TestGetAllAvailableItems();
    void TestMetric();

protected:
    static void SetUpTestCase() {
        sConcurrencyLimiter = make_shared<ConcurrencyLimiter>();
        sCtx.SetConfigName("test_config");
    }

    void SetUp() override {
        mQueue.reset(new SenderQueue(sCap, sLowWatermark, sHighWatermark, sKey, sFlusherId, sCtx));
        mQueue->SetConcurrencyLimiters(vector<shared_ptr<ConcurrencyLimiter>>{sConcurrencyLimiter});
        mQueue->mRateLimiter = RateLimiter(100);
        mQueue->SetFeedback(&sFeedback);
    }

    void TearDown() override {
        sFeedback.Clear();
        sConcurrencyLimiter->Reset();
    }

private:
    static PipelineContext sCtx;
    static const QueueKey sKey = 0;
    static const string sFlusherId;
    static const size_t sCap = 2;
    static const size_t sLowWatermark = 1;
    static const size_t sHighWatermark = 2;

    static const size_t sDataSize = 10;

    static FeedbackInterfaceMock sFeedback;
    static shared_ptr<ConcurrencyLimiter> sConcurrencyLimiter;

    unique_ptr<SenderQueueItem> GenerateItem();

    unique_ptr<SenderQueue> mQueue;
};

PipelineContext SenderQueueUnittest::sCtx;
const QueueKey SenderQueueUnittest::sKey;
const string SenderQueueUnittest::sFlusherId = "1";
const size_t SenderQueueUnittest::sDataSize;
shared_ptr<ConcurrencyLimiter> SenderQueueUnittest::sConcurrencyLimiter;
FeedbackInterfaceMock SenderQueueUnittest::sFeedback;

void SenderQueueUnittest::TestPush() {
    // normal push
    APSARA_TEST_TRUE(mQueue->Push(GenerateItem()));
    APSARA_TEST_EQUAL(1U, mQueue->Size());
    APSARA_TEST_FALSE(mQueue->Full());
    APSARA_TEST_TRUE(mQueue->IsValidToPush());

    // reach high water mark
    APSARA_TEST_TRUE(mQueue->Push(GenerateItem()));
    APSARA_TEST_EQUAL(2U, mQueue->Size());
    APSARA_TEST_TRUE(mQueue->Full());
    APSARA_TEST_FALSE(mQueue->IsValidToPush());

    // full
    APSARA_TEST_TRUE(mQueue->Push(GenerateItem()));
    APSARA_TEST_EQUAL(2U, mQueue->Size());
    APSARA_TEST_TRUE(mQueue->Full());
    APSARA_TEST_FALSE(mQueue->IsValidToPush());
    APSARA_TEST_EQUAL(1U, mQueue->mExtraBuffer.size());
}

void SenderQueueUnittest::TestRemove() {
    vector<SenderQueueItem*> items;
    for (size_t i = 0; i <= sCap; ++i) {
        auto item = GenerateItem();
        items.emplace_back(item.get());
        mQueue->Push(std::move(item));
    }

    // full
    APSARA_TEST_TRUE(mQueue->Remove(items[0]));
    APSARA_TEST_EQUAL(2U, mQueue->Size());
    APSARA_TEST_TRUE(mQueue->Full());
    APSARA_TEST_FALSE(mQueue->IsValidToPush());
    APSARA_TEST_EQUAL(0U, mQueue->mExtraBuffer.size());

    // drop to low water mark
    APSARA_TEST_TRUE(mQueue->Remove(items[1]));
    APSARA_TEST_EQUAL(1U, mQueue->Size());
    APSARA_TEST_FALSE(mQueue->Full());
    APSARA_TEST_TRUE(mQueue->IsValidToPush());
    APSARA_TEST_TRUE(sFeedback.HasFeedback(0));

    // not existed
    APSARA_TEST_FALSE(mQueue->Remove(items[0]));
}

void SenderQueueUnittest::TestGetAllAvailableItems() {
    vector<SenderQueueItem*> items;
    for (size_t i = 0; i <= sCap; ++i) {
        auto item = GenerateItem();
        items.emplace_back(item.get());
        mQueue->Push(std::move(item));
    }
    {
        // no limits
        vector<SenderQueueItem*> items;
        mQueue->GetAllAvailableItems(items, false);
        APSARA_TEST_EQUAL(2U, items.size());
        for (auto& item : items) {
            item->mStatus = SendingStatus::IDLE;
        }
    }
    {
        // with limits, limited by concurrency limiter
        mQueue->mRateLimiter->mMaxSendBytesPerSecond = 100;
        sConcurrencyLimiter->SetLimit(1);
        vector<SenderQueueItem*> items;
        mQueue->GetAllAvailableItems(items);
        APSARA_TEST_EQUAL(1U, items.size());
        APSARA_TEST_EQUAL(sDataSize, mQueue->mRateLimiter->mLastSecondTotalBytes);
        APSARA_TEST_EQUAL(0, sConcurrencyLimiter->GetLimit());
        for (auto& item : items) {
            item->mStatus = SendingStatus::IDLE;
        }
        mQueue->mRateLimiter->mLastSecondTotalBytes = 0;
    }
    {
        // with limits, limited by rate limiter
        mQueue->mRateLimiter->mMaxSendBytesPerSecond = 5;
        sConcurrencyLimiter->SetLimit(3);
        vector<SenderQueueItem*> items;
        mQueue->GetAllAvailableItems(items);
        APSARA_TEST_EQUAL(1U, items.size());
        APSARA_TEST_EQUAL(sDataSize, mQueue->mRateLimiter->mLastSecondTotalBytes);
        APSARA_TEST_EQUAL(2, sConcurrencyLimiter->GetLimit());
        mQueue->mRateLimiter->mLastSecondTotalBytes = 0;
    }
    {
        // with limits, does not work
        mQueue->mRateLimiter->mMaxSendBytesPerSecond = 100;
        sConcurrencyLimiter->SetLimit(3);
        vector<SenderQueueItem*> items;
        mQueue->GetAllAvailableItems(items);
        APSARA_TEST_EQUAL(1U, items.size());
        APSARA_TEST_EQUAL(sDataSize, mQueue->mRateLimiter->mLastSecondTotalBytes);
        APSARA_TEST_EQUAL(2, sConcurrencyLimiter->GetLimit());
    }
}

void SenderQueueUnittest::TestMetric() {
    APSARA_TEST_EQUAL(5U, mQueue->mMetricsRecordRef->GetLabels()->size());
    APSARA_TEST_TRUE(mQueue->mMetricsRecordRef.HasLabel(METRIC_LABEL_PROJECT, ""));
    APSARA_TEST_TRUE(mQueue->mMetricsRecordRef.HasLabel(METRIC_LABEL_CONFIG_NAME, "test_config"));
    APSARA_TEST_TRUE(mQueue->mMetricsRecordRef.HasLabel(METRIC_LABEL_COMPONENT_NAME, "sender_queue"));
    APSARA_TEST_TRUE(mQueue->mMetricsRecordRef.HasLabel(METRIC_LABEL_QUEUE_TYPE, "bounded"));
    APSARA_TEST_TRUE(mQueue->mMetricsRecordRef.HasLabel(METRIC_LABEL_FLUSHER_NODE_ID, sFlusherId));

    auto item1 = GenerateItem();
    auto dataSize = item1->mData.size();
    auto ptr1 = item1.get();
    mQueue->Push(std::move(item1));

    APSARA_TEST_EQUAL(1U, mQueue->mInItemsCnt->GetValue());
    APSARA_TEST_EQUAL(dataSize, mQueue->mInItemsSizeByte->GetValue());
    APSARA_TEST_EQUAL(1U, mQueue->mQueueSize->GetValue());
    APSARA_TEST_EQUAL(dataSize, mQueue->mQueueDataSizeByte->GetValue());
    APSARA_TEST_EQUAL(1U, mQueue->mValidToPushFlag->GetValue());

    auto item2 = GenerateItem();
    auto ptr2 = item2.get();
    mQueue->Push(std::move(item2));

    mQueue->Push(GenerateItem());

    APSARA_TEST_EQUAL(3U, mQueue->mInItemsCnt->GetValue());
    APSARA_TEST_EQUAL(dataSize * 3, mQueue->mInItemsSizeByte->GetValue());
    APSARA_TEST_EQUAL(2U, mQueue->mQueueSize->GetValue());
    APSARA_TEST_EQUAL(dataSize * 2, mQueue->mQueueDataSizeByte->GetValue());
    APSARA_TEST_EQUAL(0U, mQueue->mValidToPushFlag->GetValue());
    APSARA_TEST_EQUAL(1U, mQueue->mExtraBufferCnt->GetValue());
    APSARA_TEST_EQUAL(dataSize, mQueue->mExtraBufferDataSizeByte->GetValue());

    mQueue->Remove(ptr1);
    APSARA_TEST_EQUAL(1U, mQueue->mOutItemsCnt->GetValue());
    APSARA_TEST_EQUAL(2U, mQueue->mQueueSize->GetValue());
    APSARA_TEST_EQUAL(dataSize * 2, mQueue->mQueueDataSizeByte->GetValue());
    APSARA_TEST_EQUAL(0U, mQueue->mValidToPushFlag->GetValue());
    APSARA_TEST_EQUAL(0U, mQueue->mExtraBufferCnt->GetValue());
    APSARA_TEST_EQUAL(0U, mQueue->mExtraBufferDataSizeByte->GetValue());

    mQueue->Remove(ptr2);
    APSARA_TEST_EQUAL(2U, mQueue->mOutItemsCnt->GetValue());
    APSARA_TEST_EQUAL(1U, mQueue->mQueueSize->GetValue());
    APSARA_TEST_EQUAL(dataSize, mQueue->mQueueDataSizeByte->GetValue());
    APSARA_TEST_EQUAL(1U, mQueue->mValidToPushFlag->GetValue());
}

unique_ptr<SenderQueueItem> SenderQueueUnittest::GenerateItem() {
    return make_unique<SenderQueueItem>("content", sDataSize, nullptr, sKey);
}

UNIT_TEST_CASE(SenderQueueUnittest, TestPush)
UNIT_TEST_CASE(SenderQueueUnittest, TestRemove)
UNIT_TEST_CASE(SenderQueueUnittest, TestGetAllAvailableItems)
UNIT_TEST_CASE(SenderQueueUnittest, TestMetric)

} // namespace logtail

UNIT_TEST_MAIN
