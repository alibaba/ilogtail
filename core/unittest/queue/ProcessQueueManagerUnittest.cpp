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

#include <memory>

#include "models/PipelineEventGroup.h"
#include "pipeline/PipelineManager.h"
#include "pipeline/queue/ExactlyOnceQueueManager.h"
#include "pipeline/queue/ProcessQueueManager.h"
#include "pipeline/queue/QueueKeyManager.h"
#include "pipeline/queue/QueueParam.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class ProcessQueueManagerUnittest : public testing::Test {
public:
    void TestUpdateSameTypeQueue();
    void TestUpdateDifferentTypeQueue();
    void TestDeleteQueue();
    void TestSetQueueUpstreamAndDownStream();
    void TestPushQueue();
    void TestPopItem();
    void TestIsAllQueueEmpty();
    void OnPipelineUpdate();

protected:
    static void SetUpTestCase() { sProcessQueueManager = ProcessQueueManager::GetInstance(); }

    void TearDown() override {
        QueueKeyManager::GetInstance()->Clear();
        sProcessQueueManager->Clear();
        ExactlyOnceQueueManager::GetInstance()->Clear();
    }

private:
    static ProcessQueueManager* sProcessQueueManager;
    static PipelineContext sCtx;

    unique_ptr<ProcessQueueItem> GenerateItem() {
        PipelineEventGroup g(make_shared<SourceBuffer>());
        return make_unique<ProcessQueueItem>(std::move(g), 0);
    }
};

ProcessQueueManager* ProcessQueueManagerUnittest::sProcessQueueManager;
PipelineContext ProcessQueueManagerUnittest::sCtx;

void ProcessQueueManagerUnittest::TestUpdateSameTypeQueue() {
    // create queue
    //   and current index is invalid before creation
    QueueKey key = QueueKeyManager::GetInstance()->GetKey("test_config_1");
    PipelineContext ctx;
    ctx.SetConfigName("test_config_1");
    ctx.SetProcessQueueKey(key);
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(key, 0, ctx));
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mPriorityQueue[0].size());
    auto iter = sProcessQueueManager->mQueues[key].first;
    APSARA_TEST_TRUE(iter == prev(sProcessQueueManager->mPriorityQueue[0].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == iter);
    APSARA_TEST_EQUAL(sProcessQueueManager->mBoundedQueueParam.GetCapacity(), (*iter)->mCapacity);
    APSARA_TEST_EQUAL(sProcessQueueManager->mBoundedQueueParam.GetLowWatermark(),
                      static_cast<BoundedProcessQueue*>(iter->get())->mLowWatermark);
    APSARA_TEST_EQUAL(sProcessQueueManager->mBoundedQueueParam.GetHighWatermark(),
                      static_cast<BoundedProcessQueue*>(iter->get())->mHighWatermark);
    APSARA_TEST_EQUAL("test_config_1", (*iter)->GetConfigName());

    // create queue
    //   and current index is valid before creation
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(1, 0, sCtx));
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[1].first == prev(sProcessQueueManager->mPriorityQueue[0].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[0].first);

    // add more queue
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(2, 0, sCtx));
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(3, 0, sCtx));
    sProcessQueueManager->mCurrentQueueIndex.second = sProcessQueueManager->mQueues[2].first;

    // update queue with same priority
    APSARA_TEST_FALSE(sProcessQueueManager->CreateOrUpdateBoundedQueue(0, 0, sCtx));

    // update queue with different priority
    //   and current index not equal to the updated queue
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(0, 1, sCtx));
    APSARA_TEST_EQUAL(4U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(3U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mPriorityQueue[1].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[0].first == prev(sProcessQueueManager->mPriorityQueue[1].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[2].first);

    // update queue with different priority
    //   and current index equals to the updated queue
    //     and the updated queue is not the last in the list
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(2, 1, sCtx));
    APSARA_TEST_EQUAL(4U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mPriorityQueue[1].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[2].first == prev(sProcessQueueManager->mPriorityQueue[1].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[3].first);

    // update queue with different priority
    //   and current index equals to the updated queue
    //     and the updated queue is the last in the list
    //       and more queues exist
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(3, 1, sCtx));
    APSARA_TEST_EQUAL(4U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_EQUAL(3U, sProcessQueueManager->mPriorityQueue[1].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[3].first == prev(sProcessQueueManager->mPriorityQueue[1].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[1].first);

    // update queue with different priority
    //   and current index equals to the updated queue
    //     and the updated queue is the last in the list
    //       and no more queue exists
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(1, 1, sCtx));
    APSARA_TEST_EQUAL(4U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(0U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_EQUAL(4U, sProcessQueueManager->mPriorityQueue[1].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[1].first == prev(sProcessQueueManager->mPriorityQueue[1].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mPriorityQueue[0].end());

    // update queue with different priority
    //   and current index is invalid before update
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(0, 0, sCtx));
    APSARA_TEST_EQUAL(4U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_EQUAL(3U, sProcessQueueManager->mPriorityQueue[1].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[0].first == prev(sProcessQueueManager->mPriorityQueue[0].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[0].first);
}

void ProcessQueueManagerUnittest::TestUpdateDifferentTypeQueue() {
    sProcessQueueManager->CreateOrUpdateBoundedQueue(0, 0, sCtx);
    sProcessQueueManager->CreateOrUpdateBoundedQueue(1, 0, sCtx);

    // current index not equal to the updated queue
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateCircularQueue(1, 0, 100, sCtx));
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[1].first == prev(sProcessQueueManager->mPriorityQueue[0].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[0].first);

    // current index equals to the updated queue
    //   and the updated queue is not the last in the list
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateCircularQueue(0, 0, 100, sCtx));
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[0].first == prev(sProcessQueueManager->mPriorityQueue[0].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[1].first);

    // current index equals to the update queue
    //   and the updated queue is the last in the list
    sProcessQueueManager->mCurrentQueueIndex.second = prev(sProcessQueueManager->mPriorityQueue[0].end());
    APSARA_TEST_TRUE(sProcessQueueManager->CreateOrUpdateBoundedQueue(0, 0, sCtx));
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mQueues[0].first == prev(sProcessQueueManager->mPriorityQueue[0].end()));
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[1].first);
}

void ProcessQueueManagerUnittest::TestDeleteQueue() {
    QueueKey key1 = QueueKeyManager::GetInstance()->GetKey("test_config_1");
    QueueKey key2 = QueueKeyManager::GetInstance()->GetKey("test_config_2");
    QueueKey key3 = QueueKeyManager::GetInstance()->GetKey("test_config_3");
    QueueKey key4 = QueueKeyManager::GetInstance()->GetKey("test_config_4");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key1, 0, sCtx);
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key2, 0, sCtx);
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key3, 0, sCtx);
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key4, 0, sCtx);
    sProcessQueueManager->mCurrentQueueIndex.second = sProcessQueueManager->mQueues[key3].first;

    // current index not equal to the deleted queue
    APSARA_TEST_TRUE(sProcessQueueManager->DeleteQueue(key1));
    APSARA_TEST_EQUAL(3U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(3U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key3].first);
    APSARA_TEST_EQUAL("", QueueKeyManager::GetInstance()->GetName(key1));

    // current index equals to the deleted queue
    //   and the deleted queue is not the last in the list
    APSARA_TEST_TRUE(sProcessQueueManager->DeleteQueue(key3));
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(2U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key4].first);
    APSARA_TEST_EQUAL("", QueueKeyManager::GetInstance()->GetName(key3));

    // current index equals to the deleted queue
    //   and the deleted queue is the last in the list
    //     and more queues exist
    APSARA_TEST_TRUE(sProcessQueueManager->DeleteQueue(key4));
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key2].first);
    APSARA_TEST_EQUAL("", QueueKeyManager::GetInstance()->GetName(key4));

    // current index equals to the deleted queue
    //   and the deleted queue is the last in the list
    //     and no more queue exists
    APSARA_TEST_TRUE(sProcessQueueManager->DeleteQueue(key2));
    APSARA_TEST_EQUAL(0U, sProcessQueueManager->mQueues.size());
    APSARA_TEST_EQUAL(0U, sProcessQueueManager->mPriorityQueue[0].size());
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mPriorityQueue[0].end());
    APSARA_TEST_EQUAL("", QueueKeyManager::GetInstance()->GetName(key2));

    // queue not exist
    APSARA_TEST_FALSE(sProcessQueueManager->DeleteQueue(key1));
}

void ProcessQueueManagerUnittest::TestSetQueueUpstreamAndDownStream() {
    sProcessQueueManager->CreateOrUpdateBoundedQueue(0, 0, sCtx);

    // queue exists
    APSARA_TEST_TRUE(sProcessQueueManager->SetDownStreamQueues(0, vector<BoundedSenderQueueInterface*>()));
    APSARA_TEST_TRUE(sProcessQueueManager->SetFeedbackInterface(0, vector<FeedbackInterface*>()));
    // queue not exists
    APSARA_TEST_FALSE(sProcessQueueManager->SetDownStreamQueues(1, vector<BoundedSenderQueueInterface*>()));
    APSARA_TEST_FALSE(sProcessQueueManager->SetFeedbackInterface(1, vector<FeedbackInterface*>()));
}

void ProcessQueueManagerUnittest::TestPushQueue() {
    sProcessQueueManager->CreateOrUpdateBoundedQueue(0, 0, sCtx);
    ExactlyOnceQueueManager::GetInstance()->CreateOrUpdateQueue(1, 0, sCtx, vector<RangeCheckpointPtr>(5));

    // queue belongs to normal process queue
    APSARA_TEST_TRUE(sProcessQueueManager->IsValidToPush(0));
    APSARA_TEST_EQUAL(0, sProcessQueueManager->PushQueue(0, GenerateItem()));

    // queue belongs to exactly once process queue
    APSARA_TEST_TRUE(sProcessQueueManager->IsValidToPush(1));
    APSARA_TEST_EQUAL(0, sProcessQueueManager->PushQueue(1, GenerateItem()));

    // no queue exists
    APSARA_TEST_FALSE(sProcessQueueManager->IsValidToPush(2));
    APSARA_TEST_EQUAL(2, sProcessQueueManager->PushQueue(2, GenerateItem()));

    // invalid to push
    static_cast<BoundedProcessQueue*>(sProcessQueueManager->mQueues[0].first->get())->mValidToPush = false;
    APSARA_TEST_FALSE(sProcessQueueManager->IsValidToPush(0));
    APSARA_TEST_EQUAL(1, sProcessQueueManager->PushQueue(0, GenerateItem()));

    ExactlyOnceQueueManager::GetInstance()->mProcessQueues[1]->mValidToPush = false;
    APSARA_TEST_FALSE(sProcessQueueManager->IsValidToPush(1));
    APSARA_TEST_EQUAL(1, sProcessQueueManager->PushQueue(1, GenerateItem()));
}

void ProcessQueueManagerUnittest::TestPopItem() {
    unique_ptr<ProcessQueueItem> item;
    string configName;
    PipelineContext ctx;

    ctx.SetConfigName("test_config_1");
    QueueKey key1 = QueueKeyManager::GetInstance()->GetKey("test_config_1");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key1, 0, ctx);
    sProcessQueueManager->ValidatePop("test_config_1");
    ctx.SetConfigName("test_config_2");
    QueueKey key2 = QueueKeyManager::GetInstance()->GetKey("test_config_2");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key2, 1, ctx);
    sProcessQueueManager->ValidatePop("test_config_2");
    ctx.SetConfigName("test_config_3");
    QueueKey key3 = QueueKeyManager::GetInstance()->GetKey("test_config_3");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key3, 1, ctx);
    sProcessQueueManager->ValidatePop("test_config_3");
    ctx.SetConfigName("test_config_4");
    QueueKey key4 = QueueKeyManager::GetInstance()->GetKey("test_config_4");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key4, 1, ctx);
    sProcessQueueManager->ValidatePop("test_config_4");
    ctx.SetConfigName("test_config_5");
    ExactlyOnceQueueManager::GetInstance()->CreateOrUpdateQueue(5, 0, ctx, vector<RangeCheckpointPtr>(5));
    ExactlyOnceQueueManager::GetInstance()->ValidatePopProcessQueue("test_config_5");

    sProcessQueueManager->PushQueue(key2, GenerateItem());
    sProcessQueueManager->PushQueue(key3, GenerateItem());
    sProcessQueueManager->mCurrentQueueIndex = {1, prev(prev(sProcessQueueManager->mPriorityQueue[1].end()))};

    // the item comes from the queue between current index and queue list end
    APSARA_TEST_TRUE(sProcessQueueManager->PopItem(0, item, configName));
    APSARA_TEST_EQUAL("test_config_3", configName);
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mCurrentQueueIndex.first);
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key4].first);

    // the item comes from the queue between queue list and current index
    APSARA_TEST_TRUE(sProcessQueueManager->PopItem(0, item, configName));
    APSARA_TEST_EQUAL("test_config_2", configName);
    APSARA_TEST_EQUAL(1U, sProcessQueueManager->mCurrentQueueIndex.first);
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key3].first);

    sProcessQueueManager->PushQueue(key1, GenerateItem());
    // the item comes from queue list other than the one pointed by current index
    APSARA_TEST_TRUE(sProcessQueueManager->PopItem(0, item, configName));
    APSARA_TEST_EQUAL("test_config_1", configName);
    APSARA_TEST_EQUAL(0U, sProcessQueueManager->mCurrentQueueIndex.first);
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key1].first);

    sProcessQueueManager->mCurrentQueueIndex = {1, prev(sProcessQueueManager->mPriorityQueue[1].end())};
    sProcessQueueManager->PushQueue(5, GenerateItem());
    // the item comes from exactly once queue
    APSARA_TEST_TRUE(sProcessQueueManager->PopItem(0, item, configName));
    APSARA_TEST_EQUAL("test_config_5", configName);
    APSARA_TEST_EQUAL(0U, sProcessQueueManager->mCurrentQueueIndex.first);
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key1].first);

    sProcessQueueManager->mCurrentQueueIndex = {1, prev(sProcessQueueManager->mPriorityQueue[1].end())};
    // no item
    APSARA_TEST_FALSE(sProcessQueueManager->PopItem(0, item, configName));
    APSARA_TEST_EQUAL(0U, sProcessQueueManager->mCurrentQueueIndex.first);
    APSARA_TEST_TRUE(sProcessQueueManager->mCurrentQueueIndex.second == sProcessQueueManager->mQueues[key1].first);
}

void ProcessQueueManagerUnittest::TestIsAllQueueEmpty() {
    PipelineContext ctx;
    ctx.SetConfigName("test_config_1");
    QueueKey key1 = QueueKeyManager::GetInstance()->GetKey("test_config_1");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key1, 0, ctx);
    sProcessQueueManager->ValidatePop("test_config_1");

    ctx.SetConfigName("test_config_2");
    QueueKey key2 = QueueKeyManager::GetInstance()->GetKey("test_config_2");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key2, 1, ctx);
    sProcessQueueManager->ValidatePop("test_config_2");

    ctx.SetConfigName("test_config_3");
    QueueKey key3 = QueueKeyManager::GetInstance()->GetKey("test_config_3");
    ExactlyOnceQueueManager::GetInstance()->CreateOrUpdateQueue(key3, 0, ctx, vector<RangeCheckpointPtr>(5));
    ExactlyOnceQueueManager::GetInstance()->ValidatePopProcessQueue("test_config_3");

    ctx.SetConfigName("test_config_4");
    QueueKey key4 = QueueKeyManager::GetInstance()->GetKey("test_config_4");
    ExactlyOnceQueueManager::GetInstance()->CreateOrUpdateQueue(key4, 2, ctx, vector<RangeCheckpointPtr>(5));
    ExactlyOnceQueueManager::GetInstance()->ValidatePopProcessQueue("test_config_4");
    APSARA_TEST_TRUE(sProcessQueueManager->IsAllQueueEmpty());

    // non empty normal process queue
    sProcessQueueManager->PushQueue(0, GenerateItem());
    APSARA_TEST_FALSE(sProcessQueueManager->IsAllQueueEmpty());

    unique_ptr<ProcessQueueItem> item;
    string configName;
    sProcessQueueManager->PopItem(0, item, configName);
    APSARA_TEST_TRUE(sProcessQueueManager->IsAllQueueEmpty());

    // non empty exactly once process queue
    sProcessQueueManager->PushQueue(key3, GenerateItem());
    APSARA_TEST_FALSE(sProcessQueueManager->IsAllQueueEmpty());

    sProcessQueueManager->PopItem(0, item, configName);
    APSARA_TEST_TRUE(sProcessQueueManager->IsAllQueueEmpty());
}

void ProcessQueueManagerUnittest::OnPipelineUpdate() {
    PipelineContext ctx1, ctx2;
    ctx1.SetConfigName("test_config_1");
    ctx2.SetConfigName("test_config_2");
    QueueKey key = QueueKeyManager::GetInstance()->GetKey("test_config_1");
    sProcessQueueManager->CreateOrUpdateBoundedQueue(key, 0, ctx1);
    ExactlyOnceQueueManager::GetInstance()->CreateOrUpdateQueue(1, 0, ctx2, vector<RangeCheckpointPtr>(5));
    ExactlyOnceQueueManager::GetInstance()->CreateOrUpdateQueue(2, 0, ctx2, vector<RangeCheckpointPtr>(5));

    auto pipeline1 = make_shared<Pipeline>();
    auto pipeline2 = make_shared<Pipeline>();
    PipelineManager::GetInstance()->mPipelineNameEntityMap["test_config_1"] = pipeline1;
    PipelineManager::GetInstance()->mPipelineNameEntityMap["test_config_2"] = pipeline2;

    {
        auto item1 = GenerateItem();
        auto p1 = item1.get();
        sProcessQueueManager->PushQueue(key, std::move(item1));

        sProcessQueueManager->InvalidatePop("test_config_1", false);
        APSARA_TEST_FALSE((*sProcessQueueManager->mQueues[key].first)->mValidToPop);
        APSARA_TEST_EQUAL(pipeline1, p1->mPipeline);

        auto item2 = GenerateItem();
        auto p2 = item2.get();
        sProcessQueueManager->PushQueue(key, std::move(item2));

        auto pipeline3 = make_shared<Pipeline>();
        PipelineManager::GetInstance()->mPipelineNameEntityMap["test_config_1"] = pipeline3;

        sProcessQueueManager->InvalidatePop("test_config_1", false);
        APSARA_TEST_FALSE((*sProcessQueueManager->mQueues[key].first)->mValidToPop);
        APSARA_TEST_EQUAL(pipeline1, p1->mPipeline);
        APSARA_TEST_EQUAL(pipeline3, p2->mPipeline);

        auto item3 = GenerateItem();
        auto p3 = item3.get();
        sProcessQueueManager->PushQueue(key, std::move(item3));

        sProcessQueueManager->InvalidatePop("test_config_1", true);
        APSARA_TEST_FALSE((*sProcessQueueManager->mQueues[key].first)->mValidToPop);
        APSARA_TEST_EQUAL(pipeline1, p1->mPipeline);
        APSARA_TEST_EQUAL(pipeline3, p2->mPipeline);
        APSARA_TEST_EQUAL(nullptr, p3->mPipeline);

        sProcessQueueManager->ValidatePop("test_config_1");
        APSARA_TEST_TRUE((*sProcessQueueManager->mQueues[key].first)->mValidToPop);
    }
    {
        auto item1 = GenerateItem();
        auto p1 = item1.get();
        sProcessQueueManager->PushQueue(1, std::move(item1));

        auto item2 = GenerateItem();
        auto p2 = item2.get();
        sProcessQueueManager->PushQueue(2, std::move(item2));

        sProcessQueueManager->InvalidatePop("test_config_2", false);
        APSARA_TEST_FALSE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[1]->mValidToPop);
        APSARA_TEST_FALSE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[2]->mValidToPop);
        APSARA_TEST_EQUAL(pipeline2, p1->mPipeline);
        APSARA_TEST_EQUAL(pipeline2, p2->mPipeline);

        auto item3 = GenerateItem();
        auto p3 = item3.get();
        sProcessQueueManager->PushQueue(1, std::move(item3));

        auto item4 = GenerateItem();
        auto p4 = item4.get();
        sProcessQueueManager->PushQueue(2, std::move(item4));

        auto pipeline3 = make_shared<Pipeline>();
        PipelineManager::GetInstance()->mPipelineNameEntityMap["test_config_2"] = pipeline3;

        sProcessQueueManager->InvalidatePop("test_config_2", false);
        APSARA_TEST_FALSE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[1]->mValidToPop);
        APSARA_TEST_FALSE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[2]->mValidToPop);
        APSARA_TEST_EQUAL(pipeline2, p1->mPipeline);
        APSARA_TEST_EQUAL(pipeline2, p2->mPipeline);
        APSARA_TEST_EQUAL(pipeline3, p3->mPipeline);
        APSARA_TEST_EQUAL(pipeline3, p4->mPipeline);

        auto item5 = GenerateItem();
        auto p5 = item5.get();
        sProcessQueueManager->PushQueue(1, std::move(item5));

        auto item6 = GenerateItem();
        auto p6 = item6.get();
        sProcessQueueManager->PushQueue(2, std::move(item6));

        sProcessQueueManager->InvalidatePop("test_config_2", true);
        APSARA_TEST_FALSE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[1]->mValidToPop);
        APSARA_TEST_FALSE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[2]->mValidToPop);
        APSARA_TEST_EQUAL(pipeline2, p1->mPipeline);
        APSARA_TEST_EQUAL(pipeline2, p2->mPipeline);
        APSARA_TEST_EQUAL(pipeline3, p3->mPipeline);
        APSARA_TEST_EQUAL(pipeline3, p4->mPipeline);
        APSARA_TEST_EQUAL(nullptr, p5->mPipeline);
        APSARA_TEST_EQUAL(nullptr, p6->mPipeline);

        sProcessQueueManager->ValidatePop("test_config_2");
        APSARA_TEST_TRUE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[1]->mValidToPop);
        APSARA_TEST_TRUE(ExactlyOnceQueueManager::GetInstance()->mProcessQueues[2]->mValidToPop);
    }
}

UNIT_TEST_CASE(ProcessQueueManagerUnittest, TestUpdateSameTypeQueue)
UNIT_TEST_CASE(ProcessQueueManagerUnittest, TestUpdateDifferentTypeQueue)
UNIT_TEST_CASE(ProcessQueueManagerUnittest, TestDeleteQueue)
UNIT_TEST_CASE(ProcessQueueManagerUnittest, TestSetQueueUpstreamAndDownStream)
UNIT_TEST_CASE(ProcessQueueManagerUnittest, TestPushQueue)
UNIT_TEST_CASE(ProcessQueueManagerUnittest, TestPopItem)
UNIT_TEST_CASE(ProcessQueueManagerUnittest, TestIsAllQueueEmpty)
UNIT_TEST_CASE(ProcessQueueManagerUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
