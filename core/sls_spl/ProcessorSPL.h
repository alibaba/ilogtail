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

#include "plugin/interface/Processor.h"
#include <string>
#include "monitor/LogtailMetric.h"

namespace apsara::sls::spl {
    class SplPipeline;
}

namespace logtail {

class ProcessorSPL  {
public:
    static const std::string sName;
    bool Init(const ComponentConfig& config, PipelineContext& context);
    void Process(PipelineEventGroup& logGroup, std::vector<PipelineEventGroup>& logGroupList);

    uint64_t stage1TimeTotal = 0;
    uint64_t stage2TimeTotal = 0;
    uint64_t stage3TimeTotal = 0;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e);

private:
    std::shared_ptr<apsara::sls::spl::SplPipeline> mSPLPipelinePtr;
    MetricsRecordRef mMetricsRecordRef;
    
    CounterPtr mProcInRecordsTotal;
    CounterPtr mProcOutRecordsTotal;
    CounterPtr mProcTimeMS;

    CounterPtr mSplExcuteErrorCount;
    CounterPtr mSplExcuteTimeoutErrorCount;
    CounterPtr mSplExcuteMemoryExceedErrorCount;

    CounterPtr mProcessMicros;
    CounterPtr mInputMicros;
    CounterPtr mOutputMicros;
    CounterPtr mMemPeakBytes;
    CounterPtr mTotalTaskCount;
    CounterPtr mSuccTaskCount;
    CounterPtr mFailTaskCount;
};
} // namespace logtail