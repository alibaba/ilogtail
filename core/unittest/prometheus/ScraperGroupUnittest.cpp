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

#include "JsonUtil.h"
#include "prometheus/Labels.h"
#include "prometheus/ScrapeJobEvent.h"
#include "prometheus/ScraperGroup.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class ScraperGroupUnittest : public testing::Test {
public:
    void OnUpdateScrapeJob();
    void OnRemoveScrapeJob();

    void TestStartAndStop();

protected:
    void SetUp() override {
        setenv("POD_NAME", "prometheus-test", 1);
        setenv("OPERATOR_HOST", "127.0.0.1", 1);
        setenv("OPERATOR_PORT", "12345", 1);
    }
    void TearDown() override {
        unsetenv("POD_NAME");
        unsetenv("OPERATOR_HOST");
        unsetenv("OPERATOR_PORT");
    }

private:
};


void ScraperGroupUnittest::OnUpdateScrapeJob() {
    Json::Value config;
    string errorMsg;
    string configStr;
    auto scraperGroup = make_unique<ScraperGroup>();

    configStr = R"JSON({
        "job_name": "test_job",
        "scheme": "http",
        "metrics_path": "/metrics",
        "scrape_interval": "5s",
        "scrape_timeout": "5s"
    })JSON";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));


    // build scrape job and target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    Labels labels;
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});
    scrapeTargets.emplace_back(labels);

    std::unique_ptr<TargetsSubscriber> scrapeJobPtr = make_unique<TargetsSubscriber>();

    APSARA_TEST_TRUE(scrapeJobPtr->Init(config));

    APSARA_TEST_TRUE(scraperGroup->mJobEventMap.empty());
    scraperGroup->UpdateScrapeJob(std::move(scrapeJobPtr));

    APSARA_TEST_EQUAL((size_t)1, scraperGroup->mJobEventMap.count("test_job"));
}

void ScraperGroupUnittest::OnRemoveScrapeJob() {
    Json::Value config;
    string errorMsg;
    string configStr;
    auto scraperGroup = make_unique<ScraperGroup>();

    // build scrape job and target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    Labels labels;
    labels.Push(Label{"test_label", "test_value"});
    labels.Push(Label{"__address__", "192.168.0.1:1234"});
    labels.Push(Label{"job", "test_job"});
    scrapeTargets.emplace_back(labels);

    configStr = R"JSON({
        "job_name": "test_job",
        "scheme": "http",
        "metrics_path": "/metrics",
        "scrape_interval": "5s",
        "scrape_timeout": "5s"
    })JSON";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));

    std::unique_ptr<TargetsSubscriber> scrapeJobPtr = make_unique<TargetsSubscriber>();
    APSARA_TEST_TRUE(scrapeJobPtr->Init(config));


    APSARA_TEST_TRUE(scraperGroup->mJobEventMap.empty());
    scraperGroup->UpdateScrapeJob(std::move(scrapeJobPtr));

    APSARA_TEST_EQUAL((size_t)1, scraperGroup->mJobEventMap.count("test_job"));
    scraperGroup->RemoveScrapeJob("test_job");

    APSARA_TEST_TRUE(scraperGroup->mJobEventMap.find("test_job") == scraperGroup->mJobEventMap.end());

    // stop scraper group to clean up
    scraperGroup->Stop();
}

void ScraperGroupUnittest::TestStartAndStop() {
    std::unique_ptr<ScraperGroup> scraperGroup = make_unique<ScraperGroup>();
    scraperGroup->Start();
    {
        std::lock_guard<mutex> lock(scraperGroup->mStartMux);
        APSARA_TEST_EQUAL(scraperGroup->mIsStarted, true);
    }
    scraperGroup->Stop();
    {
        std::lock_guard<mutex> lock(scraperGroup->mStartMux);
        APSARA_TEST_EQUAL(scraperGroup->mIsStarted, false);
    }
    APSARA_TEST_EQUAL(0UL, scraperGroup->mJobEventMap.size());
}

UNIT_TEST_CASE(ScraperGroupUnittest, OnUpdateScrapeJob)
UNIT_TEST_CASE(ScraperGroupUnittest, OnRemoveScrapeJob)
UNIT_TEST_CASE(ScraperGroupUnittest, TestStartAndStop)

} // namespace logtail

UNIT_TEST_MAIN