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

#include <string>

#include "prometheus/Labels.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {
class LabelsUnittest : public testing::Test {
public:
    void TestGet();
    void TestPush();
    void TestRange();

private:
};

class LabelsBuilderUnittest : public testing::Test {
public:
    void TestReset();
    void TestDeleteLabel();
    void TestSet();
    void TestGet();
    void TestLabels();
};

void LabelsUnittest::TestGet() {
    Labels labels;
    labels.Push(Label{"host", "172.17.0.3:9100"});

    // 不存在返回空值
    APSARA_TEST_EQUAL("", labels.Get("hosts"));

    // 存在返回value
    APSARA_TEST_EQUAL("172.17.0.3:9100", labels.Get("host"));
}

void LabelsUnittest::TestPush() {
    Labels labels;

    labels.Push(Label{"host", "172.17.0.3:9100"});

    // 存在Label{"host", "172.17.0.3:9100"}
    APSARA_TEST_EQUAL("172.17.0.3:9100", labels.Get("host"));
}

void LabelsUnittest::TestRange() {
    Labels labels;
    map<string, string> testMap;
    testMap["host"] = "172.17.0.3:9100";
    testMap["ip"] = "172.17.0.3";
    testMap["port"] = "9100";

    labels.Push(Label{"host", "172.17.0.3:9100"});
    labels.Push(Label{"ip", "172.17.0.3"});
    labels.Push(Label{"port", "9100"});

    map<string, string> resMap;
    labels.Range([&resMap](Label l) { resMap[l.name] = l.value; });

    APSARA_TEST_EQUAL(testMap, resMap);
}


void LabelsBuilderUnittest::TestReset() {
    LabelsBuilder lb;
    Labels labels;
    labels.Push(Label{"host", "172.17.0.3:9100"});
    lb.Reset(labels);
    APSARA_TEST_EQUAL("172.17.0.3:9100", lb.mBase.Get("host"));
}

void LabelsBuilderUnittest::TestDeleteLabel() {
    LabelsBuilder lb;
    Labels labels;

    vector<string> nameList{"host"};
    lb.DeleteLabel(nameList);

    labels.Push(Label{"host", "172.17.0.3:9100"});
    lb.Reset(labels);
    APSARA_TEST_EQUAL("", lb.labels().Get("host"));
}

void LabelsBuilderUnittest::TestSet() {
    LabelsBuilder lb;
    Labels labels;
    labels.Push(Label{"host", "172.17.0.3:9100"});
    lb.Reset(labels);
    APSARA_TEST_EQUAL("172.17.0.3:9100", lb.Get("host"));

    // 根据key修改value
    lb.Set("host", "172.17.0.3:9300");
    APSARA_TEST_EQUAL("172.17.0.3:9300", lb.Get("host"));

    // 如果VALUE为空字符串则删除
    lb.Set("host", "");
    APSARA_TEST_EQUAL("", lb.Get("host"));
}

void LabelsBuilderUnittest::TestGet() {
    LabelsBuilder lb;
    Labels labels;
    labels.Push(Label{"host", "172.17.0.3:9100"});
    lb.Reset(labels);
    APSARA_TEST_EQUAL("172.17.0.3:9100", lb.Get("host"));
}

void LabelsBuilderUnittest::TestLabels() {
    LabelsBuilder lb;
    Labels labels;
    labels.Push(Label{"host", "172.17.0.3:9100"});
    lb.Reset(labels);

    vector<string> nameList{"host"};
    lb.DeleteLabel(nameList);


    APSARA_TEST_EQUAL("", lb.labels().Get("host"));
}

UNIT_TEST_CASE(LabelsUnittest, TestGet)
UNIT_TEST_CASE(LabelsUnittest, TestPush)
UNIT_TEST_CASE(LabelsUnittest, TestRange)

UNIT_TEST_CASE(LabelsBuilderUnittest, TestReset)
UNIT_TEST_CASE(LabelsBuilderUnittest, TestDeleteLabel)
UNIT_TEST_CASE(LabelsBuilderUnittest, TestSet)
UNIT_TEST_CASE(LabelsBuilderUnittest, TestGet)
UNIT_TEST_CASE(LabelsBuilderUnittest, TestLabels)

} // namespace logtail

UNIT_TEST_MAIN