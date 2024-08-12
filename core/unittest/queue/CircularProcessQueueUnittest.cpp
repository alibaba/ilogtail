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

#include <memory>

#include "models/PipelineEventGroup.h"
#include "queue/CircularProcessQueue.h"
#include "queue/SenderQueue.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {
class CircularProcessQueueUnittest : public testing::Test {
public:
    void TestPush();
    void TestPop();
    void TestReset();

protected:
    static void SetUpTestCase() { sEventGroup.reset(new PipelineEventGroup(make_shared<SourceBuffer>())); }

    void SetUp() override {
        mQueue.reset(new CircularProcessQueue(sCap, sKey, 1, "test_config"));

        mSenderQueue1.reset(new SenderQueue(10, 0, 10, 0));
        mSenderQueue2.reset(new SenderQueue(10, 0, 10, 0));
        mQueue->SetDownStreamQueues(vector<BoundedSenderQueueInterface*>{mSenderQueue1.get(), mSenderQueue2.get()});
    }

private:
    static unique_ptr<PipelineEventGroup> sEventGroup;
    static const QueueKey sKey = 0;
    static const size_t sCap = 2;

    unique_ptr<ProcessQueueItem> GenerateItem() { return make_unique<ProcessQueueItem>(std::move(*sEventGroup), 0); }

    unique_ptr<CircularProcessQueue> mQueue;
    unique_ptr<BoundedSenderQueueInterface> mSenderQueue1;
    unique_ptr<BoundedSenderQueueInterface> mSenderQueue2;
};

unique_ptr<PipelineEventGroup> CircularProcessQueueUnittest::sEventGroup;

void CircularProcessQueueUnittest::TestPush() {
    APSARA_TEST_TRUE(mQueue->Push(GenerateItem()));
    APSARA_TEST_EQUAL(1U, mQueue->Size());

    APSARA_TEST_TRUE(mQueue->Push(GenerateItem()));
    APSARA_TEST_EQUAL(2U, mQueue->Size());

    APSARA_TEST_TRUE(mQueue->Push(GenerateItem()));
    APSARA_TEST_EQUAL(2U, mQueue->Size());
}

void CircularProcessQueueUnittest::TestPop() {
    unique_ptr<ProcessQueueItem> item;
    // nothing to pop
    APSARA_TEST_FALSE(mQueue->Pop(item));

    mQueue->Push(GenerateItem());
    // invalidate pop
    mQueue->InvalidatePop();
    APSARA_TEST_FALSE(mQueue->Pop(item));
    mQueue->ValidatePop();

    // downstream queues are not valid to push
    mSenderQueue1->mValidToPush = false;
    APSARA_TEST_FALSE(mQueue->Pop(item));
    mSenderQueue1->mValidToPush = true;

    APSARA_TEST_TRUE(mQueue->Pop(item));
}

void CircularProcessQueueUnittest::TestReset() {
    unique_ptr<ProcessQueueItem> item;
    {
        mQueue->Push(GenerateItem());
        mQueue->Pop(item);
        mQueue->Push(GenerateItem());
        mQueue->Reset(4);
        APSARA_TEST_EQUAL(5U, mQueue->Capacity());
        APSARA_TEST_EQUAL(1U, mQueue->Size());
        APSARA_TEST_EQUAL(0U, mQueue->mTail);
        APSARA_TEST_EQUAL(1U, mQueue->mHead);
        APSARA_TEST_TRUE(mQueue->mDownStreamQueues.empty());
    }
    {
        mQueue->Push(GenerateItem());
        mQueue->Pop(item);
        mQueue->Push(GenerateItem());
        mQueue->Pop(item);
        mQueue->Push(GenerateItem());
        mQueue->Push(GenerateItem());
        mQueue->Reset(2);
        APSARA_TEST_EQUAL(3U, mQueue->Capacity());
        APSARA_TEST_EQUAL(2U, mQueue->Size());
        APSARA_TEST_EQUAL(0U, mQueue->mTail);
        APSARA_TEST_EQUAL(2U, mQueue->mHead);
        APSARA_TEST_TRUE(mQueue->mDownStreamQueues.empty());
    }
}

UNIT_TEST_CASE(CircularProcessQueueUnittest, TestPush)
UNIT_TEST_CASE(CircularProcessQueueUnittest, TestPop)
UNIT_TEST_CASE(CircularProcessQueueUnittest, TestReset)

} // namespace logtail

UNIT_TEST_MAIN
