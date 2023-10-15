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
#include "Go_pipeline/LogtailPlugin.h"
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
#include "Pipeline.h"

using namespace std;

namespace logtail {

bool Pipeline::Init(const PipelineConfig& config) {
    mName = config.mConfigName;
    mConfig1 = config;

    mContext.SetConfigName(config.mConfigName);
    mContext.SetLogstoreName(config.mCategory);
    mContext.SetProjectName(config.mProjectName);
    mContext.SetRegion(config.mRegion);

    int pluginIndex = 0;
    // Input plugin
    pluginIndex++;

    if (config.mLogType == STREAM_LOG || config.mLogType == PLUGIN_LOG) {
        return true;
    }

    std::unique_ptr<ProcessorInstance> pluginGroupInfo = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorTagNative::Name(), std::string(ProcessorTagNative::Name()) + "/" + std::to_string(pluginIndex++));
    if (!InitAndAddProcessor(std::move(pluginGroupInfo), config)) {
        return false;
    }

    if (config.mPluginProcessFlag && !config.mForceEnablePipeline) {
        return true;
    }

    std::unique_ptr<ProcessorInstance> pluginDecoder;
    if (config.mLogType == JSON_LOG || !config.IsMultiline()) {
        pluginDecoder = PluginRegistry::GetInstance()->CreateProcessor(
            ProcessorSplitLogStringNative::Name(),
            std::string(ProcessorSplitLogStringNative::Name()) + "/" + std::to_string(pluginIndex++));
    } else {
        pluginDecoder = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitRegexNative::Name(),
                                                                       std::string(ProcessorSplitRegexNative::Name())
                                                                           + "/" + std::to_string(pluginIndex++));
    }
    if (!InitAndAddProcessor(std::move(pluginDecoder), config)) {
        return false;
    }

    // APSARA_LOG, REGEX_LOG, STREAM_LOG, JSON_LOG, DELIMITER_LOG, PLUGIN_LOG
    std::unique_ptr<ProcessorInstance> pluginParser;
    switch (config.mLogType) {
        case APSARA_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(
                ProcessorParseApsaraNative::Name(),
                std::string(ProcessorParseApsaraNative::Name()) + "/" + std::to_string(pluginIndex++));
            break;
        case REGEX_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(ProcessorParseRegexNative::Name(),
                                                                          std::string(ProcessorParseRegexNative::Name())
                                                                              + "/" + std::to_string(pluginIndex++));
            break;
        case JSON_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(ProcessorParseJsonNative::Name(),
                                                                          std::string(ProcessorParseJsonNative::Name())
                                                                              + "/" + std::to_string(pluginIndex++));
            break;
        case DELIMITER_LOG:
            pluginParser = PluginRegistry::GetInstance()->CreateProcessor(
                ProcessorParseDelimiterNative::Name(),
                std::string(ProcessorParseDelimiterNative::Name()) + "/" + std::to_string(pluginIndex++));
            break;
        default:
            return false;
    }
    if (!InitAndAddProcessor(std::move(pluginParser), config)) {
        return false;
    }

    std::unique_ptr<ProcessorInstance> pluginTime = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorParseTimestampNative::Name(),
        std::string(ProcessorParseTimestampNative::Name()) + "/" + std::to_string(pluginIndex++));
    if (!InitAndAddProcessor(std::move(pluginTime), config)) {
        return false;
    }

    std::unique_ptr<ProcessorInstance> pluginFilter = PluginRegistry::GetInstance()->CreateProcessor(
        ProcessorFilterNative::Name(),
        std::string(ProcessorFilterNative::Name()) + "/" + std::to_string(pluginIndex++));
    if (!InitAndAddProcessor(std::move(pluginFilter), config)) {
        return false;
    }

    if (!config.mSensitiveWordCastOptions.empty()) {
        std::unique_ptr<ProcessorInstance> pluginDesensitize = PluginRegistry::GetInstance()->CreateProcessor(
            ProcessorDesensitizeNative::Name(),
            std::string(ProcessorDesensitizeNative::Name()) + "/" + std::to_string(pluginIndex++));
        if (!InitAndAddProcessor(std::move(pluginDesensitize), config)) {
            return false;
        }
    }

    return true;
}

bool Pipeline::Init(NewConfig&& config) {
    // TODO:
    // 1. exactlyonce
    // 2. priority
    const Table& table = *config.mDetail;
    int16_t pluginIndex = 0;
    Table* itr = nullptr;

    itr = table.find("global");
    if (!itr->is_object()) {
        LOG_ERROR(sLogger, ("failed to init pipeline", "global module is not of type object")("pipeline", Name()));
        return false;
    }
    if (mContext.InitGlobal(*itr)) {
        return false;
    }

    itr = table.find("inputs");
    if (!itr) {
        LOG_ERROR(sLogger, ("failed to init pipeline", "mandatory inputs module is missing")("pipeline", Name()));
        return false;
    }
    if (!itr->is_array()) {
        LOG_ERROR(sLogger,
                  ("failed to init pipeline", "mandatory inputs module is not of type array")("pipeline", Name()));
        return false;
    }
    if (itr->empty()) {
        LOG_ERROR(sLogger, ("failed to init pipeline", "mandatory inputs module has no plugin")("pipeline", Name()));
        return false;
    }
    for (size_t i = 0; i < itr->size(); ++i) {
        const Table& plugin = (*itr)[i];
        if (!plugin.is_object()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline", "param inputs[" + ToString(i) + "] is not of type object")("pipeline",
                                                                                                             Name()));
            return false;
        }
        Table* it = plugin.find("Type");
        if (!it->is_string()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline",
                       "param inputs[" + ToString(i) + "].Type is not of type string")("pipeline", Name()));
            return false;
        }
        unique_ptr<InputInstance> input = PluginRegistry::GetInstance()->CreateInput(
            it->as_string(), it->as_string() + "/" + std::to_string(++pluginIndex));
        if (input) {
            if (!input->Init(plugin, mContext)) {
                return false;
            }
            mInputs.emplace_back(std::move(input));
        } else {
            if (!PluginRegistry::GetInstance()->IsValidGoPlugin(it->as_string())) {
                LOG_ERROR(
                    sLogger,
                    ("failed to init pipeline", "plugin" + it->as_string() + "is not supported")("pipeline", Name()));
                return false;
            }
            PushPluginToGoPipelines(plugin, mGoPipelineWithInput);
        }
    }

    itr = table.find("processors");
    if (!itr->is_array()) {
        LOG_ERROR(sLogger, ("failed to init pipeline", "processors module is not of type array")("pipeline", Name()));
        return false;
    }
    for (size_t i = 0; i < itr->size(); ++i) {
        const Table& plugin = (*itr)[i];
        if (!plugin.is_object()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline",
                       "param processors[" + ToString(i) + "] is not of type object")("pipeline", Name()));
            return false;
        }
        Table* it = plugin.find("Type");
        if (!it->is_string()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline",
                       "param processors[" + ToString(i) + "].Type is not of type string")("pipeline", Name()));
            return false;
        }
        unique_ptr<ProcessorInstance> processor = PluginRegistry::GetInstance()->CreateProcessor(
            it->as_string(), it->as_string() + "/" + std::to_string(++pluginIndex));
        if (processor) {
            if (!processor->Init(plugin, mContext)) {
                return false;
            }
            mProcessorLine.emplace_back(std::move(processor));
        } else {
            if (!PluginRegistry::GetInstance()->IsValidGoPlugin(it->as_string())) {
                LOG_ERROR(
                    sLogger,
                    ("failed to init pipeline", "plugin" + it->as_string() + "is not supported")("pipeline", Name()));
                return false;
            }
            if (!mGoPipelineWithInput.is_null() && mProcessorLine.empty()) {
                PushPluginToGoPipelines(plugin, mGoPipelineWithInput);
            } else {
                PushPluginToGoPipelines(plugin, mGoPipelineWithoutInput);
            }
        }
    }

    itr = table.find("aggregators");
    if (!itr->is_array()) {
        LOG_ERROR(sLogger, ("failed to init pipeline", "aggregators module is not of type array")("pipeline", Name()));
        return false;
    }
    if (itr->size() > 1) {
        LOG_ERROR(sLogger,
                  ("failed to init pipeline", "aggregators module has more than 1 plugin")("pipeline", Name()));
        return false;
    }
    for (size_t i = 0; i < itr->size(); ++i) {
        const Table& plugin = (*itr)[i];
        if (!plugin.is_object()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline",
                       "param aggregators[" + ToString(i) + "] is not of type object")("pipeline", Name()));
            return false;
        }
        Table* it = plugin.find("Type");
        if (!it->is_string()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline",
                       "param aggregators[" + ToString(i) + "].Type is not of type string")("pipeline", Name()));
            return false;
        }
        if (!PluginRegistry::GetInstance()->IsValidGoPlugin(it->as_string())) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline", "plugin" + it->as_string() + "is not supported")("pipeline", Name()));
            return false;
        }
        if (!mGoPipelineWithInput.is_null() && mProcessorLine.empty() && mGoPipelineWithoutInput.is_null()) {
            PushPluginToGoPipelines(plugin, mGoPipelineWithInput);
        } else {
            PushPluginToGoPipelines(plugin, mGoPipelineWithoutInput);
        }
    }

    itr = table.find("flushers");
    if (!itr) {
        LOG_ERROR(sLogger, ("failed to init pipeline", "mandatory flushers module is missing")("pipeline", Name()));
        return false;
    }
    if (!itr->is_array()) {
        LOG_ERROR(sLogger,
                  ("failed to init pipeline", "mandatory flushers module is not of type array")("pipeline", Name()));
        return false;
    }
    if (itr->empty()) {
        LOG_ERROR(sLogger, ("failed to init pipeline", "mandatory flushers module has no plugin")("pipeline", Name()));
        return false;
    }
    for (size_t i = 0; i < itr->size(); ++i) {
        const Table& plugin = (*itr)[i];
        if (!plugin.is_object()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline",
                       "param flushers[" + ToString(i) + "] is not of type object")("pipeline", Name()));
            return false;
        }
        Table* it = plugin.find("Type");
        if (!it->is_string()) {
            LOG_ERROR(sLogger,
                      ("failed to init pipeline",
                       "param flushers[" + ToString(i) + "].Type is not of type string")("pipeline", Name()));
            return false;
        }
        unique_ptr<FlusherInstance> flusher = PluginRegistry::GetInstance()->CreateFlusher(
            it->as_string(), it->as_string() + "/" + std::to_string(++pluginIndex));
        if (flusher) {
            if (!flusher->Init(plugin, mContext)) {
                return false;
            }
            mFlushers.emplace_back(std::move(flusher));
        } else {
            if (!PluginRegistry::GetInstance()->IsValidGoPlugin(it->as_string())) {
                LOG_ERROR(
                    sLogger,
                    ("failed to init pipeline", "plugin" + it->as_string() + "is not supported")("pipeline", Name()));
                return false;
            }
            // note: When input and flusher both have both C++ and Go plugin, and no processor is given, there are
            // actually 2 possible topology. For the sake of simplicity here, we choose the go dominated one.
            if (!mGoPipelineWithInput.is_null() && mProcessorLine.empty() && mGoPipelineWithoutInput.is_null()) {
                PushPluginToGoPipelines(plugin, mGoPipelineWithInput);
            } else {
                PushPluginToGoPipelines(plugin, mGoPipelineWithoutInput);
            }
        }
    }

    if (!LoadGoPipelines()) {
        LOG_ERROR(sLogger,
                  ("failed to init pipeline", "Go pipeline is invalid, see logtail_plugin.LOG for detail")("pipeline",
                                                                                                           Name()));
        return false;
    }

    mConfig = std::move(config);
    return true;
}

void Pipeline::Start() {
    // TODO: 应该保证指定时间内返回，如果无法返回，将配置放入startDisabled里
    for (const auto& flusher : mFlushers) {
        flusher->Start();
    }
    if (!mGoPipelineWithoutInput.is_null()) {
        // TODO: 加载该Go流水线
    }
    // TODO: 启用Process中改流水线对应的输入队列
    if (!mGoPipelineWithInput.is_null()) {
        // TODO: 加载该Go流水线
    }
    for (const auto& input : mInputs) {
        input->Start();
    }
}

void Pipeline::Process(PipelineEventGroup& logGroup) {
    for (auto& p : mProcessorLine) {
        p->Process(logGroup);
    }
}

void Pipeline::Stop(bool isRemoving) {
    // TODO: 应该保证指定时间内返回，如果无法返回，将配置放入stopDisabled里
    if (!mInputs.empty()) {
        for (const auto& input : mInputs) {
            input->Stop(isRemoving);
        }
    }
    if (!mRequiringSpecialStopOrder) {
        // TODO: 禁用Process中改流水线对应的输入队列
        if (!mGoPipelineWithInput.is_null()) {
            // TODO: 卸载该Go流水线
        }
    } else {
        if (!mGoPipelineWithInput.is_null()) {
            // TODO: 卸载该Go流水线
        }
        // TODO: 禁用Process中改流水线对应的输入队列
    }
    if (!mGoPipelineWithoutInput.is_null()) {
        // TODO: 卸载该Go流水线
    }
    for (const auto& flusher : mFlushers) {
        flusher->Stop(isRemoving);
    }
}

bool Pipeline::LoadGoPipelines() const {
    std::string project, logstore, region;
    LogstoreFeedBackKey logstoreKey;
    // TODO：目前mGoPipelineWithInput和mGoPipelineWithoutInput只会有1个，因此不存在问题。当Go
    // input支持向C++发送时，需要将下面的代码替换成批量原子Load。
    if (!mGoPipelineWithInput.is_null()) {
        if (!LogtailPlugin::GetInstance()->LoadPipeline(
                mName, mGoPipelineWithInput.serialize(), project, logstore, region, logstoreKey)) {
            return false;
        }
    }
    if (!mGoPipelineWithoutInput.is_null()) {
        if (!LogtailPlugin::GetInstance()->LoadPipeline(
                mName, mGoPipelineWithInput.serialize(), project, logstore, region, logstoreKey)) {
            return false;
        }
    }
    return true;
}

bool Pipeline::InitAndAddProcessor(std::unique_ptr<ProcessorInstance>&& processor, const PipelineConfig& config) {
    if (!processor) {
        LOG_ERROR(GetContext().GetLogger(),
                  ("CreateProcessor", ProcessorSplitRegexNative::Name())("Error", "Cannot find plugin"));
        return false;
    }
    ComponentConfig componentConfig(processor->Id(), config);
    if (!processor->Init(componentConfig, mContext)) {
        LOG_ERROR(GetContext().GetLogger(), ("InitProcessor", processor->Id())("Error", "Init failed"));
        return false;
    }
    mProcessorLine.emplace_back(std::move(processor));
    return true;
}

} // namespace logtail