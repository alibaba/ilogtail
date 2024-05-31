#include <json/json.h>

#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "logger/Logger.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "prometheus/PrometheusInputRunner.h"
#include "sdk/Client.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"
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
}

void MockHttpClient::AsynSend(sdk::AsynRequest* request) {
}

class PrometheusInputRunnerUnittest : public testing::Test {
public:
    void OnSuccessfulInit();

protected:
    // static void SetUpTestCase() { AppConfig::GetInstance()->mPurageContainerMode = true; }
    // void SetUp() override {
    //     p.mName = "test_config";
    //     ctx.SetConfigName("test_config");
    //     ctx.SetPipeline(p);
    // }

private:
    Pipeline p;
    PipelineContext ctx;
};


void PrometheusInputRunnerUnittest::OnSuccessfulInit() {
    // MockHttpClient* client = new MockHttpClient();
    // PrometheusInputRunner::GetInstance()->client.reset(client);
    auto scrapeTargets = std::set<ScrapeTarget>();
    scrapeTargets.insert(ScrapeTarget{"jonName", 3, 3, "http", "/metrics", "172.17.0.1:9100", 9100, "target_id1"});
    scrapeTargets.insert(ScrapeTarget{"jonName", 5, 5, "http", "/metrics", "172.17.0.1:9100", 9100, "target_id2"});
    auto scrapeJobs = std::vector<ScrapeJob>();
    scrapeJobs.push_back(ScrapeJob{"test_job", "/metrics", "http", 15, 15, scrapeTargets});
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput("test", scrapeJobs);
    PrometheusInputRunner::GetInstance()->Start();
    sleep(7);
    PrometheusInputRunner::GetInstance()->Stop();
}


UNIT_TEST_CASE(PrometheusInputRunnerUnittest, OnSuccessfulInit)


} // namespace logtail

UNIT_TEST_MAIN