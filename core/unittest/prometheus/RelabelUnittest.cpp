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

#include <json/json.h>

#include <boost/regex.hpp>
#include <string>

#include "common/JsonUtil.h"
#include "prometheus/Relabel.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class ActionConverterUnittest : public testing::Test {
public:
    void TestStringToAction();
    void TestActionToString();
};

class RelabelConfigUnittest : public testing::Test {
public:
    void TestRelabelConfig();
    void TestProcess();
};


void ActionConverterUnittest::TestStringToAction() {
    // Arrange
    std::string actionString = "replace";
    Action expectedAction = Action::replace;

    // Act
    Action result = StringToAction(actionString);

    // Assert
    APSARA_TEST_EQUAL(result, expectedAction);
}

void ActionConverterUnittest::TestActionToString() {
    // Arrange
    Action action = Action::replace;
    std::string expectedString = "replace";

    // Act
    std::string result = ActionToString(action);

    // Assert
    APSARA_TEST_EQUAL(result, expectedString);
}

void RelabelConfigUnittest::TestRelabelConfig() {
    Json::Value configJson;
    string configStr, errorMsg;

    configStr = R"(
        {
                "action": "keep",
                "regex": "node-exporter",
                "replacement": "$1",
                "separator": ";",
                "source_labels": [
                    "__meta_kubernetes_pod_label_app"
                ],
                "target_label": "__address__",
                "modulus": 222
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));

    RelabelConfig config = RelabelConfig(configJson);

    APSARA_TEST_EQUAL(Action::keep, config.action);
    // APSARA_TEST_EQUAL("node-exporter", config.regex.get_data());
    APSARA_TEST_EQUAL("$1", config.replacement);
    APSARA_TEST_EQUAL(";", config.separator);
    APSARA_TEST_EQUAL((size_t)1, config.sourceLabels.size());
    APSARA_TEST_EQUAL("__meta_kubernetes_pod_label_app", config.sourceLabels[0]);
    APSARA_TEST_EQUAL("__address__", config.targetLabel);
    APSARA_TEST_EQUAL((uint64_t)222, config.modulus);
}

void RelabelConfigUnittest::TestProcess() {
    Json::Value configJson;
    string configStr, errorMsg;
    Labels labels;
    labels.push_back(Label{"__meta_kubernetes_pod_ip", "172.17.0.3"});
    labels.push_back(Label{"__meta_kubernetes_pod_label_app", "node-exporter"});
    vector<RelabelConfig> cfgs;

    // single relabel replace
    configStr = configStr + R"(
        {
                "action": "replace",
                "regex": "(.*)"
        + ")\",\n" +
        R"(
                "replacement": "${1}:9100",
                "separator": ";",
                "source_labels": [
                    "__meta_kubernetes_pod_ip"
                ],
                "target_label": "__address__"
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    RelabelConfig config = RelabelConfig(configJson);
    cfgs.push_back(config);

    Labels result;
    relabel::Process(labels, cfgs, result);

    APSARA_TEST_EQUAL((size_t)3, result.size());
    APSARA_TEST_EQUAL("172.17.0.3:9100", result.Get("__address__"));
    APSARA_TEST_EQUAL("node-exporter", result.Get("__meta_kubernetes_pod_label_app"));
    APSARA_TEST_EQUAL("172.17.0.3", result.Get("__meta_kubernetes_pod_ip"));

    // single relabel keep
    configStr = R"(
        {
                "action": "keep",
                "regex": "172.*",
                "separator": ";",
                "source_labels": [
                    "__meta_kubernetes_pod_ip"
                ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    config = RelabelConfig(configJson);
    cfgs.clear();
    cfgs.push_back(config);
    relabel::Process(labels, cfgs, result);

    APSARA_TEST_EQUAL((size_t)2, result.size());
    APSARA_TEST_EQUAL("172.17.0.3", result.Get("__meta_kubernetes_pod_ip"));

    // single relabel drop
    configStr = R"(
        {
                "action": "drop",
                "regex": "172.*",
                "source_labels": [
                    "__meta_kubernetes_pod_ip"
                ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    config = RelabelConfig(configJson);
    cfgs.clear();
    cfgs.push_back(config);
    relabel::Process(labels, cfgs, result);
    APSARA_TEST_EQUAL((size_t)0, result.size());
    APSARA_TEST_EQUAL("", result.Get("__meta_kubernetes_pod_label_app"));

    // multi relabel
    string configStr1, configStr2;
    configStr1 = configStr1 + R"(
        {
                "action": "replace",
                "regex": "(.*)"
        + ")\",\n" +
        R"(
                "replacement": "${1}:9100",
                "separator": ";",
                "source_labels": [
                    "__meta_kubernetes_pod_ip"
                ],
                "target_label": "__address__"
        }
    )";
    configStr2 = R"(
        {
                "action": "drop",
                "regex": "172.*",
                "separator": ";",
                "source_labels": [
                    "__address__"
                ]
        }
    )";

    APSARA_TEST_TRUE(ParseJsonTable(configStr1, configJson, errorMsg));
    config = RelabelConfig(configJson);
    cfgs.clear();
    cfgs.push_back(config);
    APSARA_TEST_TRUE(ParseJsonTable(configStr2, configJson, errorMsg));
    config = RelabelConfig(configJson);
    cfgs.push_back(config);

    relabel::Process(labels, cfgs, result);
    APSARA_TEST_EQUAL((size_t)0, result.size());
    APSARA_TEST_EQUAL("", result.Get("__address__"));
}

UNIT_TEST_CASE(ActionConverterUnittest, TestStringToAction)
UNIT_TEST_CASE(ActionConverterUnittest, TestActionToString)

UNIT_TEST_CASE(RelabelConfigUnittest, TestRelabelConfig)
UNIT_TEST_CASE(RelabelConfigUnittest, TestProcess)

} // namespace logtail

UNIT_TEST_MAIN
