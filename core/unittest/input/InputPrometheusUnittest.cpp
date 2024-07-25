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

#include <json/json.h>

#include <cstdint>
#include <memory>
#include <string>

#include "PluginRegistry.h"
#include "Relabel.h"
#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "inner/ProcessorRelabelMetricNative.h"
#include "input/InputPrometheus.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "prometheus/PrometheusInputRunner.h"
#include "unittest/Unittest.h"


using namespace std;
namespace logtail {

// MockHttpClient
class MockHttpClient : public sdk::HTTPClient {
public:
    MockHttpClient();

    virtual void Send(const std::string& httpMethod,
                      const std::string& host,
                      const int32_t port,
                      const std::string& url,
                      const std::string& queryString,
                      const std::map<std::string, std::string>& header,
                      const std::string& body,
                      const int32_t timeout,
                      sdk::HttpMessage& httpMessage,
                      const std::string& intf,
                      const bool httpsFlag);
    virtual void AsynSend(sdk::AsynRequest* request);
};

MockHttpClient::MockHttpClient() {
}

void MockHttpClient::Send(const std::string& httpMethod,
                          const std::string& host,
                          const int32_t port,
                          const std::string& url,
                          const std::string& queryString,
                          const std::map<std::string, std::string>& header,
                          const std::string& body,
                          const int32_t timeout,
                          sdk::HttpMessage& httpMessage,
                          const std::string& intf,
                          const bool httpsFlag) {
    httpMessage.content
        = "# HELP go_gc_duration_seconds A summary of the pause duration of garbage collection cycles.\n"
          "# TYPE go_gc_duration_seconds summary\n"
          "go_gc_duration_seconds{quantile=\"0\"} 1.5531e-05\n"
          "go_gc_duration_seconds{quantile=\"0.25\"} 3.9357e-05\n"
          "go_gc_duration_seconds{quantile=\"0.5\"} 4.1114e-05\n"
          "go_gc_duration_seconds{quantile=\"0.75\"} 4.3372e-05\n"
          "go_gc_duration_seconds{quantile=\"1\"} 0.000112326\n"
          "go_gc_duration_seconds_sum 0.034885631\n"
          "go_gc_duration_seconds_count 850\n"
          "# HELP go_goroutines Number of goroutines that currently exist.\n"
          "# TYPE go_goroutines gauge\n"
          "go_goroutines 7\n"
          "# HELP go_info Information about the Go environment.\n"
          "# TYPE go_info gauge\n"
          "go_info{version=\"go1.22.3\"} 1\n"
          "# HELP go_memstats_alloc_bytes Number of bytes allocated and still in use.\n"
          "# TYPE go_memstats_alloc_bytes gauge\n"
          "go_memstats_alloc_bytes 6.742688e+06\n"
          "# HELP go_memstats_alloc_bytes_total Total number of bytes allocated, even if freed.\n"
          "# TYPE go_memstats_alloc_bytes_total counter\n"
          "go_memstats_alloc_bytes_total 1.5159292e+08";
    httpMessage.statusCode = 200;
}

void MockHttpClient::AsynSend(sdk::AsynRequest* request) {
}
class InputPrometheusUnittest : public testing::Test {
public:
    void OnSuccessfulInit();
    void OnFailedInit();
    void OnPipelineUpdate();
    void TestCreateInnerProcessor();

protected:
    static void SetUpTestCase() {
        setenv("POD_NAME", "matrix-test", 1);
        setenv("OPERATOR_HOST", "127.0.0.1", 1);
        setenv("OPERATOR_PORT", "12345", 1);

        AppConfig::GetInstance()->mPurageContainerMode = true;
        PluginRegistry::GetInstance()->LoadPlugins();
    }
    void SetUp() override {
        p.mName = "test_config";
        ctx.SetConfigName("test_config");
        ctx.SetPipeline(p);
    }
    static void TearDownTestCase() {
        unsetenv("POD_NAME");
        unsetenv("OPERATOR_HOST");
        unsetenv("OPERATOR_PORT");
        PluginRegistry::GetInstance()->UnloadPlugins();
    }

private:
    Pipeline p;
    PipelineContext ctx;
};

void InputPrometheusUnittest::OnSuccessfulInit() {
    unique_ptr<InputPrometheus> input;
    Json::Value configJson, optionalGoPipeline;
    uint32_t pluginIndex = 0;
    string configStr, errorMsg;
    // only mandatory param
    configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "_arms-prom/node-exporter/0",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": "15s",
                "scrape_timeout": "15s",
                "scrape_targets": [
                    {
                        "host": "172.17.0.3:9100",
                    }
                ]
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputPrometheus());
    input->SetContext(ctx);
    input->SetMetricsRecordRef(InputPrometheus::sName, "1");
    APSARA_TEST_TRUE(input->Init(configJson, pluginIndex, optionalGoPipeline));

    input->mScrapeJobPtr->mClient.reset(new MockHttpClient());
    APSARA_TEST_EQUAL("_arms-prom/node-exporter/0", input->mScrapeJobPtr->mJobName);
    APSARA_TEST_EQUAL("/metrics", input->mScrapeJobPtr->mScrapeConfigPtr->mMetricsPath);
    APSARA_TEST_EQUAL(15LL, input->mScrapeJobPtr->mScrapeConfigPtr->mScrapeIntervalSeconds);
    APSARA_TEST_EQUAL(15LL, input->mScrapeJobPtr->mScrapeConfigPtr->mScrapeTimeoutSeconds);
    APSARA_TEST_EQUAL(-1, input->mScrapeJobPtr->mScrapeConfigPtr->mMaxScrapeSizeBytes);
    APSARA_TEST_EQUAL(-1, input->mScrapeJobPtr->mScrapeConfigPtr->mSampleLimit);
    APSARA_TEST_EQUAL(-1, input->mScrapeJobPtr->mScrapeConfigPtr->mSeriesLimit);

    // all useful config
    configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "_arms-prom/node-exporter/0",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": "15s",
                "scrape_timeout": "15s",
                "scrape_targets": [
                    {
                        "host": "172.17.0.3:9100",
                    }
                ],
                "max_scrape_size": "10MiB",
                "sample_limit": 1000000,
                "series_limit": 1000000
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputPrometheus());
    input->SetContext(ctx);
    input->SetMetricsRecordRef(InputPrometheus::sName, "1");
    APSARA_TEST_TRUE(input->Init(configJson, pluginIndex, optionalGoPipeline));

    input->mScrapeJobPtr->mClient.reset(new MockHttpClient());
    APSARA_TEST_EQUAL("_arms-prom/node-exporter/0", input->mScrapeJobPtr->mJobName);
    APSARA_TEST_EQUAL("/metrics", input->mScrapeJobPtr->mScrapeConfigPtr->mMetricsPath);
    APSARA_TEST_EQUAL(15, input->mScrapeJobPtr->mScrapeConfigPtr->mScrapeIntervalSeconds);
    APSARA_TEST_EQUAL(15, input->mScrapeJobPtr->mScrapeConfigPtr->mScrapeTimeoutSeconds);
    APSARA_TEST_EQUAL(10 * 1024 * 1024, input->mScrapeJobPtr->mScrapeConfigPtr->mMaxScrapeSizeBytes);
    APSARA_TEST_EQUAL(1000000, input->mScrapeJobPtr->mScrapeConfigPtr->mSampleLimit);
    APSARA_TEST_EQUAL(1000000, input->mScrapeJobPtr->mScrapeConfigPtr->mSeriesLimit);
}

void InputPrometheusUnittest::OnFailedInit() {
    unique_ptr<InputPrometheus> input;
    Json::Value configJson, optionalGoPipeline;
    uint32_t pluginIndex = 0;
    string configStr, errorMsg;
    // only mandatory param
    configStr = R"(
        {
            "Type": "input_prometheus",
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputPrometheus());
    input->SetContext(ctx);
    input->SetMetricsRecordRef(InputPrometheus::sName, "1");
    APSARA_TEST_FALSE(input->Init(configJson, pluginIndex, optionalGoPipeline));

    // with invalid ScrapeConfig
    configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": "15s",
                "scrape_timeout": "15s",
                "scrape_targets": [
                    {
                        "host": "172.17.0.3:9100",
                    }
                ]
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputPrometheus());
    input->SetContext(ctx);
    input->SetMetricsRecordRef(InputPrometheus::sName, "1");
    APSARA_TEST_FALSE(input->Init(configJson, pluginIndex, optionalGoPipeline));
}

void InputPrometheusUnittest::OnPipelineUpdate() {
    unique_ptr<InputPrometheus> input;
    Json::Value configJson, optionalGoPipeline;
    uint32_t pluginIndex = 0;
    string configStr, errorMsg;
    configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "_arms-prom/node-exporter/0",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": "15s",
                "scrape_timeout": "15s",
                "scrape_targets": [
                    {
                        "host": "172.17.0.3:9100",
                    }
                ]
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputPrometheus());
    input->SetContext(ctx);
    input->SetMetricsRecordRef(InputPrometheus::sName, "1");
    APSARA_TEST_TRUE(input->Init(configJson, pluginIndex, optionalGoPipeline));
    input->mScrapeJobPtr->mClient.reset(new MockHttpClient());

    APSARA_TEST_TRUE(input->Start());
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("test_config")
                     != PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());

    APSARA_TEST_TRUE(input->Stop(true));
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("test_config")
                     == PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());

    PrometheusInputRunner::GetInstance()->Stop();
}

void InputPrometheusUnittest::TestCreateInnerProcessor() {
    unique_ptr<InputPrometheus> input;
    Json::Value configJson, optionalGoPipeline;
    uint32_t pluginIndex = 0;
    string configStr, errorMsg;
    {
        // only mandatory param
        configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "_arms-prom/node-exporter/0",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": "15s",
                "scrape_timeout": "15s",
                "scrape_targets": [
                    {
                        "host": "172.17.0.3:9100",
                    }
                ]
            }
        }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        input.reset(new InputPrometheus());
        input->SetContext(ctx);
        input->SetMetricsRecordRef(InputPrometheus::sName, "1");

        APSARA_TEST_TRUE(input->Init(configJson, pluginIndex, optionalGoPipeline));

        input->mScrapeJobPtr->mClient.reset(new MockHttpClient());
        APSARA_TEST_EQUAL(1U, input->mInnerProcessors.size());
        APSARA_TEST_EQUAL(ProcessorRelabelMetricNative::sName, input->mInnerProcessors[0]->Name());
        APSARA_TEST_EQUAL(0U,
                          dynamic_cast<ProcessorRelabelMetricNative*>(input->mInnerProcessors[0]->mPlugin.get())
                              ->mRelabelConfigs.size());
    }
    {
        // with metric relabel config
        configStr = R"JSON(
            {
                "Type": "input_prometheus",
                "ScrapeConfig": {
                    "enable_http2": true,
                    "follow_redirects": true,
                    "honor_timestamps": false,
                    "job_name": "_kube-state-metrics",
                    "kubernetes_sd_configs": [
                        {
                            "enable_http2": true,
                            "follow_redirects": true,
                            "kubeconfig_file": "",
                            "namespaces": {
                                "names": [
                                    "arms-prom"
                                ],
                                "own_namespace": false
                            },
                            "role": "pod"
                        }
                    ],
                    "metrics_path": "/metrics",
                    "relabel_configs": [
                        {
                            "action": "keep",
                            "regex": "kube-state-metrics",
                            "replacement": "$1",
                            "separator": ";",
                            "source_labels": [
                                "__meta_kubernetes_pod_label_k8s_app"
                            ]
                        },
                        {
                            "action": "keep",
                            "regex": "8080",
                            "replacement": "$1",
                            "separator": ";",
                            "source_labels": [
                                "__meta_kubernetes_pod_container_port_number"
                            ]
                        },
                        {
                            "action": "replace",
                            "regex": "([^:]+)(?::\\d+)?;(\\d+)",
                            "replacement": "$1:$2",
                            "separator": ";",
                            "source_labels": [
                                "__address__",
                                "__meta_kubernetes_pod_container_port_number"
                            ],
                            "target_label": "__address__"
                        }
                    ],
                    "metric_relabel_configs": [
                        {
                            "action": "keep",
                            "regex": "kube-state-metrics",
                            "replacement": "$1",
                            "separator": ";",
                            "source_labels": [
                                "__meta_kubernetes_pod_label_k8s_app"
                            ]
                        },
                        {
                            "action": "keep",
                            "regex": "8080",
                            "replacement": "$1",
                            "separator": ";",
                            "source_labels": [
                                "__meta_kubernetes_pod_container_port_number"
                            ]
                        },
                        {
                            "action": "replace",
                            "regex": "([^:]+)(?::\\d+)?;(\\d+)",
                            "replacement": "$1:$2",
                            "separator": ";",
                            "source_labels": [
                                "__address__",
                                "__meta_kubernetes_pod_container_port_number"
                            ],
                            "target_label": "__address__"
                        }
                    ],
                    "scheme": "http",
                    "scrape_interval": "3s",
                    "scrape_timeout": "3s"
                }
            }
        )JSON";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        input.reset(new InputPrometheus());
        input->SetContext(ctx);
        input->SetMetricsRecordRef(InputPrometheus::sName, "1");

        APSARA_TEST_TRUE(input->Init(configJson, pluginIndex, optionalGoPipeline));

        input->mScrapeJobPtr->mClient.reset(new MockHttpClient());
        APSARA_TEST_EQUAL(1U, input->mInnerProcessors.size());
        APSARA_TEST_EQUAL(ProcessorRelabelMetricNative::sName, input->mInnerProcessors[0]->Name());
        APSARA_TEST_EQUAL(ProcessorRelabelMetricNative::sName, input->mInnerProcessors[0]->mPlugin->Name());
        APSARA_TEST_EQUAL(3U,
                          dynamic_cast<ProcessorRelabelMetricNative*>(input->mInnerProcessors[0]->mPlugin.get())
                              ->mRelabelConfigs.size());
        APSARA_TEST_EQUAL(Action::KEEP,
                          dynamic_cast<ProcessorRelabelMetricNative*>(input->mInnerProcessors[0]->mPlugin.get())
                              ->mRelabelConfigs[0]
                              .mAction);
        APSARA_TEST_EQUAL(Action::KEEP,
                          dynamic_cast<ProcessorRelabelMetricNative*>(input->mInnerProcessors[0]->mPlugin.get())
                              ->mRelabelConfigs[1]
                              .mAction);
        APSARA_TEST_EQUAL(Action::REPLACE,
                          dynamic_cast<ProcessorRelabelMetricNative*>(input->mInnerProcessors[0]->mPlugin.get())
                              ->mRelabelConfigs[2]
                              .mAction);
    }
}

UNIT_TEST_CASE(InputPrometheusUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputPrometheusUnittest, OnFailedInit)
UNIT_TEST_CASE(InputPrometheusUnittest, OnPipelineUpdate)
UNIT_TEST_CASE(InputPrometheusUnittest, TestCreateInnerProcessor)

} // namespace logtail

UNIT_TEST_MAIN