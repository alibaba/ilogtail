/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/Constants.h"
#include "common/JsonUtil.h"
#include "config/Config.h"
#include "processor/inner/ProcessorRelabelMetricNative.h"
#include "prometheus/TextParser.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {
class ProcessorRelabelMetricNativeUnittest : public testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }

    void TestInit();
    void TestProcess();

    PipelineContext mContext;
};

void ProcessorRelabelMetricNativeUnittest::TestInit() {
    // make config
    Json::Value config;
    config["test"] = "test";

    ProcessorRelabelMetricNative processor;
    processor.SetContext(mContext);

    // fatal test
    APSARA_TEST_FALSE(processor.Init(config));

    string configStr, errorMsg;
    configStr = configStr + R"(
        {
            "metric_relabel_configs": [
                {
                    "action": "keep",
                    "regex": "node-exporter",
                    "replacement": "$1",
                    "separator": ";",
                    "source_labels": [
                        "__meta_kubernetes_pod_label_app"
                    ]
                },
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
            ]
        }
    )";

    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));
    APSARA_TEST_TRUE(processor.Init(config));
}

void ProcessorRelabelMetricNativeUnittest::TestProcess() {
    // make config
    Json::Value config;

    ProcessorRelabelMetricNative processor;
    processor.SetContext(mContext);

    string configStr, errorMsg;
    configStr = configStr + R"(
        {
            "metric_relabel_configs": [
                {
                    "action": "drop",
                    "regex": "v.*",
                    "replacement": "$1",
                    "separator": ";",
                    "source_labels": [
                        "k3"
                    ]
                },
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
            ]
        }
    )";

    // init
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));
    APSARA_TEST_TRUE(processor.Init(config));

    // make events

    const auto& srcBuf = make_shared<SourceBuffer>();
    auto parser = TextParser(srcBuf);
    APSARA_TEST_TRUE(parser.Ok());
    auto eventGroup = parser.Parse(R"""(
# begin

test_metric1{k1="v1", k2="v2"} 1.0
  test_metric2{k1="v1", k2="v2"} 2.0 1234567890
test_metric3{k1="v1",k2="v2"} 9.9410452992e+10
  test_metric4{k1="v1",k2="v2"} 9.9410452992e+10 1715829785083
  test_metric5{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083
test_metric6{k1="v1",k2="v2",} 9.9410452992e+10 1715829785083
test_metric7{k1="v1",k3="2", } 9.9410452992e+10 1715829785083  
test_metric8{k1="v1", k3="v2", } 9.9410452992e+10 1715829785083

# end
    )""");

    // run function
    std::string pluginId = "testID";
    APSARA_TEST_EQUAL((size_t)8, eventGroup.GetEvents().size());
    processor.Process(eventGroup);

    // judge result
    APSARA_TEST_EQUAL((size_t)7, eventGroup.GetEvents().size());
}

UNIT_TEST_CASE(ProcessorRelabelMetricNativeUnittest, TestInit)
UNIT_TEST_CASE(ProcessorRelabelMetricNativeUnittest, TestProcess)

} // namespace logtail

UNIT_TEST_MAIN