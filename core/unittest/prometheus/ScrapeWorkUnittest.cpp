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


#include <memory>
#include <string>

#include "JsonUtil.h"
#include "Labels.h"
#include "ScrapeConfig.h"
#include "json/value.h"
#include "prometheus/ScrapeWorkEvent.h"
#include "sdk/Common.h"
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

class ScrapeWorkUnittest : public testing::Test {
public:
    void OnStartAndStopScrapeLoop();
    void OnGetRandSleep();

private:
};

void ScrapeWorkUnittest::OnStartAndStopScrapeLoop() {
    Json::Value config;
    string errorMsg;
    string configStr = R"JSON(
    {
        "job_name": "test_job",
        "scheme": "http",
        "metrics_path": "/metrics",
        "scrape_interval": "30s",
        "scrape_timeout": "30s"
    }
    )JSON";
    auto scrapeConfigPtr = std::make_shared<ScrapeConfig>();
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));
    APSARA_TEST_TRUE(scrapeConfigPtr->Init(config));

    auto labels = Labels();
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});

    auto target = ScrapeTarget(labels);

    ScrapeWork work(scrapeConfigPtr, target, 0, 0);
    MockHttpClient* client = new MockHttpClient();
    work.mClient.reset(client);

    // before start
    APSARA_TEST_EQUAL(nullptr, work.mScrapeLoopThread);

    // start
    work.StartScrapeLoop();
    APSARA_TEST_NOT_EQUAL(nullptr, work.mScrapeLoopThread);

    // stop
    work.StopScrapeLoop();
    APSARA_TEST_EQUAL(nullptr, work.mScrapeLoopThread);
}

void ScrapeWorkUnittest::OnGetRandSleep() {
    // target1
    Json::Value config;
    string errorMsg;
    string configStr = R"JSON(
    {
        "job_name": "test_job",
        "scheme": "http",
        "metrics_path": "/metrics",
        "scrape_interval": "30s",
        "scrape_timeout": "30s"
    }
    )JSON";
    auto scrapeConfigPtr = std::make_shared<ScrapeConfig>();
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));
    APSARA_TEST_TRUE(scrapeConfigPtr->Init(config));

    auto labels = Labels();
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});
    auto target = ScrapeTarget(labels);
    ScrapeWork work1(scrapeConfigPtr, target, 0, 0);

    // target2
    configStr = R"JSON(
    {
        "job_name": "test_job",
        "scheme": "http",
        "metrics_path": "/metrics",
        "scrape_interval": "30s",
        "scrape_timeout": "30s"
    }
    )JSON";
    auto scrapeConfigPtr2 = std::make_shared<ScrapeConfig>();
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));
    APSARA_TEST_TRUE(scrapeConfigPtr2->Init(config));
    auto labels2 = Labels();
    labels2.Push(Label{"__address__", "192.168.0.1:1234"});
    labels2.Push(Label{"job", "test_job"});
    auto target2 = ScrapeTarget(labels2);
    ScrapeWork work2(scrapeConfigPtr2, target2, 0, 0);

    uint64_t rand1 = work1.GetRandSleep();
    uint64_t rand2 = work2.GetRandSleep();
    APSARA_TEST_NOT_EQUAL(rand1, rand2);
}

UNIT_TEST_CASE(ScrapeWorkUnittest, OnStartAndStopScrapeLoop)
UNIT_TEST_CASE(ScrapeWorkUnittest, OnGetRandSleep)

} // namespace logtail

UNIT_TEST_MAIN