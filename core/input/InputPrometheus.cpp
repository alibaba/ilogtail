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
#include "logger/Logger.h"
#include "pipeline/PipelineContext.h"
#include "prometheus/PrometheusInputRunner.h"

using namespace std;

namespace logtail {

const string InputPrometheus::sName = "input_prometheus";

/// @brief Init
bool InputPrometheus::Init(const Json::Value& config, uint32_t& pluginIdx, Json::Value& optionalGoPipeline) {
    LOG_INFO(sLogger,("LOG_INFO input config", config.toStyledString()));
    
    string errorMsg;

    // config["ScrapeConfig"]
    if (!config.isMember("ScrapeConfig")) {
        errorMsg = "ScrapeConfig not found";
        LOG_ERROR(sLogger, ("init prometheus input failed", errorMsg));
        return false;
    }
    const Json::Value& scrapeConfig = config["ScrapeConfig"];

    // build scrape job
    mScrapeJobPtr = make_unique<ScrapeJob>(scrapeConfig);
    if (!mScrapeJobPtr->isValid()) {
        LOG_ERROR(sLogger, ("scrape config not valid", config.toStyledString()));
        return false;
    }

    mJobName = mScrapeJobPtr->mJobName;

    // 为每个job设置queueKey、inputIndex，inputIndex暂时用0代替
    mScrapeJobPtr->queueKey = mContext->GetProcessQueueKey();
    mScrapeJobPtr->inputIndex = pluginIdx;

    LOG_INFO(sLogger,("input config init success", mJobName));
    return true;
}

/// @brief register scrape job by PrometheusInputRunner
bool InputPrometheus::Start() {
    LOG_INFO(sLogger,("input config start", mJobName));
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput(mContext->GetConfigName(), move(mScrapeJobPtr));
    return true;
}

/// @brief unregister scrape job by PrometheusInputRunner
bool InputPrometheus::Stop(bool isPipelineRemoving) {
    LOG_INFO(sLogger,("input config stop", mJobName));
    PrometheusInputRunner::GetInstance()->RemoveScrapeInput(mContext->GetConfigName());
    return true;
}

} // namespace logtail