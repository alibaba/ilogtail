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
#include <thread>

#include "prometheus/Labels.h"
#include "prometheus/ScrapeWork.h"
#include "prometheus/Scraper.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

// MockHttpClient
class MockScraperHttpClient : public sdk::HTTPClient {
public:
    MockScraperHttpClient();

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

MockScraperHttpClient::MockScraperHttpClient() {
}

void MockScraperHttpClient::Send(const std::string& httpMethod,
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
    httpMessage.statusCode = 200;
}

void MockScraperHttpClient::AsynSend(sdk::AsynRequest* request) {
}

class ScraperGroupUnittest : public testing::Test {
public:
    void OnUpdateScrapeJob();
    void OnRemoveScrapeJob();

private:
};


void ScraperGroupUnittest::OnUpdateScrapeJob() {
    // start scraper group first
    ScraperGroup::GetInstance()->Start();

    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "", 3, 3, map<string, string>()));
    Labels labels;
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});

    scrapeTargets[0].SetLabels(labels);
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].GetHash(), scrapeTargets[0]);
    scrapeJobPtr->mClient.reset(new MockScraperHttpClient());

    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeWorkMap.empty());
    ScraperGroup::GetInstance()->UpdateScrapeJob(std::move(scrapeJobPtr));

    // sleep 6s
    std::this_thread::sleep_for(std::chrono::seconds(6));
    APSARA_TEST_EQUAL((size_t)1,
                      ScraperGroup::GetInstance()->mScrapeJobMap["test_job"]->GetScrapeTargetsMapCopy().size());
    APSARA_TEST_NOT_EQUAL(nullptr, ScraperGroup::GetInstance()->mScrapeWorkMap["test_job"][scrapeTargets[0].GetHash()]);
    ScraperGroup::GetInstance()->RemoveScrapeJob("test_job");

    // stop scraper group to clean up
    ScraperGroup::GetInstance()->Stop();
}

void ScraperGroupUnittest::OnRemoveScrapeJob() {
    // start scraper group first
    ScraperGroup::GetInstance()->Start();

    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "", 3, 4, map<string, string>()));
    Labels labels;
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});

    scrapeTargets[0].SetLabels(labels);
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].GetHash(), scrapeTargets[0]);
    scrapeJobPtr->mClient.reset(new MockScraperHttpClient());

    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeJobMap.empty());
    ScraperGroup::GetInstance()->UpdateScrapeJob(std::move(scrapeJobPtr));

    // sleep 6s
    std::this_thread::sleep_for(std::chrono::seconds(6));
    APSARA_TEST_EQUAL((size_t)1,
                      ScraperGroup::GetInstance()->mScrapeJobMap["test_job"]->GetScrapeTargetsMapCopy().size());
    APSARA_TEST_NOT_EQUAL(nullptr, ScraperGroup::GetInstance()->mScrapeWorkMap["test_job"][scrapeTargets[0].GetHash()]);
    ScraperGroup::GetInstance()->RemoveScrapeJob("test_job");

    // sleep 1s
    std::this_thread::sleep_for(std::chrono::seconds(1));
    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeJobMap.find("test_job")
                     == ScraperGroup::GetInstance()->mScrapeJobMap.end());
    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeWorkMap.find("test_job")
                     == ScraperGroup::GetInstance()->mScrapeWorkMap.end());

    // stop scraper group to clean up
    ScraperGroup::GetInstance()->Stop();
}

UNIT_TEST_CASE(ScraperGroupUnittest, OnUpdateScrapeJob)
UNIT_TEST_CASE(ScraperGroupUnittest, OnRemoveScrapeJob)

} // namespace logtail

UNIT_TEST_MAIN