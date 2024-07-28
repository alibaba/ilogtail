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

#include <json/json.h>
#include <json/value.h>

#include <memory>
#include <string>

#include "PluginRegistry.h"
#include "inner/ProcessorRelabelMetricNative.h"
#include "logger/Logger.h"
#include "pipeline/PipelineContext.h"
#include "plugin/instance/ProcessorInstance.h"
#include "prometheus/Constants.h"
#include "prometheus/PrometheusInputRunner.h"

using namespace std;

namespace logtail {

const string InputPrometheus::sName = "input_prometheus";

/// @brief Init
bool InputPrometheus::Init(const Json::Value& config, uint32_t& pluginIdx, Json::Value&) {
    string errorMsg;

    // config["ScrapeConfig"]
    if (!config.isMember(prometheus::SCRAPE_CONFIG) || !config[prometheus::SCRAPE_CONFIG].isObject()) {
        errorMsg = "ScrapeConfig not found";
        LOG_ERROR(sLogger, ("init prometheus input failed", errorMsg));
        return false;
    }
    const Json::Value& scrapeConfig = config[prometheus::SCRAPE_CONFIG];

    // build scrape job
    mScrapeJobPtr = make_unique<ScrapeJob>();
    if (!mScrapeJobPtr->Init(scrapeConfig)) {
        return false;
    }

    mJobName = mScrapeJobPtr->mJobName;
    mScrapeJobPtr->mInputIndex = mIndex;
    return CreateInnerProcessors(scrapeConfig, pluginIdx);
}

/// @brief register scrape job by PrometheusInputRunner
bool InputPrometheus::Start() {
    LOG_INFO(sLogger, ("input config start", mJobName));

    mScrapeJobPtr->mQueueKey = mContext->GetProcessQueueKey();

    PrometheusInputRunner::GetInstance()->UpdateScrapeInput(mContext->GetConfigName(), std::move(mScrapeJobPtr));
    return true;
}

/// @brief unregister scrape job by PrometheusInputRunner
bool InputPrometheus::Stop(bool) {
    LOG_INFO(sLogger, ("input config stop", mJobName));

    PrometheusInputRunner::GetInstance()->RemoveScrapeInput(mContext->GetConfigName());
    return true;
}

bool InputPrometheus::CreateInnerProcessors(const Json::Value& inputConfig, uint32_t& pluginIdx) {
    unique_ptr<ProcessorInstance> processor;
    processor
        = PluginRegistry::GetInstance()->CreateProcessor(ProcessorRelabelMetricNative::sName, to_string(++pluginIdx));
    if (!processor->Init(inputConfig, *mContext)) {
        return false;
    }
    mInnerProcessors.emplace_back(std::move(processor));
    return true;
}

} // namespace logtail