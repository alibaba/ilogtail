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

#include "logger/Logger.h"
#include "prometheus/PrometheusInputRunner.h"
#include "prometheus/Scraper.h"
#include "sdk/Client.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"
#include "unittest/Unittest.h"
using namespace std;

namespace logtail {
class ScraperUnittest : public testing::Test {
public:
    void OnUpdateScrapeJob();
    void OnRemoveScrapeJob();

private:
};


void ScraperUnittest::OnUpdateScrapeJob() {
    // start scraper group first
    ScraperGroup::GetInstance()->Start();
   
    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "172.17.0.1", 9100, 3, 3));
    scrapeTargets[0].mHash = "index-0";
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].mHash, scrapeTargets[0]);

    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeWorkMap.empty());
    ScraperGroup::GetInstance()->UpdateScrapeJob(move(scrapeJobPtr));

    // sleep 6s
    std::this_thread::sleep_for(std::chrono::seconds(6));
    APSARA_TEST_EQUAL((size_t)1, ScraperGroup::GetInstance()->mScrapeJobMap["test_job"]->GetScrapeTargetsMapCopy().size());
    APSARA_TEST_NOT_EQUAL(nullptr, ScraperGroup::GetInstance()->mScrapeWorkMap["test_job"]["index-0"]);
    ScraperGroup::GetInstance()->RemoveScrapeJob("test_job");

    // stop scraper group to clean up
    ScraperGroup::GetInstance()->Stop();
}

void ScraperUnittest::OnRemoveScrapeJob() {
    // start scraper group first
    ScraperGroup::GetInstance()->Start();

    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "172.17.0.1", 9100, 3, 3));
    scrapeTargets[0].mHash = "index-0";
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].mHash, scrapeTargets[0]);

    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeJobMap.empty());
    ScraperGroup::GetInstance()->UpdateScrapeJob(move(scrapeJobPtr));

    // sleep 6s
    std::this_thread::sleep_for(std::chrono::seconds(6));
    APSARA_TEST_EQUAL((size_t)1, ScraperGroup::GetInstance()->mScrapeJobMap["test_job"]->GetScrapeTargetsMapCopy().size());
    APSARA_TEST_NOT_EQUAL(nullptr, ScraperGroup::GetInstance()->mScrapeWorkMap["test_job"]["index-0"]);
    ScraperGroup::GetInstance()->RemoveScrapeJob("test_job");

    // sleep 1s
    std::this_thread::sleep_for(std::chrono::seconds(1));
    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeJobMap.find("test_job") == ScraperGroup::GetInstance()->mScrapeJobMap.end());
    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->mScrapeWorkMap.find("test_job") == ScraperGroup::GetInstance()->mScrapeWorkMap.end());

    // stop scraper group to clean up
    ScraperGroup::GetInstance()->Stop();
}

UNIT_TEST_CASE(ScraperUnittest, OnUpdateScrapeJob)
UNIT_TEST_CASE(ScraperUnittest, OnRemoveScrapeJob)

} // namespace logtail

UNIT_TEST_MAIN