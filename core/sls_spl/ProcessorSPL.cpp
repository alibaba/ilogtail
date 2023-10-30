/*
 * Copyright 2023 iLogtail Authors
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

#include "sls_spl/ProcessorSPL.h"
#include <curl/curl.h>
#include <iostream>
#include "logger/StdoutLogger.h"
#include "pipeline/SplPipeline.h"
#include "sls_spl/PipelineEventGroupInput.h"
#include "sls_spl/PipelineEventGroupOutput.h"
#include "logger/Logger.h"
#include "sls_spl/LogtailLogger.h"
#include "sls_spl/SplConstants.h"


using namespace apsara::sls::spl;

namespace logtail {

const std::string ProcessorSPL::sName = "spl";

bool ProcessorSPL::Init(const ComponentConfig& componentConfig, PipelineContext& context) {
    Config config = componentConfig.GetConfig();
   
    initSPL();

    // logger初始化
    // logger由调用方提供
    auto logger = std::make_shared<LogtailLogger>();

    std::string spl = config.mSpl;

    const uint64_t timeoutMills = 100;
    const int64_t maxMemoryBytes = 2 * 1024L * 1024L * 1024L;
    Error error;
    mSPLPipelinePtr = std::make_shared<SplPipeline>(spl, error, timeoutMills, maxMemoryBytes, logger);
    if (error.code_ != StatusCode::OK) {
        LOG_ERROR(sLogger, ("pipeline create error", error.msg_)("raw spl", spl));
        return false;
    }
    return true;
}


void ProcessorSPL::Process(PipelineEventGroup& logGroup, std::vector<PipelineEventGroup>& logGroupList) {
    std::string errorMsg;

    std::vector<std::string> colNames{FIELD_CONTENT};

    auto input = std::make_shared<PipelineEventGroupInput>(colNames, logGroup);

    // 根据spip->getInputSearches()，设置input数组
    std::vector<InputPtr> inputs;
    for (auto search : mSPLPipelinePtr->getInputSearches()) {
        inputs.push_back(input);
    }

    // 根据spip->getOutputLabels()，设置output数组
    std::vector<OutputPtr> outputs;

    for (auto resultTaskLabel : mSPLPipelinePtr->getOutputLabels()) {
        outputs.emplace_back(std::make_shared<PipelineEventGroupOutput>(logGroup, logGroupList, resultTaskLabel));
    }

    // 开始调用pipeline.execute
    // 传入inputs, outputs
    // 输出pipelineStats, error
    PipelineStats pipelineStats;
    auto errCode = mSPLPipelinePtr->execute(inputs, outputs, &errorMsg, &pipelineStats);
    if (errCode != StatusCode::OK) {
        LOG_ERROR(sLogger, ("execute error", errorMsg));
        return;
    }

    //LOG_DEBUG(sLogger, ("pipelineStats", *pipelineStatsPtr.get()));
    return;
}

bool ProcessorSPL::IsSupportedEvent(const PipelineEventPtr& /*e*/) {
    return true;
}

} // namespace logtail