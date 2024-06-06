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


#include "aggregator/Aggregator.h"
#include "common/JsonUtil.h"
#include "unittest/Unittest.h"
#include "unittest/plugin/PluginMock.h"

using namespace std;

namespace logtail {

class AggregatorUnittest : public ::testing::Test {
public:
    void TestParamInit();
    void TestInitWithoutGroupBatch();
    void TestInitWithGroupBatch();
    void TestAddWithoutGroupBatch();
    void TestAddWithGroupBatch();
    void TestFlushEventQueueWithoutGroupBatch();
    void TestFlushEventQueueWithGroupBatch();
    void TestFlushGroupQueue();
    void TestFlushAllWithoutGroupBatch();
    void TestFlushAllWithGroupBatch();

protected:
    static void SetUpTestCase() { sFlusher = make_unique<FlusherMock>(); }

    void SetUp() override {
        mCtx.SetConfigName("test_config");
        sFlusher->SetContext(mCtx);
        sFlusher->SetMetricsRecordRef(FlusherMock::sName, "1");
    }

    void TearDown() override { TimeoutFlushManager::GetInstance()->mTimeoutRecords.clear(); }

private:
    PipelineEventGroup CreateEventGroup(size_t cnt);

    static unique_ptr<FlusherMock> sFlusher;

    PipelineContext mCtx;
};

unique_ptr<FlusherMock> AggregatorUnittest::sFlusher;

void AggregatorUnittest::TestParamInit() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 1;
    strategy.mMaxSizeBytes = 100;
    strategy.mTimeoutSecs = 3;
    {
        // empty config
        Aggregator<> aggregator;
        aggregator.Init(Json::Value(), sFlusher.get(), strategy);
        APSARA_TEST_EQUAL(1U, aggregator.mEventFlushStrategy.GetMaxCnt());
        APSARA_TEST_EQUAL(100U, aggregator.mEventFlushStrategy.GetMaxSizeBytes());
        APSARA_TEST_EQUAL(3U, aggregator.mEventFlushStrategy.GetTimeoutSecs());
    }
    {
        // invalid param
        Json::Value configJson;
        string configStr, errorMsg;
        configStr = R"(
            {
                "MaxSizeBytes": "1000",
                "MaxCnt": "10",
                "TimeoutSecs": "5"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));

        Aggregator<> aggregator;
        aggregator.Init(configJson, sFlusher.get(), strategy);
        APSARA_TEST_EQUAL(1U, aggregator.mEventFlushStrategy.GetMaxCnt());
        APSARA_TEST_EQUAL(100U, aggregator.mEventFlushStrategy.GetMaxSizeBytes());
        APSARA_TEST_EQUAL(3U, aggregator.mEventFlushStrategy.GetTimeoutSecs());
    }
}

void AggregatorUnittest::TestInitWithoutGroupBatch() {
    Json::Value configJson;
    string configStr, errorMsg;
    configStr = R"(
        {
            "MaxSizeBytes": 1000,
            "MaxCnt": 10,
            "TimeoutSecs": 5
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));

    Aggregator<> aggregator;
    aggregator.Init(configJson, sFlusher.get(), DefaultFlushStrategy());
    APSARA_TEST_EQUAL(10U, aggregator.mEventFlushStrategy.GetMaxCnt());
    APSARA_TEST_EQUAL(1000U, aggregator.mEventFlushStrategy.GetMaxSizeBytes());
    APSARA_TEST_EQUAL(5U, aggregator.mEventFlushStrategy.GetTimeoutSecs());
    APSARA_TEST_EQUAL(sFlusher.get(), aggregator.mFlusher);
}

void AggregatorUnittest::TestInitWithGroupBatch() {
    Json::Value configJson;
    string configStr, errorMsg;
    configStr = R"(
            {
                "MaxSizeBytes": 1000,
                "MaxCnt": 10,
                "TimeoutSecs": 5
            }
        )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));

    Aggregator<> aggregator;
    aggregator.Init(configJson, sFlusher.get(), DefaultFlushStrategy(), true);
    APSARA_TEST_EQUAL(10U, aggregator.mEventFlushStrategy.GetMaxCnt());
    APSARA_TEST_EQUAL(1000U, aggregator.mEventFlushStrategy.GetMaxSizeBytes());
    APSARA_TEST_EQUAL(3U, aggregator.mEventFlushStrategy.GetTimeoutSecs());
    APSARA_TEST_TRUE(aggregator.mGroupFlushStrategy);
    APSARA_TEST_EQUAL(2000U, aggregator.mGroupFlushStrategy->GetMaxSizeBytes());
    APSARA_TEST_EQUAL(2U, aggregator.mGroupFlushStrategy->GetTimeoutSecs());
    APSARA_TEST_TRUE(aggregator.mGroupQueue);
    APSARA_TEST_EQUAL(sFlusher.get(), aggregator.mFlusher);
}

void AggregatorUnittest::TestAddWithoutGroupBatch() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 3;
    strategy.mMaxSizeBytes = 1000;
    strategy.mTimeoutSecs = 3;

    Aggregator<> aggregator;
    aggregator.Init(Json::Value(), sFlusher.get(), strategy);

    // add to empty batch item
    vector<BatchedEventsList> res;
    PipelineEventGroup group1 = CreateEventGroup(2);
    size_t key = group1.GetTagsHash();
    SourceBuffer* buffer1 = group1.GetSourceBuffer().get();
    RangeCheckpoint* eoo1 = group1.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group1), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(2U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(0U, res.size());
    APSARA_TEST_EQUAL(1U, TimeoutFlushManager::GetInstance()->mTimeoutRecords.size());
    APSARA_TEST_EQUAL(1U, TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].size());
    TimeoutRecord& record = TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key));
    time_t updateTime = record.mUpdateTime;
    APSARA_TEST_EQUAL(3U, record.mTimeoutSecs);
    APSARA_TEST_EQUAL(sFlusher.get(), record.mFlusher);
    APSARA_TEST_EQUAL(key, record.mKey);
    APSARA_TEST_GT(updateTime, 0);

    // flush by cnt && one batch item contains more than 1 original event group
    PipelineEventGroup group2 = CreateEventGroup(2);
    SourceBuffer* buffer2 = group2.GetSourceBuffer().get();
    RangeCheckpoint* eoo2 = group2.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group2), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(1U, res.size());
    APSARA_TEST_EQUAL(1U, res[0].size());
    APSARA_TEST_EQUAL(3U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(2U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer1, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(buffer2, res[0][0].mSourceBuffers[1].get());
    APSARA_TEST_EQUAL(eoo1, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);

    // flush by time then by size
    res.clear();
    aggregator.mEventFlushStrategy.SetTimeoutSecs(0);
    aggregator.mEventFlushStrategy.SetMaxSizeBytes(10);
    PipelineEventGroup group3 = CreateEventGroup(1);
    SourceBuffer* buffer3 = group3.GetSourceBuffer().get();
    RangeCheckpoint* eoo3 = group3.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group3), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(0U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(2U, res.size());
    APSARA_TEST_EQUAL(1U, res[0].size());
    APSARA_TEST_EQUAL(1U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer2, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo2, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);
    APSARA_TEST_EQUAL(1U, res[1].size());
    APSARA_TEST_EQUAL(1U, res[1][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[1][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[1][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[1][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer3, res[1][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo3, res[1][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[1][0].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);
}

void AggregatorUnittest::TestAddWithGroupBatch() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 3;
    strategy.mMaxSizeBytes = 1000;
    strategy.mTimeoutSecs = 3;

    Aggregator<> aggregator;
    aggregator.Init(Json::Value(), sFlusher.get(), strategy, true);

    // add to empty batch item
    vector<BatchedEventsList> res;
    PipelineEventGroup group1 = CreateEventGroup(2);
    size_t key = group1.GetTagsHash();
    SourceBuffer* buffer1 = group1.GetSourceBuffer().get();
    RangeCheckpoint* eoo1 = group1.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group1), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(2U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(0U, res.size());
    APSARA_TEST_EQUAL(1U, TimeoutFlushManager::GetInstance()->mTimeoutRecords.size());
    APSARA_TEST_EQUAL(1U, TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].size());
    TimeoutRecord& record = TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key));
    time_t updateTime = record.mUpdateTime;
    APSARA_TEST_EQUAL(2U, record.mTimeoutSecs);
    APSARA_TEST_EQUAL(sFlusher.get(), record.mFlusher);
    APSARA_TEST_EQUAL(key, record.mKey);
    APSARA_TEST_GT(updateTime, 0);

    // flush by cnt && one batch item contains more than 1 original event group
    PipelineEventGroup group2 = CreateEventGroup(2);
    SourceBuffer* buffer2 = group2.GetSourceBuffer().get();
    RangeCheckpoint* eoo2 = group2.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group2), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(1U, res.size());
    APSARA_TEST_EQUAL(1U, res[0].size());
    APSARA_TEST_EQUAL(3U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(2U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer1, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(buffer2, res[0][0].mSourceBuffers[1].get());
    APSARA_TEST_EQUAL(eoo1, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);

    // flush by time to group batch
    res.clear();
    aggregator.mEventFlushStrategy.SetTimeoutSecs(0);
    PipelineEventGroup group3 = CreateEventGroup(1);
    SourceBuffer* buffer3 = group3.GetSourceBuffer().get();
    RangeCheckpoint* eoo3 = group3.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group3), res);
    APSARA_TEST_EQUAL(0U, res.size());
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());

    // flush by time to group batch, and then group flush by time
    aggregator.mGroupFlushStrategy->SetTimeoutSecs(0);
    PipelineEventGroup group4 = CreateEventGroup(1);
    SourceBuffer* buffer4 = group4.GetSourceBuffer().get();
    RangeCheckpoint* eoo4 = group4.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group4), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(1U, res.size());
    APSARA_TEST_EQUAL(1U, res[0].size());
    APSARA_TEST_EQUAL(1U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer2, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo2, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);

    // flush by time to group batch, and then group flush by size
    res.clear();
    aggregator.mGroupFlushStrategy->SetTimeoutSecs(3);
    aggregator.mGroupFlushStrategy->SetMaxSizeBytes(10);
    PipelineEventGroup group5 = CreateEventGroup(1);
    SourceBuffer* buffer5 = group5.GetSourceBuffer().get();
    RangeCheckpoint* eoo5 = group5.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group5), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(1U, res.size());
    APSARA_TEST_EQUAL(2U, res[0].size());
    APSARA_TEST_EQUAL(1U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer3, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo3, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);
    APSARA_TEST_EQUAL(1U, res[0][1].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][1].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][1].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0][1].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer4, res[0][1].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo4, res[0][1].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][1].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);

    // flush by size
    res.clear();
    aggregator.mEventFlushStrategy.SetMaxSizeBytes(10);
    aggregator.mEventFlushStrategy.SetTimeoutSecs(3);
    PipelineEventGroup group6 = CreateEventGroup(1);
    SourceBuffer* buffer6 = group6.GetSourceBuffer().get();
    aggregator.Add(std::move(group6), res);
    APSARA_TEST_EQUAL(1U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(0U, aggregator.mEventQueueMap[key].mBatch.mEvents.size());
    APSARA_TEST_EQUAL(1U, res.size());
    APSARA_TEST_EQUAL(1U, res[0].size());
    APSARA_TEST_EQUAL(2U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(2U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer5, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(buffer6, res[0][0].mSourceBuffers[1].get());
    APSARA_TEST_EQUAL(eoo5, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
    APSARA_TEST_GT(TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, key)).mUpdateTime,
                   updateTime - 1);
}

void AggregatorUnittest::TestFlushEventQueueWithoutGroupBatch() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 3;
    strategy.mMaxSizeBytes = 1000;
    strategy.mTimeoutSecs = 3;

    Aggregator<> aggregator;
    aggregator.Init(Json::Value(), sFlusher.get(), strategy);

    PipelineEventGroup group = CreateEventGroup(2);
    size_t key = group.GetTagsHash();
    SourceBuffer* buffer = group.GetSourceBuffer().get();
    RangeCheckpoint* eoo = group.GetExactlyOnceCheckpoint().get();
    vector<BatchedEventsList> tmp;
    aggregator.Add(std::move(group), tmp);

    BatchedEventsList res;
    // key not existed
    aggregator.FlushQueue(key + 1, res);
    APSARA_TEST_EQUAL(0U, res.size());

    // key existed
    aggregator.FlushQueue(key, res);
    APSARA_TEST_EQUAL(0U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(1U, res.size());
    APSARA_TEST_EQUAL(2U, res[0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer, res[0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo, res[0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0].mPackIdPrefix.data());
}

void AggregatorUnittest::TestFlushEventQueueWithGroupBatch() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 10;
    strategy.mMaxSizeBytes = 1000;
    strategy.mTimeoutSecs = 3;

    Aggregator<> aggregator;
    aggregator.Init(Json::Value(), sFlusher.get(), strategy, true);

    BatchedEventsList res;
    vector<BatchedEventsList> tmp;
    // flush to group item, but no actual output
    PipelineEventGroup group1 = CreateEventGroup(2);
    size_t key = group1.GetTagsHash();
    SourceBuffer* buffer1 = group1.GetSourceBuffer().get();
    RangeCheckpoint* eoo1 = group1.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group1), tmp);
    aggregator.FlushQueue(key, res);
    APSARA_TEST_EQUAL(0U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(0U, res.size());
    APSARA_TEST_EQUAL(1U, TimeoutFlushManager::GetInstance()->mTimeoutRecords.size());
    APSARA_TEST_EQUAL(2U, TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].size());
    TimeoutRecord& record = TimeoutFlushManager::GetInstance()->mTimeoutRecords["test_config"].at(make_pair(0, 0));
    time_t updateTime = record.mUpdateTime;
    APSARA_TEST_EQUAL(1U, record.mTimeoutSecs);
    APSARA_TEST_EQUAL(sFlusher.get(), record.mFlusher);
    APSARA_TEST_EQUAL(0U, record.mKey);
    APSARA_TEST_GT(updateTime, 0);
    APSARA_TEST_EQUAL(1U, aggregator.mGroupQueue->mGroups.size());

    // flush to group item, and group is flushed by time then by size
    aggregator.mGroupFlushStrategy->SetTimeoutSecs(0);
    aggregator.mGroupFlushStrategy->SetMaxSizeBytes(10);
    PipelineEventGroup group2 = CreateEventGroup(2);
    SourceBuffer* buffer2 = group2.GetSourceBuffer().get();
    RangeCheckpoint* eoo2 = group2.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group2), tmp);
    aggregator.FlushQueue(key, res);
    APSARA_TEST_EQUAL(0U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(2U, res.size());
    APSARA_TEST_EQUAL(2U, res[0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer1, res[0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo1, res[0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0].mPackIdPrefix.data());
    APSARA_TEST_EQUAL(2U, res[1].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[1].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[1].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[1].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer2, res[1].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo2, res[1].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[1].mPackIdPrefix.data());
}

void AggregatorUnittest::TestFlushGroupQueue() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 3;
    strategy.mMaxSizeBytes = 1000;
    strategy.mTimeoutSecs = 3;
    {
        // no group item
        Aggregator<> aggregator;
        aggregator.Init(Json::Value(), sFlusher.get(), strategy);
        BatchedEventsList res;
        aggregator.FlushQueue(0, res);
        APSARA_TEST_TRUE(res.empty());
    }
    {
        // with group item
        Aggregator<> aggregator;
        aggregator.Init(Json::Value(), sFlusher.get(), strategy, true);

        BatchedEventsList res;
        vector<BatchedEventsList> tmp;
        PipelineEventGroup group = CreateEventGroup(2);
        size_t key = group.GetTagsHash();
        SourceBuffer* buffer = group.GetSourceBuffer().get();
        RangeCheckpoint* eoo = group.GetExactlyOnceCheckpoint().get();
        aggregator.Add(std::move(group), tmp);
        aggregator.FlushQueue(key, res);
        APSARA_TEST_TRUE(res.empty());

        aggregator.FlushQueue(0, res);
        APSARA_TEST_EQUAL(1U, res.size());
        APSARA_TEST_EQUAL(2U, res[0].mEvents.size());
        APSARA_TEST_EQUAL(1U, res[0].mTags.mInner.size());
        APSARA_TEST_STREQ("val", res[0].mTags.mInner["key"].data());
        APSARA_TEST_EQUAL(1U, res[0].mSourceBuffers.size());
        APSARA_TEST_EQUAL(buffer, res[0].mSourceBuffers[0].get());
        APSARA_TEST_EQUAL(eoo, res[0].mExactlyOnceCheckpoint.get());
        APSARA_TEST_STREQ("pack_id", res[0].mPackIdPrefix.data());
    }
}

void AggregatorUnittest::TestFlushAllWithoutGroupBatch() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 3;
    strategy.mMaxSizeBytes = 1000;
    strategy.mTimeoutSecs = 3;

    Aggregator<> aggregator;
    aggregator.Init(Json::Value(), sFlusher.get(), strategy);
    PipelineEventGroup group = CreateEventGroup(2);
    SourceBuffer* buffer = group.GetSourceBuffer().get();
    RangeCheckpoint* eoo = group.GetExactlyOnceCheckpoint().get();
    vector<BatchedEventsList> tmp;
    aggregator.Add(std::move(group), tmp);

    vector<BatchedEventsList> res;
    aggregator.FlushAll(res);
    APSARA_TEST_EQUAL(0U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(1U, res.size());
    APSARA_TEST_EQUAL(1U, res[0].size());
    APSARA_TEST_EQUAL(2U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
}

void AggregatorUnittest::TestFlushAllWithGroupBatch() {
    DefaultFlushStrategy strategy;
    strategy.mMaxCnt = 3;
    strategy.mMaxSizeBytes = 1000;
    strategy.mTimeoutSecs = 3;

    Aggregator<> aggregator;
    aggregator.Init(Json::Value(), sFlusher.get(), strategy, true);

    BatchedEventsList tmp1;
    vector<BatchedEventsList> tmp2;
    // flush to group item, but no actual output
    PipelineEventGroup group1 = CreateEventGroup(2);
    size_t key = group1.GetTagsHash();
    SourceBuffer* buffer1 = group1.GetSourceBuffer().get();
    RangeCheckpoint* eoo1 = group1.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group1), tmp2);
    aggregator.FlushQueue(key, tmp1);
    APSARA_TEST_TRUE(tmp1.empty());
    APSARA_TEST_TRUE(tmp2.empty());

    // add another group to event batch
    PipelineEventGroup group2 = CreateEventGroup(2);
    SourceBuffer* buffer2 = group2.GetSourceBuffer().get();
    RangeCheckpoint* eoo2 = group2.GetExactlyOnceCheckpoint().get();
    aggregator.Add(std::move(group2), tmp2);
    APSARA_TEST_TRUE(tmp2.empty());

    // flush all by time then by size
    aggregator.mGroupFlushStrategy->SetTimeoutSecs(0);
    aggregator.mGroupFlushStrategy->SetMaxSizeBytes(10);
    vector<BatchedEventsList> res;
    aggregator.FlushAll(res);
    APSARA_TEST_EQUAL(0U, aggregator.mEventQueueMap.size());
    APSARA_TEST_EQUAL(2U, res.size());
    APSARA_TEST_EQUAL(1U, res[0].size());
    APSARA_TEST_EQUAL(2U, res[0][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[0][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[0][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[0][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer1, res[0][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo1, res[0][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[0][0].mPackIdPrefix.data());
    APSARA_TEST_EQUAL(1U, res[1].size());
    APSARA_TEST_EQUAL(2U, res[1][0].mEvents.size());
    APSARA_TEST_EQUAL(1U, res[1][0].mTags.mInner.size());
    APSARA_TEST_STREQ("val", res[1][0].mTags.mInner["key"].data());
    APSARA_TEST_EQUAL(1U, res[1][0].mSourceBuffers.size());
    APSARA_TEST_EQUAL(buffer2, res[1][0].mSourceBuffers[0].get());
    APSARA_TEST_EQUAL(eoo2, res[1][0].mExactlyOnceCheckpoint.get());
    APSARA_TEST_STREQ("pack_id", res[1][0].mPackIdPrefix.data());
}

PipelineEventGroup AggregatorUnittest::CreateEventGroup(size_t cnt) {
    PipelineEventGroup group(make_shared<SourceBuffer>());
    group.SetTag(string("key"), string("val"));
    StringBuffer b = group.GetSourceBuffer()->CopyString(string("pack_id"));
    group.SetMetadataNoCopy(EventGroupMetaKey::SOURCE_ID, StringView(b.data, b.size));
    group.SetExactlyOnceCheckpoint(RangeCheckpointPtr(new RangeCheckpoint));
    for (size_t i = 0; i < cnt; ++i) {
        group.AddLogEvent();
    }
    return group;
}

UNIT_TEST_CASE(AggregatorUnittest, TestParamInit)
UNIT_TEST_CASE(AggregatorUnittest, TestInitWithoutGroupBatch)
UNIT_TEST_CASE(AggregatorUnittest, TestInitWithGroupBatch)
UNIT_TEST_CASE(AggregatorUnittest, TestAddWithoutGroupBatch)
UNIT_TEST_CASE(AggregatorUnittest, TestAddWithGroupBatch)
UNIT_TEST_CASE(AggregatorUnittest, TestFlushEventQueueWithoutGroupBatch)
UNIT_TEST_CASE(AggregatorUnittest, TestFlushEventQueueWithGroupBatch)
UNIT_TEST_CASE(AggregatorUnittest, TestFlushGroupQueue)
UNIT_TEST_CASE(AggregatorUnittest, TestFlushAllWithoutGroupBatch)
UNIT_TEST_CASE(AggregatorUnittest, TestFlushAllWithGroupBatch)

} // namespace logtail

UNIT_TEST_MAIN
