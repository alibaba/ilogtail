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
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", 3, 3, "172.17.0.1:9100", 9100));
    scrapeTargets[0].targetId = "index-0";
    ScrapeJob scrapeJob = ScrapeJob("test_job", "/metrics", "http", 3, 3);
    scrapeJob.scrapeTargets = scrapeTargets;

    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->scrapeJobTargetsMap.empty());
    ScraperGroup::GetInstance()->UpdateScrapeJob(scrapeJob);

    // sleep 1s
    std::this_thread::sleep_for(std::chrono::seconds(1));
    APSARA_TEST_EQUAL((size_t)1, ScraperGroup::GetInstance()->scrapeJobTargetsMap["test_job"].size());
    APSARA_TEST_NOT_EQUAL(nullptr, ScraperGroup::GetInstance()->scrapeIdWorkMap["index-0"]);
    ScraperGroup::GetInstance()->RemoveScrapeJob(scrapeJob);

    // stop scraper group to clean up
    ScraperGroup::GetInstance()->Stop();
}

void ScraperUnittest::OnRemoveScrapeJob() {
    // start scraper group first
    ScraperGroup::GetInstance()->Start();

    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", 3, 3, "172.17.0.1:9100", 9100));
    scrapeTargets[0].targetId = "index-0";
    ScrapeJob scrapeJob = ScrapeJob("test_job", "/metrics", "http", 3, 3);
    scrapeJob.scrapeTargets = scrapeTargets;

    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->scrapeJobTargetsMap.empty());
    ScraperGroup::GetInstance()->UpdateScrapeJob(scrapeJob);

    // sleep 1s
    std::this_thread::sleep_for(std::chrono::seconds(3));
    APSARA_TEST_EQUAL((size_t)1, ScraperGroup::GetInstance()->scrapeJobTargetsMap["test_job"].size());
    APSARA_TEST_NOT_EQUAL(nullptr, ScraperGroup::GetInstance()->scrapeIdWorkMap["index-0"]);
    ScraperGroup::GetInstance()->RemoveScrapeJob(scrapeJob);

    // sleep 1s
    std::this_thread::sleep_for(std::chrono::seconds(3));
    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->scrapeJobTargetsMap.empty());
    APSARA_TEST_TRUE(ScraperGroup::GetInstance()->scrapeIdWorkMap.empty());

    // stop scraper group to clean up
    ScraperGroup::GetInstance()->Stop();
}

UNIT_TEST_CASE(ScraperUnittest, OnUpdateScrapeJob)
UNIT_TEST_CASE(ScraperUnittest, OnRemoveScrapeJob)

} // namespace logtail

UNIT_TEST_MAIN