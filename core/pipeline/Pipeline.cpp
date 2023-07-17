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

#include "pipeline/Pipeline.h"

#include "pipeline/PipelineContext.h"
#include "plugin/PluginRegistry.h"
#include "processor/ProcessorSplitRegexNative.h"
#include "processor/ProcessorParseRegexNative.h"
#include "processor/ProcessorParseTimestampNative.h"
#include "processor/ProcessorFillSlsGroupInfo.h"

namespace logtail {
Pipeline::~Pipeline() {
}

bool Pipeline::Init(const PipelineConfig& config) {
    mName = config.mConfigName;

    mContext.SetConfigName(config.mConfigName);
    mContext.SetLogstoreName(config.mCategory);
    mContext.SetProjectName(config.mProjectName);
    mContext.SetRegion(config.mRegion);

    std::unique_ptr<ProcessorInstance> plugin1 = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorFillSlsGroupInfo::Name(), std::string(ProcessorFillSlsGroupInfo::Name()) + "/1"); // /0 is the input
    if (!InitAndAddProcessor(plugin1, config)) {
        return false;
    }

    std::unique_ptr<ProcessorInstance> plugin2 = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorSplitRegexNative::Name(), std::string(ProcessorSplitRegexNative::Name()) + "/2");
    if (!InitAndAddProcessor(plugin2, config)) {
        return false;
    }

    std::unique_ptr<ProcessorInstance> plugin3 = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorParseRegexNative::Name(), std::string(ProcessorParseRegexNative::Name()) + "/3");
    if (!InitAndAddProcessor(plugin3, config)) {
        return false;
    }

    std::unique_ptr<ProcessorInstance> plugin4 = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorParseTimestampNative::Name(), std::string(ProcessorParseTimestampNative::Name()) + "/4");
    if (!InitAndAddProcessor(plugin4, config)) {
        return false;
    }

    return true;
}

void Pipeline::Process(PipelineEventGroup& logGroup) {
    for (auto& p : mProcessorLine) {
        p->Process(logGroup);
    }
}

bool Pipeline::InitAndAddProcessor(std::unique_ptr<ProcessorInstance>& processor, const ComponentConfig& config) {
    if (!processor) {
        LOG_ERROR(GetContext().GetLogger(),
                  ("CreateProcessor", ProcessorSplitRegexNative::Name())("Error", "Cannot find plugin"));
        return false;
    }
    if (!processor->Init(config, mContext)) {
        LOG_ERROR(GetContext().GetLogger(), ("InitProcessor", processor->Id())("Error", "Init failed"));
        return false;
    }
    mProcessorLine.emplace_back(std::move(processor));
    return true;
}

} // namespace logtail