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

#pragma once

#include "plugin/instance/PluginInstance.h"
#include "pipeline/PipelineConfig.h"
#include "processor/Processor.h"
#include "monitor/LogtailMetric.h"
#include "monitor/MetricConstants.h"

namespace logtail {


class ProcessorInstance : public PluginInstance {
public:
    ProcessorInstance(Processor* plugin, const std::string& pluginId) : PluginInstance(pluginId), mPlugin(plugin) {}

    bool Init(const ComponentConfig& config, PipelineContext& context);
    void Process(PipelineEventGroup& logGroup);

private:
    std::unique_ptr<Processor> mPlugin;

    CounterPtr mProcInRecordsTotal;
    CounterPtr mProcOutRecordsTotal;
    // CounterPtr mProcInRecordsSizeBytes;
    // CounterPtr mProcOutRecordsSizeBytes;
    CounterPtr mProcTimeMS;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseRegexNativeUnittest;
    friend class ProcessorParseTimestampNativeUnittest;
    friend class ProcessorParseJsonNativeUnittest;
    friend class ProcessorParseApsaraNativeUnittest;
    friend class ProcessorParseDelimiterNativeUnittest;
    friend class ProcessorFilterNativeUnittest;
    friend class ProcessorDesensitizeNativeUnittest;
#endif
};

} // namespace logtail