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
#include "spl/pipeline/SplPipeline.h"
#include "sls_spl/PipelineEventGroupInput.h"
#include "sls_spl/PipelineEventGroupOutput.h"
#include "spl/logger/Logger.h"
#include "logger/Logger.h"
#include "sls_spl/SplConstants.h"
#include "monitor/MetricConstants.h"


using namespace apsara::sls::spl;

namespace logtail {

const std::string ProcessorSPL::sName = "spl";

bool ProcessorSPL::Init(const ComponentConfig& componentConfig, PipelineContext& context) {
    Config config = componentConfig.GetConfig();
    std::vector<std::pair<std::string, std::string>> labels;
    WriteMetrics::GetInstance()->PreparePluginCommonLabels(context.GetProjectName(),
                                                            context.GetLogstoreName(),
                                                            context.GetRegion(),
                                                            context.GetConfigName(),
                                                            sName,
                                                            componentConfig.GetId(),
                                                            labels);

    WriteMetrics::GetInstance()->PrepareMetricsRecordRef(mMetricsRecordRef, std::move(labels));
    
    mProcInRecordsTotal = mMetricsRecordRef.CreateCounter(METRIC_PROC_IN_RECORDS_TOTAL);
    mProcOutRecordsTotal = mMetricsRecordRef.CreateCounter(METRIC_PROC_OUT_RECORDS_TOTAL);
    mProcTimeMS = mMetricsRecordRef.CreateCounter(METRIC_PROC_TIME_MS);

    mSplExcuteErrorCount = mMetricsRecordRef.CreateCounter("proc_spl_excute_error_count");
    mSplExcuteTimeoutErrorCount = mMetricsRecordRef.CreateCounter("proc_spl_excute_timeout_error_count");
    mSplExcuteMemoryExceedErrorCount = mMetricsRecordRef.CreateCounter("proc_spl_excute_memory_exceed_error_count");

    // spl raw statistic
    mProcessMicros = mMetricsRecordRef.CreateCounter("proc_spl_process_micros");
    mInputMicros = mMetricsRecordRef.CreateCounter("proc_spl_input_micros");
    mOutputMicros = mMetricsRecordRef.CreateCounter("proc_spl_output_micros");
    mMemPeakBytes = mMetricsRecordRef.CreateCounter("proc_spl_mem_peak_bytes");
    mTotalTaskCount = mMetricsRecordRef.CreateCounter("proc_spl_total_task_count");
    mSuccTaskCount = mMetricsRecordRef.CreateCounter("proc_spl_succ_task_count");
    mFailTaskCount = mMetricsRecordRef.CreateCounter("proc_spl_fail_task_count");
   
    initSPL();

    // logger初始化
    // logger由调用方提供
    // auto logger = std::make_shared<LogtailLogger>();
    LoggerPtr logger;
    logger = sLogger;

    std::string spl = config.mSpl;

    const uint64_t timeoutMills = 100;
    const int64_t maxMemoryBytes = 2 * 1024L * 1024L * 1024L;
    Error error;
    mSPLPipelinePtr = std::make_shared<SplPipeline>(spl, error, timeoutMills, maxMemoryBytes, logger);
    if (error.code_ != StatusCode::OK) {
        LOG_ERROR(sLogger, ("pipeline create error", error.msg_)("raw spl", spl));
        LogtailAlarm::GetInstance()->SendAlarm(USER_CONFIG_ALARM,
                                                   std::string("invalid spl config: ") + context.GetConfigName() +  std::string("  spl: " + spl),
                                                   context.GetProjectName(),
                                                   context.GetLogstoreName(),
                                                   context.GetRegion());
        return false;
    }
    return true;
}




void ProcessorSPL::Process(PipelineEventGroup& logGroup, std::vector<PipelineEventGroup>& logGroupList) {
    std::string errorMsg;
    size_t inSize = logGroup.GetEvents().size();
    mProcInRecordsTotal->Add(inSize);

    uint64_t startTime = GetCurrentTimeInMicroSeconds();

    std::vector<std::string> colNames{FIELD_CONTENT};

    // 根据spip->getInputSearches()，设置input数组
    std::vector<Input*> inputs;
    for (auto search : mSPLPipelinePtr->getInputSearches()) {
        inputs.push_back(new PipelineEventGroupInput(colNames, logGroup));
    }
    // 根据spip->getOutputLabels()，设置output数组
    std::vector<Output*> outputs;
    for (auto resultTaskLabel : mSPLPipelinePtr->getOutputLabels()) {
        outputs.emplace_back(new PipelineEventGroupOutput(logGroup, logGroupList, resultTaskLabel));
    }

    /// 开始调用pipeline.execute
    // 传入inputs, outputs
    // 输出pipelineStats, error
    PipelineStats pipelineStats;
    auto errCode = mSPLPipelinePtr->execute(inputs, outputs, &errorMsg, &pipelineStats);
    if (errCode != StatusCode::OK) {
        LOG_ERROR(sLogger, ("execute error", errorMsg));
        mSplExcuteErrorCount->Add(1);
        if (errCode == StatusCode::TIMEOUT_ERROR) {
            // todo:: 
            mSplExcuteTimeoutErrorCount->Add(1);
        } else if (errCode == StatusCode::MEM_EXCEEDED) {
            mSplExcuteMemoryExceedErrorCount->Add(1);
        }
    }

    mProcessMicros->Add(pipelineStats.processMicros_);
    mInputMicros->Add(pipelineStats.inputMicros_);
    mOutputMicros->Add(pipelineStats.outputMicros_);
    mMemPeakBytes->Add(pipelineStats.memPeakBytes_);
    mTotalTaskCount->Add(pipelineStats.totalTaskCount_);
    mSuccTaskCount->Add(pipelineStats.succTaskCount_);
    mFailTaskCount->Add(pipelineStats.failTaskCount_);

    for (auto& input : inputs) delete input;
    for (auto& output : outputs) delete output;
    //LOG_DEBUG(sLogger, ("pipelineStats", *pipelineStatsPtr.get()));


    size_t outSize = 0;
    for (auto& logGroup : logGroupList) {
        outSize += logGroup.GetEvents().size();
    }
    mProcOutRecordsTotal->Add(outSize);
    uint64_t durationTime = GetCurrentTimeInMicroSeconds() - startTime;
    mProcTimeMS->Add(durationTime);
    return;
}

bool ProcessorSPL::IsSupportedEvent(const PipelineEventPtr& e) {
    return e.Is<LogEvent>();
}
} // namespace logtail
