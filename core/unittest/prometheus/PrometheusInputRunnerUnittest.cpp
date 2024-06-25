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

#include <json/json.h>

#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "logger/Logger.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "prometheus/PrometheusInputRunner.h"
#include "prometheus/Scraper.h"
#include "queue/ProcessQueueManager.h"
#include "sdk/Client.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class PrometheusInputRunnerUnittest : public testing::Test {
public:
    void OnUpdateScrapeInput();
    void OnRemoveScrapeInput();
    void OnSuccessfulStartAndStop();

protected:
private:
};


void PrometheusInputRunnerUnittest::OnUpdateScrapeInput() {
    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "172.17.0.2", 9100, 3, 3));
    scrapeTargets[0].mHash = "index-0";
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].mHash, scrapeTargets[0]);

    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName") == PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    // 代替插件手动注册scrapeJob
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput("testInputName", move(scrapeJobPtr));
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName") != PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    PrometheusInputRunner::GetInstance()->Stop();
}

void PrometheusInputRunnerUnittest::OnRemoveScrapeInput() {
    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "172.17.0.1", 9100, 3, 3));
    scrapeTargets[0].mHash = "index-0";
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].mHash, scrapeTargets[0]);

    // 代替插件手动注册scrapeJob
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput("testInputName", move(scrapeJobPtr));
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName") != PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    PrometheusInputRunner::GetInstance()->RemoveScrapeInput("testInputName");
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("testInputName") == PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
    PrometheusInputRunner::GetInstance()->Stop();
}

void PrometheusInputRunnerUnittest::OnSuccessfulStartAndStop() {
    // 手动构造插件的scrape job 和 target
    auto scrapeTargets = std::vector<ScrapeTarget>();
    scrapeTargets.push_back(ScrapeTarget("test_job", "/metrics", "http", "172.17.0.1", 9100, 3, 3));
    scrapeTargets[0].mHash = "index-0";
    auto scrapeJobs = std::vector<ScrapeJob>();
    std::unique_ptr<ScrapeJob> scrapeJobPtr = make_unique<ScrapeJob>("test_job", "/metrics", "http", 3, 3);
    scrapeJobPtr->AddScrapeTarget(scrapeTargets[0].mHash, scrapeTargets[0]);

    printf("12312312");
    // 代替插件手动注册scrapeJob
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput("test_input_name", move(scrapeJobPtr));
    // start
    PrometheusInputRunner::GetInstance()->Start();
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("test_input_name") != PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());

    sleep(10);

    // stop
    PrometheusInputRunner::GetInstance()->Stop();
    APSARA_TEST_TRUE(PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.find("test_input_name") == PrometheusInputRunner::GetInstance()->mPrometheusInputsMap.end());
}


UNIT_TEST_CASE(PrometheusInputRunnerUnittest, OnUpdateScrapeInput)
UNIT_TEST_CASE(PrometheusInputRunnerUnittest, OnRemoveScrapeInput)
UNIT_TEST_CASE(PrometheusInputRunnerUnittest, OnSuccessfulStartAndStop)


} // namespace logtail

UNIT_TEST_MAIN