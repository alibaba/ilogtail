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


#include "Labels.h"
#include "StringTools.h"
#include "prometheus/ScrapeWork.h"
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

class ScrapeTargetUnittest : public testing::Test {
public:
    void OnInitScrapeTarget();
    void OnSetHostAndPort();
    void OnSetLabels();

private:
};

void ScrapeTargetUnittest::OnInitScrapeTarget() {
    ScrapeTarget target = ScrapeTarget("test_job", "/metrics", "http", "", 3, 4, map<string, string>());
    APSARA_TEST_EQUAL(target.mPort, 80UL);
    APSARA_TEST_EQUAL(target.mJobName, "test_job");
    APSARA_TEST_EQUAL(target.mMetricsPath, "/metrics");
    APSARA_TEST_EQUAL(target.mScheme, "http");
    APSARA_TEST_EQUAL(target.mHost, "");
    APSARA_TEST_EQUAL(target.mScrapeTimeout, 4);
    APSARA_TEST_EQUAL(target.mScrapeInterval, 3);
    APSARA_TEST_EQUAL(target.mLabels.Size(), 0UL);
}

void ScrapeTargetUnittest::OnSetHostAndPort() {
    ScrapeTarget target = ScrapeTarget("test_job", "/metrics", "http", "", 3, 4, map<string, string>());
    target.SetHostAndPort("127.0.0.1", 9100);
    APSARA_TEST_EQUAL(target.mPort, 9100UL);
    APSARA_TEST_EQUAL(target.mHost, "127.0.0.1");
}

void ScrapeTargetUnittest::OnSetLabels() {
    ScrapeTarget target = ScrapeTarget("test_job", "/metrics", "http", "", 3, 4, map<string, string>());
    Labels labels;
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});

    target.SetLabels(labels);
    APSARA_TEST_EQUAL(target.mLabels.Size(), 3UL);
    APSARA_TEST_EQUAL(target.mHost, "192.168.0.1");
    APSARA_TEST_EQUAL(target.mPort, 1234UL);

    APSARA_TEST_EQUAL("test_jobhttp://192.168.0.1:1234/metrics" + ToString(labels.Hash()), target.GetHash());
}

class ScrapeWorkUnittest : public testing::Test {
public:
    void OnStartAndStopScrapeLoop();
    void OnGetRandSleep();

private:
};

void ScrapeWorkUnittest::OnStartAndStopScrapeLoop() {
    ScrapeTarget target = ScrapeTarget("test_job", "/metrics", "http", "", 3, 3, map<string, string>());
    target.SetHostAndPort("127.0.0.1", 9100);
    ScrapeWork work(target);
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
    ScrapeTarget target1 = ScrapeTarget("test_job", "/metrics", "http", "", 3, 3, map<string, string>());
    target1.SetHostAndPort("127.0.0.1", 9100);
    ScrapeTarget target2 = ScrapeTarget("test_job", "/metrics", "http", "", 3, 3, map<string, string>());
    target2.SetHostAndPort("127.0.0.1", 9200);
    ScrapeWork work1(target1);
    ScrapeWork work2(target2);
    uint64_t rand1 = work1.GetRandSleep();
    uint64_t rand2 = work2.GetRandSleep();
    APSARA_TEST_NOT_EQUAL(rand1, rand2);
}

UNIT_TEST_CASE(ScrapeTargetUnittest, OnInitScrapeTarget)
UNIT_TEST_CASE(ScrapeTargetUnittest, OnSetHostAndPort)
UNIT_TEST_CASE(ScrapeTargetUnittest, OnSetLabels)
UNIT_TEST_CASE(ScrapeWorkUnittest, OnStartAndStopScrapeLoop)
UNIT_TEST_CASE(ScrapeWorkUnittest, OnGetRandSleep)

} // namespace logtail

UNIT_TEST_MAIN