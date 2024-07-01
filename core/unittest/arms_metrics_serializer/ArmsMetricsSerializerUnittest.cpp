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

#include <snappy/snappy.h>

#include "flusher/FlusherArmsMetrics.h"
#include "serializer/ArmsSerializer.h"
#include "unittest/Unittest.h"

DECLARE_FLAG_INT32(max_send_log_group_size);

using namespace std;

namespace logtail {

class ArmsMetricsSerializerUnittest : public ::testing::Test {
public:
    void TestSerializeEventGroupList();

protected:
    static void SetUpTestCase() { armsFlusher = make_unique<FlusherArmsMetrics>(); }

    void SetUp() override {}

private:
    BatchedEvents CreateBatchedEvents(bool enableNanosecond);

    static unique_ptr<FlusherArmsMetrics> armsFlusher;

    PipelineContext mCtx;
};

unique_ptr<FlusherArmsMetrics> ArmsMetricsSerializerUnittest::armsFlusher;


void ArmsMetricsSerializerUnittest::TestSerializeEventGroupList() {
    std::cout << "start test ArmsMetricsSerializerUnittest" << std::endl;
    std::vector<BatchedEventsList> vec(1);
    std::vector<BatchedEvents> batchedEventList(10);
    for (int i = 0; i < 10; i++) {
        auto events = CreateBatchedEvents(true);
        batchedEventList.emplace_back(std::move(events));
    }
    vec.emplace_back(std::move(batchedEventList));
}

BatchedEvents ArmsMetricsSerializerUnittest::CreateBatchedEvents(bool enableNanosecond) {
    PipelineEventGroup group(make_shared<SourceBuffer>());
    group.SetTag(std::string("appName"), std::string("cmonitor-test"));
    group.SetTag(std::string("clusterId"), std::string("clusterId-test"));
    group.SetTag(std::string("workloadName"), std::string("cmonitor-test"));
    group.SetTag(std::string("workloadKind"), std::string("deployment"));
    group.SetTag(std::string("appId"), std::string("xxxxdsgejosldie"));
    group.SetTag(std::string("source_ip"), std::string("192.168.88.11"));
    StringBuffer b = group.GetSourceBuffer()->CopyString(string("pack_id"));
    group.SetMetadataNoCopy(EventGroupMetaKey::SOURCE_ID, StringView(b.data, b.size));
    group.SetExactlyOnceCheckpoint(RangeCheckpointPtr(new RangeCheckpoint));
    LogEvent* e = group.AddLogEvent();
    e->SetContent(std::string("key"), std::string("value"));
    if (enableNanosecond) {
        e->SetTimestamp(1234567890, 1);
    } else {
        e->SetTimestamp(1234567890);
    }
    BatchedEvents batch(std::move(group.MutableEvents()),
                        std::move(group.GetSizedTags()),
                        std::move(group.GetSourceBuffer()),
                        group.GetMetadata(EventGroupMetaKey::SOURCE_ID),
                        std::move(group.GetExactlyOnceCheckpoint()));

    return batch;
}

UNIT_TEST_CASE(ArmsMetricsSerializerUnittest, TestSerializeEventGroupList)

} // namespace logtail

UNIT_TEST_MAIN
