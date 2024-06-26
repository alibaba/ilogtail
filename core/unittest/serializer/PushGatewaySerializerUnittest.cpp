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

#include "unittest/Unittest.h"
#include "serializer/PushGatewaySerializer.h"

using namespace std;

namespace logtail {

class PushGatewaySerializerUnittest : public ::testing::Test {
public:
    void TestSerialize();
};

void PushGatewaySerializerUnittest::TestSerialize() {
    auto serializer = new PushGatewaySerializer();
    
    const auto &srcBuf = make_shared<SourceBuffer>();
    auto eGroup = PipelineEventGroup(srcBuf);
    auto ev1 = eGroup.AddMetricEvent();
    ev1->SetName("test_metric1");
    ev1->SetValue(MetricValue(UntypedSingleValue{ 1.0 }));
    ev1->SetTimestamp(1234567890);
    ev1->SetTag(StringView("key11"), StringView("val11"));
    ev1->SetTag(StringView("key12"), StringView("val12"));

    auto ev2 = eGroup.AddMetricEvent();
    ev2->SetName("test_metric2");
    ev2->SetValue(MetricValue(UntypedSingleValue{ 2.0 }));
    ev2->SetTimestamp(1234567891);
    ev2->SetTag(StringView("key21"), StringView("val21"));
    ev2->SetTag(StringView("key22"), StringView("val22"));

    auto batchedEvents = BatchedEvents();
    batchedEvents.mEvents = std::move(eGroup.MutableEvents());

    string res, errorMsg;
    serializer->Serialize(std::move(batchedEvents), res, errorMsg);

    APSARA_TEST_STREQ("", errorMsg.data());
    APSARA_TEST_STREQ("test_metric1{key11=\"val11\",key12=\"val12\",} 1 1234567890\ntest_metric2{key21=\"val21\",key22=\"val22\",} 2 1234567891\n", res.data());
}

UNIT_TEST_CASE(PushGatewaySerializerUnittest, TestSerialize)

} // namespace logtail

UNIT_TEST_MAIN
