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

#include <cstdint>
#include <utility>

#include "common/ParamExtractor.h"
#include "flusher/FlusherSLS.h"
#include "plugin/PluginRegistry.h"
#include "processor/daemon/LogProcess.h"
#include "processor/ProcessorSplitLogStringNative.h"
#include "processor/ProcessorSplitRegexNative.h"
#include "processor/ProcessorParseApsaraNative.h"

// for special treatment
#include "input/InputFile.h"
#include "file_server/MultilineOptions.h"

DECLARE_FLAG_INT32(default_plugin_log_queue_size);

using namespace std;

namespace logtail {

bool Pipeline::Init(NewConfig&& config) {
    mName = config.mName;
    mConfig = std::move(config.mDetail);
    mContext.SetConfigName(mName);
    mContext.SetCreateTime(config.mCreateTime);
    mContext.SetPipeline(*this);
    mContext.SetIsFirstProcessorJsonFlag(config.mIsFirstProcessorJson);

    // for special treatment below
    const InputFile* inputFile = nullptr;

    int16_t pluginIndex = 0;
    for (auto detail : config.mInputs) {
        string name = (*detail)["Type"].asString();
        unique_ptr<InputInstance> input = PluginRegistry::GetInstance()->CreateInput(name, to_string(++pluginIndex));
        if (input) {
            Json::Value optionalGoPipeline;
            if (!input->Init(*detail, mContext, optionalGoPipeline)) {
                return false;
            }
            mInputs.emplace_back(std::move(input));
            if (!optionalGoPipeline.isNull()) {
                MergeGoPipeline(optionalGoPipeline, mGoPipelineWithInput);
            }
            // for special treatment below
            if (name == "input_file") {
                inputFile = static_cast<const InputFile*>(mInputs[0]->GetPlugin());
            }
        } else {
            AddPluginToGoPipeline(*detail, "inputs", mGoPipelineWithInput);
        }
    }

    // add log split processor for input_file
    if (inputFile) {
        unique_ptr<ProcessorInstance> processor;
        Json::Value detail;
        if (config.mIsFirstProcessorJson || inputFile->mMultiline.mMode == MultilineOptions::Mode::JSON) {
            processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitLogStringNative::sName,
                                                                       to_string(++pluginIndex));
            detail["SplitChar"] = Json::Value('\0');
            detail["AppendingLogPositionMeta"] = Json::Value(inputFile->mFileReader.mAppendingLogPositionMeta);
        } else if (inputFile->mMultiline.IsMultiline()) {
            processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitRegexNative::sName,
                                                                       to_string(++pluginIndex));
            detail["StartPattern"] = Json::Value(inputFile->mMultiline.mStartPattern);
            detail["ContinuePattern"] = Json::Value(inputFile->mMultiline.mContinuePattern);
            detail["EndPattern"] = Json::Value(inputFile->mMultiline.mEndPattern);
            detail["AppendingLogPositionMeta"] = Json::Value(inputFile->mFileReader.mAppendingLogPositionMeta);
        } else {
            processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitLogStringNative::sName,
                                                                       to_string(++pluginIndex));
            detail["AppendingLogPositionMeta"] = Json::Value(inputFile->mFileReader.mAppendingLogPositionMeta);
        }
        if (!processor->Init(detail, mContext)) {
            return false;
        }
        mProcessorLine.emplace_back(std::move(processor));
    }

    for (size_t i = 0; i < config.mProcessors.size(); ++i) {
        string name = (*config.mProcessors[i])["Type"].asString();
        unique_ptr<ProcessorInstance> processor
            = PluginRegistry::GetInstance()->CreateProcessor(name, to_string(++pluginIndex));
        if (processor) {
            if (!processor->Init(*config.mProcessors[i], mContext)) {
                return false;
            }
            mProcessorLine.emplace_back(std::move(processor));
            // for special treatment of topicformat in apsara mode
            if (i == 0 && name == ProcessorParseApsaraNative::sName) {
                mContext.SetIsFirstProcessorApsaraFlag(true);
            }
        } else {
            if (ShouldAddPluginToGoPipelineWithInput()) {
                AddPluginToGoPipeline(*config.mProcessors[i], "processors", mGoPipelineWithInput);
            } else {
                AddPluginToGoPipeline(*config.mProcessors[i], "processors", mGoPipelineWithoutInput);
            }
        }
    }

    for (auto detail : config.mAggregators) {
        if (ShouldAddPluginToGoPipelineWithInput()) {
            AddPluginToGoPipeline(*detail, "aggregators", mGoPipelineWithInput);
        } else {
            AddPluginToGoPipeline(*detail, "aggregators", mGoPipelineWithoutInput);
        }
    }

    for (auto detail : config.mFlushers) {
        string name = (*detail)["Type"].asString();
        unique_ptr<FlusherInstance> flusher
            = PluginRegistry::GetInstance()->CreateFlusher(name, to_string(++pluginIndex));
        if (flusher) {
            Json::Value optionalGoPipeline;
            if (!flusher->Init(*detail, mContext, optionalGoPipeline)) {
                return false;
            }
            mFlushers.emplace_back(std::move(flusher));
            if (!optionalGoPipeline.isNull() && config.ShouldNativeFlusherConnectedByGoPipeline()) {
                if (ShouldAddPluginToGoPipelineWithInput()) {
                    MergeGoPipeline(optionalGoPipeline, mGoPipelineWithInput);
                } else {
                    MergeGoPipeline(optionalGoPipeline, mGoPipelineWithoutInput);
                }
            }
            if (name == FlusherSLS::sName) {
                mContext.SetSLSInfo(static_cast<const FlusherSLS*>(mFlushers[0]->GetPlugin()));
            }
        } else {
            if (ShouldAddPluginToGoPipelineWithInput()) {
                AddPluginToGoPipeline(*detail, "flushers", mGoPipelineWithInput);
            } else {
                AddPluginToGoPipeline(*detail, "flushers", mGoPipelineWithoutInput);
            }
        }
    }

    for (auto detail : config.mExtensions) {
        if (!mGoPipelineWithInput.isNull()) {
            AddPluginToGoPipeline(*detail, "extensions", mGoPipelineWithInput);
        }
        if (!mGoPipelineWithoutInput.isNull()) {
            AddPluginToGoPipeline(*detail, "extensions", mGoPipelineWithoutInput);
        }
    }

    // global module must be initialized at last, since native input or flusher plugin may generate global param in Go
    // pipeline, which should be overriden by explicitly provided global module.
    if (config.mGlobal) {
        Json::Value nonNativeParams;
        if (!mContext.InitGlobalConfig(*config.mGlobal, nonNativeParams)) {
            return false;
        }
        if (!mGoPipelineWithInput.isNull()) {
            Json::Value& global = mGoPipelineWithInput["global"];
            for (auto itr = nonNativeParams.begin(); itr != nonNativeParams.end(); itr++) {
                global[itr.name()] = *itr;
            }
            global["EnableTimestampNanosecond"] = mContext.GetGlobalConfig().mEnableTimestampNanosecond;
            global["UsingOldContentTag"] = mContext.GetGlobalConfig().mUsingOldContentTag;
        }
        if (!mGoPipelineWithoutInput.isNull()) {
            Json::Value& global = mGoPipelineWithoutInput["global"];
            for (auto itr = nonNativeParams.begin(); itr != nonNativeParams.end(); itr++) {
                global[itr.name()] = *itr;
            }
            global["EnableTimestampNanosecond"] = mContext.GetGlobalConfig().mEnableTimestampNanosecond;
            global["UsingOldContentTag"] = mContext.GetGlobalConfig().mUsingOldContentTag;
        }
    }

    // mandatory override global.DefaultLogQueueSize in Go pipeline when input_file and Go processing coexist.
    if (inputFile && !mGoPipelineWithoutInput.isNull()) {
        mGoPipelineWithoutInput["global"]["DefaultLogQueueSize"]
            = Json::Value(INT32_FLAG(default_plugin_log_queue_size));
    }

    // special treatment
    const GlobalConfig& global = mContext.GetGlobalConfig();
    if (global.mProcessPriority > 0) {
        LogProcess::GetInstance()->SetPriorityWithHoldOn(mContext.GetLogstoreKey(), global.mProcessPriority);
    } else {
        LogProcess::GetInstance()->DeletePriorityWithHoldOn(mContext.GetLogstoreKey());
    }

    if (inputFile && inputFile->mExactlyOnceConcurrency > 0) {
        if (!mGoPipelineWithoutInput.isNull()) {
            PARAM_ERROR_RETURN(mContext.GetLogger(), "exactly once enabled when not in native mode exist", noModule, mName);
        }
        // flusher_sls is guaranteed to exist here.
        if (mContext.GetSLSInfo()->mBatch.mMergeType != FlusherSLS::Batch::MergeType::TOPIC) {
            PARAM_ERROR_RETURN(
                mContext.GetLogger(), "exactly once enabled when flusher_sls.MergeType is not topic", noModule, mName);
        }
    }

    // if (!LoadGoPipelines()) {
    //     LOG_ERROR(
    //         sLogger,
    //         ("failed to init pipeline", "Go pipeline is invalid, see logtail_plugin.LOG for detail")("config",
    //         Name()));
    //     return false;
    // }

    return true;
}
void Pipeline::Start() {
    // TODO: 应该保证指定时间内返回，如果无法返回，将配置放入startDisabled里
    for (const auto& flusher : mFlushers) {
        flusher->Start();
    }
    if (!mGoPipelineWithoutInput.isNull()) {
        // TODO: 加载该Go流水线
    }
    // TODO: 启用Process中改流水线对应的输入队列
    if (!mGoPipelineWithInput.isNull()) {
        // TODO: 加载该Go流水线
    }
    for (const auto& input : mInputs) {
        input->Start();
    }
}

void Pipeline::Process(PipelineEventGroup&& logGroup, vector<PipelineEventGroup>& logGroupList) {
    for (auto& p : mProcessorLine) {
        p->Process(logGroup);
    }
    logGroupList.emplace_back(std::move(logGroup));
}

void Pipeline::Stop(bool isRemoving) {
    // TODO: 应该保证指定时间内返回，如果无法返回，将配置放入stopDisabled里
    for (const auto& input : mInputs) {
        input->Stop(isRemoving);
    }
    if (!mGoPipelineWithInput.isNull()) {
        // TODO: 卸载该Go流水线
    }
    // TODO: 禁用Process中改流水线对应的输入队列
    if (!mGoPipelineWithoutInput.isNull()) {
        // TODO: 卸载该Go流水线
    }
    for (const auto& flusher : mFlushers) {
        flusher->Stop(isRemoving);
    }
}

void Pipeline::MergeGoPipeline(const Json::Value& src, Json::Value& dst) {
    for (auto itr = src.begin(); itr != src.end(); ++itr) {
        if (itr->isArray()) {
            Json::Value& module = dst[itr.name()];
            for (auto it = itr->begin(); it != itr->end(); ++it) {
                module.append(*it);
            }
        } else if (itr->isObject()) {
            Json::Value& module = dst[itr.name()];
            for (auto it = itr->begin(); it != itr->end(); ++it) {
                module[it.name()] = *it;
            }
        }
    }
}

void Pipeline::AddPluginToGoPipeline(const Json::Value& plugin, const string& module, Json::Value& dst) {
    Json::Value res(Json::objectValue), detail = plugin;
    detail.removeMember("Type");
    res["type"] = plugin["Type"];
    res["detail"] = detail;
    dst[module].append(res);
}

} // namespace logtail