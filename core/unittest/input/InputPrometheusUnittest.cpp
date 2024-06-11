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

#include <filesystem>
#include <memory>
#include <string>

#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "input/InputPrometheus.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "prometheus/PrometheusInputRunner.h"
#include "queue/ProcessQueueManager.h"
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
    printf("%s %s %d %s %s\n", httpMethod.c_str(), host.c_str(), port, url.c_str(), queryString.c_str());
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
    void OnPipelineUpdate();
    void TestScrapeData();

protected:
    static void SetUpTestCase() { AppConfig::GetInstance()->mPurageContainerMode = true; }
    void SetUp() override {
        p.mName = "test_config";
        ctx.SetConfigName("test_config");
        ctx.SetPipeline(p);
    }

private:
    Pipeline p;
    PipelineContext ctx;
};

void InputPrometheusUnittest::OnSuccessfulInit() {
    unique_ptr<InputPrometheus> input;
    Json::Value configJson, optionalGoPipeline;
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
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));

    // with scrape job
    configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "_arms-prom/node-exporter/0",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": 15,
                "scrape_timeout": 15,
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
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));
    APSARA_TEST_EQUAL("_arms-prom/node-exporter/0", input->scrapeJob.jobName);
    APSARA_TEST_EQUAL("/metrics", input->scrapeJob.metricsPath);
    APSARA_TEST_EQUAL(15, input->scrapeJob.scrapeInterval);
    APSARA_TEST_EQUAL(15, input->scrapeJob.scrapeTimeout);
    APSARA_TEST_EQUAL(9100, input->scrapeJob.scrapeTargets[0].port);

    PrometheusInputRunner::GetInstance()->Stop();
}

void InputPrometheusUnittest::OnPipelineUpdate() {
    unique_ptr<InputPrometheus> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "_arms-prom/node-exporter/0",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": 15,
                "scrape_timeout": 15,
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
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));

    APSARA_TEST_TRUE(input->Start());
    APSARA_TEST_EQUAL((size_t)1, PrometheusInputRunner::GetInstance()->scrapeInputsMap["test_config"].size());

    APSARA_TEST_TRUE(input->Stop(true));
    APSARA_TEST_EQUAL((size_t)0, PrometheusInputRunner::GetInstance()->scrapeInputsMap["test_config"].size());

    PrometheusInputRunner::GetInstance()->Stop();
}

void InputPrometheusUnittest::TestScrapeData() {
    ProcessQueueManager::GetInstance()->CreateOrUpdateQueue(ctx.GetProcessQueueKey(), 0);

    unique_ptr<InputPrometheus> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    MockHttpClient* client = new MockHttpClient();
    // with scrape job
    configStr = R"(
        {
            "Type": "input_prometheus",
            "ScrapeConfig": {
                "job_name": "_arms-prom/node-exporter/0",
                "metrics_path": "/metrics",
                "scheme": "http",
                "scrape_interval": 3,
                "scrape_timeout": 3,
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
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));
    input->Start();
    PrometheusInputRunner::GetInstance()->Start();

    std::this_thread::sleep_for(std::chrono::seconds(3));
    ScraperGroup::GetInstance()->scrapeIdWorkMap["_arms-prom/node-exporter/0-index-0"]->StopScrapeLoop();
    ScraperGroup::GetInstance()->scrapeIdWorkMap["_arms-prom/node-exporter/0-index-0"]->client.reset(client);
    ScraperGroup::GetInstance()->scrapeIdWorkMap["_arms-prom/node-exporter/0-index-0"]->StartScrapeLoop();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    unique_ptr<ProcessQueueItem> item;
    ProcessQueueManager::GetInstance()->mQueues[ctx.GetProcessQueueKey()]->Pop(item);

    APSARA_TEST_EQUAL((size_t)11, item->mEventGroup.GetEvents().size());

    // for (PipelineEventPtr& e : item->mEventGroup.MutableEvents()) {
    //     MetricEvent& sourceEvent = e.Cast<MetricEvent>();
    //     cout << sourceEvent.GetName() << endl;
    // }
}

UNIT_TEST_CASE(InputPrometheusUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputPrometheusUnittest, OnPipelineUpdate)
UNIT_TEST_CASE(InputPrometheusUnittest, TestScrapeData)

} // namespace logtail

UNIT_TEST_MAIN