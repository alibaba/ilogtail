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

#include "spl/ProcessorSPL.h"
#include <curl/curl.h>
#include <iostream>
#include "spl/pipeline/SplPipeline.h"
#include "spl/PipelineEventGroupInput.h"
#include "spl/PipelineEventGroupOutput.h"
#include "spl/logger/Logger.h"
#include "logger/Logger.h"
#include "spl/SplConstants.h"
#include "monitor/MetricConstants.h"
#include "common/ParamExtractor.h"


using namespace apsara::sls::spl;

namespace logtail {

const std::string ProcessorSPL::sName = "spl";

bool ProcessorSPL::Init(const Json::Value& config) {
    mSplExcuteErrorCount = GetMetricsRecordRef().CreateCounter("proc_spl_excute_error_count");
    mSplExcuteTimeoutErrorCount = GetMetricsRecordRef().CreateCounter("proc_spl_excute_timeout_error_count");
    mSplExcuteMemoryExceedErrorCount = GetMetricsRecordRef().CreateCounter("proc_spl_excute_memory_exceed_error_count");

    // spl raw statistic
    mProcessMicros = GetMetricsRecordRef().CreateCounter("proc_spl_process_micros");
    mInputMicros = GetMetricsRecordRef().CreateCounter("proc_spl_input_micros");
    mOutputMicros = GetMetricsRecordRef().CreateCounter("proc_spl_output_micros");
    mMemPeakBytes = GetMetricsRecordRef().CreateCounter("proc_spl_mem_peak_bytes");
    mTotalTaskCount = GetMetricsRecordRef().CreateCounter("proc_spl_total_task_count");
    mSuccTaskCount = GetMetricsRecordRef().CreateCounter("proc_spl_succ_task_count");
    mFailTaskCount = GetMetricsRecordRef().CreateCounter("proc_spl_fail_task_count");
   

    PipelineOptions splOptions;
    // different parse mode support different spl operators
    splOptions.parserMode = parser::ParserMode::LOGTAIL;
    // sampling for error
    splOptions.errorSampling = false;

    initSPL(&splOptions);

    LoggerPtr logger;
    logger = sLogger;

    std::string errorMsg;

    if (!GetMandatoryStringParam(config, "Spl", mSpl, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }
    if (!GetOptionalUIntParam(config, "TimeoutMilliSeconds", mTimeoutMills, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    } 
    if (!GetOptionalUIntParam(config, "MaxMemoryBytes", mMaxMemoryBytes, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }

    Error error;
    
    mSPLPipelinePtr = std::make_shared<apsara::sls::spl::SplPipeline>(mSpl, error, (u_int64_t)mTimeoutMills, (int64_t)mMaxMemoryBytes, logger);
    if (error.code_ != StatusCode::OK) {
        LOG_ERROR(sLogger, ("pipeline create error", error.msg_)("project", GetContext().GetProjectName())("logstore", GetContext().GetLogstoreName())("configName", GetContext().GetConfigName())("raw spl", mSpl));
        LogtailAlarm::GetInstance()->SendAlarm(USER_CONFIG_ALARM,
                                                   std::string("invalid spl config: ") + GetContext().GetConfigName() +  std::string("  spl: " + mSpl),
                                                   GetContext().GetProjectName(),
                                                   GetContext().GetLogstoreName(),
                                                   GetContext().GetRegion());
        return false;
    }
    return true;
}


void ProcessorSPL::Process(PipelineEventGroup& logGroup) {
}


void ProcessorSPL::Process(std::vector<PipelineEventGroup>& logGroupList) {
    std::string errorMsg;
    PipelineEventGroup logGroup = std::move(logGroupList[0]);
    std::vector<PipelineEventGroup>().swap(logGroupList);

    size_t inSize = logGroup.GetEvents().size();
    std::vector<std::string> colNames{FIELD_CONTENT};
    // 根据spip->getInputSearches()，设置input数组
    std::vector<Input*> inputs;
    for (const auto& search : mSPLPipelinePtr->getInputSearches()) {
        inputs.push_back(new PipelineEventGroupInput(colNames, logGroup));
    }
    // 根据spip->getOutputLabels()，设置output数组
    std::vector<Output*> outputs;
    for (const auto& resultTaskLabel : mSPLPipelinePtr->getOutputLabels()) {
        outputs.emplace_back(new PipelineEventGroupOutput(logGroup, logGroupList, resultTaskLabel));
    }

    // 开始调用pipeline.execute
    // 传入inputs, outputs
    // 输出pipelineStats, error
    PipelineStats pipelineStats;
    auto errCode = mSPLPipelinePtr->execute(inputs, outputs, &errorMsg, &pipelineStats);

    if (errCode != StatusCode::OK) {
        LOG_ERROR(sLogger, ("execute error", errorMsg));
        mSplExcuteErrorCount->Add(1);
        // 出现错误，把原数据放回来
        logGroupList.emplace_back(std::move(logGroup));
        if (errCode == StatusCode::TIMEOUT_ERROR) {
            mSplExcuteTimeoutErrorCount->Add(1);
        } else if (errCode == StatusCode::MEM_EXCEEDED) {
            mSplExcuteMemoryExceedErrorCount->Add(1);
        }
    } else {
        mProcessMicros->Add(pipelineStats.processMicros_);
        mInputMicros->Add(pipelineStats.inputMicros_);
        mOutputMicros->Add(pipelineStats.outputMicros_);
        mMemPeakBytes->Add(pipelineStats.memPeakBytes_);
        mTotalTaskCount->Add(pipelineStats.totalTaskCount_);
        mSuccTaskCount->Add(pipelineStats.succTaskCount_);
        mFailTaskCount->Add(pipelineStats.failTaskCount_);
    }

    for (auto& input : inputs) delete input;
    for (auto& output : outputs) delete output;
    return;
}


bool ProcessorSPL::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}
} // namespace logtail