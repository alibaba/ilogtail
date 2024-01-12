/*
 * Copyright 2022 iLogtail Authors
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

#include "plugin/instance/ProcessorInstance.h"

#include <cstdint>

#include "logger/Logger.h"
#include "monitor/MetricConstants.h"

namespace logtail {

bool ProcessorInstance::Init(const ComponentConfig& config, PipelineContext& context) {
    mPlugin->SetContext(context);
    auto meta = Meta();
    mPlugin->SetMetricsRecordRef(Name(), meta.pluginID, meta.childPluginID);
    bool inited = mPlugin->Init(config);
    if (!inited) {
        return inited;
    }
    // should init plugin first， then could GetMetricsRecordRef from plugin
    mProcInRecordsTotal = mPlugin->GetMetricsRecordRef().CreateCounter(METRIC_PROC_IN_RECORDS_TOTAL);
    mProcOutRecordsTotal = mPlugin->GetMetricsRecordRef().CreateCounter(METRIC_PROC_OUT_RECORDS_TOTAL);
    mProcTimeMS = mPlugin->GetMetricsRecordRef().CreateCounter(METRIC_PROC_TIME_MS);
    
    return inited;
}

void ProcessorInstance::Process(PipelineEventGroup& logGroup) {
    size_t inSize = logGroup.GetEvents().size();

    mProcInRecordsTotal->Add(inSize);

    uint64_t startTime = GetCurrentTimeInMilliSeconds();
    mPlugin->Process(logGroup);
    uint64_t durationTime = GetCurrentTimeInMilliSeconds() - startTime;
    
    mProcTimeMS->Add(durationTime);

    size_t outSize = logGroup.GetEvents().size();
    mProcOutRecordsTotal->Add(outSize);
    LOG_DEBUG(mPlugin->GetContext().GetLogger(), ("Processor", Meta().pluginID)("InSize", inSize)("OutSize", outSize));
}

} // namespace logtail
