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

#include "common/JsonUtil.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class UntypedSingleValueUnittest : public ::testing::Test {
public:
    void TestToJson();
    void TestFromJson();
};

void UntypedSingleValueUnittest::TestToJson() {
    UntypedSingleValue value{10.0};
    Json::Value res = value.ToJson();

    Json::Value valueJson = Json::Value(10.0);

    APSARA_TEST_TRUE(valueJson == res);
}

void UntypedSingleValueUnittest::TestFromJson() {
    UntypedSingleValue value;
    value.FromJson(Json::Value(10.0));

    APSARA_TEST_EQUAL(10.0, value.mValue);
}

UNIT_TEST_CASE(UntypedSingleValueUnittest, TestToJson)
UNIT_TEST_CASE(UntypedSingleValueUnittest, TestFromJson)

class UntypedMultiFloatValuesUnittest : public ::testing::Test {
public:
    void TestToJson();
    void TestFromJson();

protected:
    void SetUp() override {
        mSourceBuffer.reset(new SourceBuffer);
        mEventGroup.reset(new PipelineEventGroup(mSourceBuffer));
        mMetricEvent = mEventGroup->CreateMetricEvent();
    }

private:
    shared_ptr<SourceBuffer> mSourceBuffer;
    unique_ptr<PipelineEventGroup> mEventGroup;
    unique_ptr<MetricEvent> mMetricEvent;
};

void UntypedMultiFloatValuesUnittest::TestToJson() {
    UntypedMultiFloatValues value(mMetricEvent.get());
    value.SetMultiKeyValue(string("test-1"), 10.0);
    value.SetMultiKeyValue(string("test-2"), 2.0);
    Json::Value res = value.ToJson();

    Json::Value valueJson;
    valueJson["test-1"] = 10.0;
    valueJson["test-2"] = 2.0;

    APSARA_TEST_TRUE(valueJson == res);
}

void UntypedMultiFloatValuesUnittest::TestFromJson() {
    UntypedMultiFloatValues value(mMetricEvent.get());
    Json::Value valueJson;
    valueJson["test-1"] = 10.0;
    valueJson["test-2"] = 2.0;
    value.FromJson(valueJson);

    APSARA_TEST_EQUAL(10.0, value.GetMultiKeyValue("test-1"));
    APSARA_TEST_EQUAL(2.0, value.GetMultiKeyValue("test-2"));
}

UNIT_TEST_CASE(UntypedMultiFloatValuesUnittest, TestToJson)
UNIT_TEST_CASE(UntypedMultiFloatValuesUnittest, TestFromJson)

} // namespace logtail

UNIT_TEST_MAIN
