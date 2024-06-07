/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "input/InputPrometheus.h"

#include "common/ParamExtractor.h"
#include "pipeline/PipelineContext.h"
#include "prometheus/PrometheusInputRunner.h"

using namespace std;

namespace logtail {

const string InputPrometheus::sName = "input_prometheus";

InputPrometheus::InputPrometheus() {
}

/// @brief Init
bool InputPrometheus::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;

    // config["ScrapeConfig"]
    if (!config.isMember("ScrapeConfig")) {
        errorMsg = "ScrapeConfig not found";
        LOG_ERROR(sLogger, ("init prometheus input failed", errorMsg));
        return false;
    }
    const Json::Value& scrapeConfig = config["ScrapeConfig"];

    // build scrape job
    scrapeJob = ScrapeJob(scrapeConfig);
    if (!scrapeJob.isValid()) {
        return false;
    }

    // 根据config参数构造target 过渡
    // 过渡方法 传参targets
    if (scrapeConfig["scrape_targets"].isArray()) {
        for (auto targetConfig : scrapeConfig["scrape_targets"]) {
            ScrapeTarget target(targetConfig);
            target.jobName = scrapeJob.jobName;
            target.scheme = scrapeJob.scheme;
            target.metricsPath = scrapeJob.metricsPath;
            target.scrapeInterval = scrapeJob.scrapeInterval;
            target.scrapeTimeout = scrapeJob.scrapeTimeout;
            target.targetId = scrapeJob.jobName + "-index-" + to_string(scrapeJob.scrapeTargets.size());
            scrapeJob.scrapeTargets.push_back(target);
        }
    }

    // 从Master中请求scrapetargets，但当前缺少依赖master，由config传入

    // 为每个job设置queueKey、inputIndex，inputIndex暂时用0代替
    scrapeJob.queueKey = mContext->GetProcessQueueKey();
    scrapeJob.inputIndex = 0;
    for (ScrapeTarget& target : scrapeJob.scrapeTargets) {
        target.jobName = scrapeJob.jobName;
        target.queueKey = mContext->GetProcessQueueKey();
        target.inputIndex = 0;
    }

    return true;
}

/// @brief register scrape job by PrometheusInputRunner
bool InputPrometheus::Start() {
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput(mContext->GetConfigName(), vector<ScrapeJob>{scrapeJob});
    return true;
}

/// @brief unregister scrape job by PrometheusInputRunner
bool InputPrometheus::Stop(bool isPipelineRemoving) {
    PrometheusInputRunner::GetInstance()->RemoveScrapeInput(mContext->GetConfigName());
    return true;
}

} // namespace logtail