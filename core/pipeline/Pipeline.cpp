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
#include "processor/ProcessorSplitLogStringNative.h"
#include "processor/ProcessorSplitRegexNative.h"
#include "processor/ProcessorParseApsaraNative.h"
#include "processor/ProcessorParseRegexNative.h"
#include "processor/ProcessorParseJsonNative.h"
#include "processor/ProcessorParseDelimiterNative.h"
#include "processor/ProcessorParseTimestampNative.h"
#include "processor/ProcessorDesensitizeNative.h"
#include "processor/ProcessorTagNative.h"
#include "processor/ProcessorFilterNative.h"
#include "plugin/instance/PluginInstance.h"


namespace logtail {

void genPluginAndNodeID(int& pluginIndex, bool lastOne, PluginInstance::PluginMeta& pluginMeta) {
    pluginIndex ++;
    int childPluginID = pluginIndex;
    if (!lastOne) {
        childPluginID = pluginIndex + 1;
    }
    pluginMeta.pluginID = std::to_string(pluginIndex);
    pluginMeta.childPluginID = std::to_string(childPluginID);
}

bool Pipeline::Init(const PipelineConfig& config) {
    mName = config.mConfigName;
    mConfig = config;

    mContext.SetConfigName(config.mConfigName);
    mContext.SetLogstoreName(config.mCategory);
    mContext.SetProjectName(config.mProjectName);
    mContext.SetRegion(config.mRegion);

    int pluginIndex = 0;

    PluginInstance::PluginMeta pluginMeta;
    // Input plugin
    pluginIndex++;

    if (config.mLogType == STREAM_LOG || config.mLogType == PLUGIN_LOG) {
        return true;
    }

    genPluginAndNodeID(pluginIndex, false, pluginMeta);
    std::unique_ptr<ProcessorInstance> pluginGroupInfo = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorTagNative::sName, pluginMeta);
    if (!InitAndAddProcessor(std::move(pluginGroupInfo), config)) {
        return false;
    }

    if (config.mPluginProcessFlag && !config.mForceEnablePipeline) {
        return true;
    }

    std::unique_ptr<ProcessorInstance> pluginDecoder;
    genPluginAndNodeID(pluginIndex, false, pluginMeta);
    if (config.mLogType == JSON_LOG || !config.IsMultiline()) {
        pluginDecoder = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitLogStringNative::sName,
                                                                       pluginMeta);
    } else {
        pluginDecoder = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitRegexNative::sName,
                                                                       pluginMeta);
    }
    if (!InitAndAddProcessor(std::move(pluginDecoder), config)) {
        return false;
    }

    // APSARA_LOG, REGEX_LOG, STREAM_LOG, JSON_LOG, DELIMITER_LOG, PLUGIN_LOG
    std::unique_ptr<ProcessorInstance> pluginParser;
    genPluginAndNodeID(pluginIndex, false, pluginMeta);
    switch (config.mLogType) {
        case APSARA_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(ProcessorParseApsaraNative::sName,
                                                                          pluginMeta);
            break;
        case REGEX_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(ProcessorParseRegexNative::sName,
                                                                          pluginMeta);
            break;
        case JSON_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(ProcessorParseJsonNative::sName,
                                                                          pluginMeta);
            break;
        case DELIMITER_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(ProcessorParseDelimiterNative::sName,
                                                                          pluginMeta);
            break;
        default:
            return false;
    }
    if (!InitAndAddProcessor(std::move(pluginParser), config)) {
        return false;
    }

    genPluginAndNodeID(pluginIndex, false, pluginMeta);
    std::unique_ptr<ProcessorInstance> pluginTime = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorParseTimestampNative::sName, pluginMeta);
    if (!InitAndAddProcessor(std::move(pluginTime), config)) {
        return false;
    }

    if (!config.mSensitiveWordCastOptions.empty()) {
        genPluginAndNodeID(pluginIndex, false, pluginMeta);
    } else {
        genPluginAndNodeID(pluginIndex, true, pluginMeta);
    }
    std::unique_ptr<ProcessorInstance> pluginFilter = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorFilterNative::sName, pluginMeta);
    if (!InitAndAddProcessor(std::move(pluginFilter), config)) {
        return false;
    }

    if (!config.mSensitiveWordCastOptions.empty()) {
        genPluginAndNodeID(pluginIndex, true, pluginMeta);
        std::unique_ptr<ProcessorInstance> pluginDesensitize = PluginRegistry::GetInstance()->CreateProcessor(
            ProcessorDesensitizeNative::sName, pluginMeta);
        if (!InitAndAddProcessor(std::move(pluginDesensitize), config)) {
            return false;
        }
    }

    return true;
}

void Pipeline::Process(PipelineEventGroup& logGroup) {
    for (auto& p : mProcessorLine) {
        p->Process(logGroup);
    }
}

bool Pipeline::InitAndAddProcessor(std::unique_ptr<ProcessorInstance>&& processor, const PipelineConfig& config) {
    if (!processor) {
        LOG_ERROR(GetContext().GetLogger(),
                  ("CreateProcessor", ProcessorSplitRegexNative::sName)("Error", "Cannot find plugin"));
        return false;
    }
    ComponentConfig componentConfig(processor->Meta().pluginID, config);
    if (!processor->Init(componentConfig, mContext)) {
        LOG_ERROR(GetContext().GetLogger(), ("InitProcessor", processor->Meta().pluginID)("Error", "Init failed"));
        return false;
    }
    mProcessorLine.emplace_back(std::move(processor));
    return true;
}

} // namespace logtail