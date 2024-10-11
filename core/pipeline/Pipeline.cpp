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

#include "common/Flags.h"
#include "common/ParamExtractor.h"
#include "flusher/FlusherSLS.h"
#include "go_pipeline/LogtailPlugin.h"
#include "plugin/PluginRegistry.h"
#include "processor/ProcessorParseApsaraNative.h"
#include "processor/daemon/LogProcess.h"
#include "processor/inner/ProcessorMergeMultilineLogNative.h"
#include "processor/inner/ProcessorParseContainerLogNative.h"
#include "processor/inner/ProcessorSplitLogStringNative.h"
#include "processor/inner/ProcessorSplitMultilineLogStringNative.h"
#include "processor/inner/ProcessorTagNative.h"

// for special treatment
#include "file_server/MultilineOptions.h"

DECLARE_FLAG_INT32(default_plugin_log_queue_size);

using namespace std;


namespace {
class AggregatorDefaultConfig {
public:
    static AggregatorDefaultConfig& Instance() {
        static AggregatorDefaultConfig instance;
        return instance;
    }

    Json::Value* GetJsonConfig() { return &aggregatorDefault; }

private:
    Json::Value aggregatorDefault;
    AggregatorDefaultConfig() { aggregatorDefault["Type"] = "aggregator_default"; }

    AggregatorDefaultConfig(AggregatorDefaultConfig const&) = delete;
    void operator=(AggregatorDefaultConfig const&) = delete;
};
} // namespace

namespace logtail {

void AddExtendedGlobalParamToGoPipeline(const Json::Value& extendedParams, Json::Value& pipeline) {
    if (!pipeline.isNull()) {
        Json::Value& global = pipeline["global"];
        for (auto itr = extendedParams.begin(); itr != extendedParams.end(); itr++) {
            global[itr.name()] = *itr;
        }
    }
}

bool Pipeline::handleInputFileProcessor(const logtail::InputFile* inputFile, const Config& config) {
    unique_ptr<ProcessorInstance> processor;
    Json::Value detail;
    if (config.mIsFirstProcessorJson || inputFile->mMultiline.mMode == MultilineOptions::Mode::JSON) {
        mContext.SetRequiringJsonReaderFlag(true);
        processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitLogStringNative::sName,
                                                                   GenNextPluginMeta(false));
        detail["SplitChar"] = Json::Value('\0');
        detail["AppendingLogPositionMeta"] = Json::Value(inputFile->mFileReader.mAppendingLogPositionMeta);
    } else if (inputFile->mMultiline.IsMultiline()) {
        processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitMultilineLogStringNative::sName,
                                                                   GenNextPluginMeta(false));
        detail["Mode"] = Json::Value("custom");
        detail["StartPattern"] = Json::Value(inputFile->mMultiline.mStartPattern);
        detail["ContinuePattern"] = Json::Value(inputFile->mMultiline.mContinuePattern);
        detail["EndPattern"] = Json::Value(inputFile->mMultiline.mEndPattern);
        detail["AppendingLogPositionMeta"] = Json::Value(inputFile->mFileReader.mAppendingLogPositionMeta);
        detail["IgnoringUnmatchWarning"] = Json::Value(inputFile->mMultiline.mIgnoringUnmatchWarning);
        if (inputFile->mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::DISCARD) {
            detail["UnmatchedContentTreatment"] = Json::Value("discard");
        } else if (inputFile->mMultiline.mUnmatchedContentTreatment
                   == MultilineOptions::UnmatchedContentTreatment::SINGLE_LINE) {
            detail["UnmatchedContentTreatment"] = Json::Value("single_line");
        }
    } else {
        processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitLogStringNative::sName,
                                                                   GenNextPluginMeta(false));
        detail["AppendingLogPositionMeta"] = Json::Value(inputFile->mFileReader.mAppendingLogPositionMeta);
    }
    if (!processor->Init(detail, mContext)) {
        // should not happen
        return false;
    }
    mProcessorLine.emplace_back(std::move(processor));
    return true;
}

bool Pipeline::handleInputContainerStdioProcessor(const logtail::InputContainerStdio* inputContainerStdio,
                                                  const Config& config) {
    unique_ptr<ProcessorInstance> processor;
    // ProcessorSplitLogStringNative
    {
        Json::Value detail;
        processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitLogStringNative::sName,
                                                                   GenNextPluginMeta(false));
        detail["SplitChar"] = Json::Value('\n');
        if (!processor->Init(detail, mContext)) {
            return false;
        }
        mProcessorLine.emplace_back(std::move(processor));
    }
    // ProcessorParseContainerLogNative
    {
        Json::Value detail;
        processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorParseContainerLogNative::sName,
                                                                   GenNextPluginMeta(false));
        detail["IgnoringStdout"] = Json::Value(inputContainerStdio->mIgnoringStdout);
        detail["IgnoringStderr"] = Json::Value(inputContainerStdio->mIgnoringStderr);
        detail["KeepingSourceWhenParseFail"] = Json::Value(inputContainerStdio->mKeepingSourceWhenParseFail);
        detail["IgnoreParseWarning"] = Json::Value(inputContainerStdio->mIgnoreParseWarning);
        if (!processor->Init(detail, mContext)) {
            return false;
        }
        mProcessorLine.emplace_back(std::move(processor));
    }
    // ProcessorMergeMultilineLogNative
    {
        Json::Value detail;
        processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorMergeMultilineLogNative::sName,
                                                                   GenNextPluginMeta(false));
        detail["MergeType"] = Json::Value("flag");
        if (!processor->Init(detail, mContext)) {
            return false;
        }
        mProcessorLine.emplace_back(std::move(processor));
    }
    if (inputContainerStdio->mMultiline.IsMultiline()) {
        Json::Value detail;
        if (config.mIsFirstProcessorJson || inputContainerStdio->mMultiline.mMode == MultilineOptions::Mode::JSON) {
            mContext.SetRequiringJsonReaderFlag(true);
            processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorSplitLogStringNative::sName,
                                                                       GenNextPluginMeta(false));
            detail["SplitChar"] = Json::Value('\0');
        } else {
            processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorMergeMultilineLogNative::sName,
                                                                       GenNextPluginMeta(false));
            detail["Mode"] = Json::Value("custom");
            detail["MergeType"] = Json::Value("regex");
            detail["StartPattern"] = Json::Value(inputContainerStdio->mMultiline.mStartPattern);
            detail["ContinuePattern"] = Json::Value(inputContainerStdio->mMultiline.mContinuePattern);
            detail["EndPattern"] = Json::Value(inputContainerStdio->mMultiline.mEndPattern);
            detail["IgnoringUnmatchWarning"] = Json::Value(inputContainerStdio->mMultiline.mIgnoringUnmatchWarning);
            if (inputContainerStdio->mMultiline.mUnmatchedContentTreatment
                == MultilineOptions::UnmatchedContentTreatment::DISCARD) {
                detail["UnmatchedContentTreatment"] = Json::Value("discard");
            } else if (inputContainerStdio->mMultiline.mUnmatchedContentTreatment
                       == MultilineOptions::UnmatchedContentTreatment::SINGLE_LINE) {
                detail["UnmatchedContentTreatment"] = Json::Value("single_line");
            }
        }
        if (!processor->Init(detail, mContext)) {
            return false;
        }
        mProcessorLine.emplace_back(std::move(processor));
    }
    return true;
}

bool Pipeline::Init(Config&& config) {
    mName = config.mName;
    mConfig = std::move(config.mDetail);
    mContext.SetConfigName(mName);
    mContext.SetCreateTime(config.mCreateTime);
    mContext.SetPipeline(*this);

    // for special treatment below
    const InputFile* inputFile = nullptr;
    const InputContainerStdio* inputContainerStdio = nullptr;

    // to send alarm and init MetricsRecord before flusherSLS is built, a temporary object is made, which will be overriden shortly after.
    unique_ptr<FlusherSLS> SLSTmp = unique_ptr<FlusherSLS>(new FlusherSLS());
    if (!config.mProject.empty()) {
        SLSTmp->mProject = config.mProject;
        SLSTmp->mLogstore = config.mLogstore;
        SLSTmp->mRegion = config.mRegion;
        mContext.SetSLSInfo(SLSTmp.get());
    }

    mPluginID.store(0);
    for (size_t i = 0; i < config.mInputs.size(); ++i) {
        const Json::Value& detail = *config.mInputs[i];
        string pluginType = detail["Type"].asString();
        unique_ptr<InputInstance> input
            = PluginRegistry::GetInstance()->CreateInput(pluginType, GenNextPluginMeta(false));
        if (input) {
            Json::Value optionalGoPipeline;
            if (!input->Init(detail, mContext, optionalGoPipeline)) {
                return false;
            }
            mInputs.emplace_back(std::move(input));
            if (!optionalGoPipeline.isNull()) {
                MergeGoPipeline(optionalGoPipeline, mGoPipelineWithInput);
            }
            // for special treatment below
            if (pluginType == InputFile::sName) {
                inputFile = static_cast<const InputFile*>(mInputs[0]->GetPlugin());
            } else if (pluginType == InputContainerStdio::sName) {
                inputContainerStdio = static_cast<const InputContainerStdio*>(mInputs[0]->GetPlugin());
            }
        } else {
            AddPluginToGoPipeline(pluginType, detail, "inputs", mGoPipelineWithInput);
        }
        ++mPluginCntMap["inputs"][pluginType];
    }

    if (config.IsProcessRunnerInvolved()) {
        Json::Value detail;
        unique_ptr<ProcessorInstance> processor
            = PluginRegistry::GetInstance()->CreateProcessor(ProcessorTagNative::sName, GenNextPluginMeta(false));
        if (!processor->Init(detail, mContext)) {
            // should not happen
            return false;
        }
        mProcessorLine.emplace_back(std::move(processor));
    }

    // add log split processor for input_file
    if (inputFile && !handleInputFileProcessor(inputFile, config)) {
        return false;
    }
    if (inputContainerStdio && !handleInputContainerStdioProcessor(inputContainerStdio, config)) {
        return false;
    }

    for (size_t i = 0; i < config.mProcessors.size(); ++i) {
        const Json::Value& detail = *config.mProcessors[i];
        string pluginType = detail["Type"].asString();
        unique_ptr<ProcessorInstance> processor
            = PluginRegistry::GetInstance()->CreateProcessor(pluginType, GenNextPluginMeta(false));
        if (processor) {
            if (!processor->Init(detail, mContext)) {
                return false;
            }
            mProcessorLine.emplace_back(std::move(processor));
            // for special treatment of topicformat in apsara mode
            if (i == 0 && pluginType == ProcessorParseApsaraNative::sName) {
                mContext.SetIsFirstProcessorApsaraFlag(true);
            }
        } else {
            if (ShouldAddPluginToGoPipelineWithInput()) {
                AddPluginToGoPipeline(pluginType, detail, "processors", mGoPipelineWithInput);
            } else {
                AddPluginToGoPipeline(pluginType, detail, "processors", mGoPipelineWithoutInput);
            }
        }
        ++mPluginCntMap["processors"][pluginType];
    }

    if (config.mAggregators.empty() && config.IsFlushingThroughGoPipelineExisted()) {
        // an aggregator_default plugin will be add to go pipeline when mAggregators is empty and need to send go data
        // to cpp flusher.
        config.mAggregators.push_back(AggregatorDefaultConfig::Instance().GetJsonConfig());
    }
    for (size_t i = 0; i < config.mAggregators.size(); ++i) {
        const Json::Value& detail = *config.mAggregators[i];
        string pluginType = detail["Type"].asString();
        GenNextPluginMeta(false);
        if (ShouldAddPluginToGoPipelineWithInput()) {
            AddPluginToGoPipeline(pluginType, detail, "aggregators", mGoPipelineWithInput);
        } else {
            AddPluginToGoPipeline(pluginType, detail, "aggregators", mGoPipelineWithoutInput);
        }
        ++mPluginCntMap["aggregators"][pluginType];
    }

    for (size_t i = 0; i < config.mFlushers.size(); ++i) {
        const Json::Value& detail = *config.mFlushers[i];
        string pluginType = detail["Type"].asString();
        unique_ptr<FlusherInstance> flusher
            = PluginRegistry::GetInstance()->CreateFlusher(pluginType, GenNextPluginMeta(false));
        if (flusher) {
            Json::Value optionalGoPipeline;
            if (!flusher->Init(detail, mContext, optionalGoPipeline)) {
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
            if (pluginType == FlusherSLS::sName) {
                mContext.SetSLSInfo(static_cast<const FlusherSLS*>(mFlushers[0]->GetPlugin()));
            }
        } else {
            if (ShouldAddPluginToGoPipelineWithInput()) {
                AddPluginToGoPipeline(pluginType, detail, "flushers", mGoPipelineWithInput);
            } else {
                AddPluginToGoPipeline(pluginType, detail, "flushers", mGoPipelineWithoutInput);
            }
        }
        ++mPluginCntMap["flushers"][pluginType];
    }

    for (size_t i = 0; i < config.mExtensions.size(); ++i) {
        const Json::Value& detail = *config.mExtensions[i];
        string pluginType = detail["Type"].asString();
        GenNextPluginMeta(false);
        if (!mGoPipelineWithInput.isNull()) {
            AddPluginToGoPipeline(pluginType, detail, "extensions", mGoPipelineWithInput);
        }
        if (!mGoPipelineWithoutInput.isNull()) {
            AddPluginToGoPipeline(pluginType, detail, "extensions", mGoPipelineWithoutInput);
        }
        ++mPluginCntMap["extensions"][pluginType];
    }

    // global module must be initialized at last, since native input or flusher plugin may generate global param in Go
    // pipeline, which should be overriden by explicitly provided global module.
    if (config.mGlobal) {
        Json::Value extendedParams;
        if (!mContext.InitGlobalConfig(*config.mGlobal, extendedParams)) {
            return false;
        }
        AddExtendedGlobalParamToGoPipeline(extendedParams, mGoPipelineWithInput);
        AddExtendedGlobalParamToGoPipeline(extendedParams, mGoPipelineWithoutInput);
    }
    CopyNativeGlobalParamToGoPipeline(mGoPipelineWithInput);
    CopyNativeGlobalParamToGoPipeline(mGoPipelineWithoutInput);

    // mandatory override global.DefaultLogQueueSize in Go pipeline when input_file and Go processing coexist.
    if ((inputFile != nullptr || inputContainerStdio != nullptr) && IsFlushingThroughGoPipeline()) {
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
        if (IsFlushingThroughGoPipeline()) {
            PARAM_ERROR_RETURN(mContext.GetLogger(),
                               mContext.GetAlarm(),
                               "exactly once enabled when not in native mode exist",
                               noModule,
                               mName,
                               mContext.GetProjectName(),
                               mContext.GetLogstoreName(),
                               mContext.GetRegion());
        }
        // flusher_sls is guaranteed to exist here.
        if (mContext.GetSLSInfo()->mBatch.mMergeType != FlusherSLS::Batch::MergeType::TOPIC) {
            PARAM_ERROR_RETURN(mContext.GetLogger(),
                               mContext.GetAlarm(),
                               "exactly once enabled when flusher_sls.MergeType is not topic",
                               noModule,
                               mName,
                               mContext.GetProjectName(),
                               mContext.GetLogstoreName(),
                               mContext.GetRegion());
        }
    }

#ifndef APSARA_UNIT_TEST_MAIN
    if (!LoadGoPipelines()) {
        return false;
    }
#endif

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
    LOG_INFO(sLogger, ("pipeline start", "succeeded")("config", mName));
}

void Pipeline::Process(vector<PipelineEventGroup>& logGroupList) {
    for (auto& p : mProcessorLine) {
        p->Process(logGroupList);
    }
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
    LOG_INFO(sLogger, ("pipeline stop", "succeeded")("config", mName));
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

std::string Pipeline::GenPluginTypeWithID(std::string pluginType, std::string pluginID) {
    return pluginType + "/" + pluginID;
}

// Rule: pluginTypeWithID=pluginType/pluginID#pluginPriority.
void Pipeline::AddPluginToGoPipeline(const string& pluginType,
                                     const Json::Value& plugin,
                                     const string& module,
                                     Json::Value& dst) {
    Json::Value res(Json::objectValue), detail = plugin;
    detail.removeMember("Type");
    res["type"] = GenPluginTypeWithID(pluginType, GetNowPluginID());
    res["detail"] = detail;
    dst[module].append(res);
}

void Pipeline::CopyNativeGlobalParamToGoPipeline(Json::Value& pipeline) {
    if (!pipeline.isNull()) {
        Json::Value& global = pipeline["global"];
        global["EnableTimestampNanosecond"] = mContext.GetGlobalConfig().mEnableTimestampNanosecond;
        global["UsingOldContentTag"] = mContext.GetGlobalConfig().mUsingOldContentTag;
    }
}

bool Pipeline::LoadGoPipelines() const {
    // TODO：将下面的代码替换成批量原子Load。
    // note:
    // 目前按照从后往前顺序加载，即便without成功with失败导致without残留在插件系统中，也不会有太大的问题，但最好改成原子的。
    if (!mGoPipelineWithoutInput.isNull()) {
        string content = mGoPipelineWithoutInput.toStyledString();
        if (!LogtailPlugin::GetInstance()->LoadPipeline(mName + "/2",
                                                        content,
                                                        mContext.GetProjectName(),
                                                        mContext.GetLogstoreName(),
                                                        mContext.GetRegion(),
                                                        mContext.GetLogstoreKey())) {
            LOG_ERROR(mContext.GetLogger(),
                      ("failed to init pipeline", "Go pipeline is invalid, see logtail_plugin.LOG for detail")(
                          "Go pipeline num", "2")("Go pipeline content", content)("config", mName));
            LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
                                                   "Go pipeline is invalid, content: " + content + ", config: " + mName,
                                                   mContext.GetProjectName(),
                                                   mContext.GetLogstoreName(),
                                                   mContext.GetRegion());
            return false;
        }
    }
    if (!mGoPipelineWithInput.isNull()) {
        string content = mGoPipelineWithInput.toStyledString();
        if (!LogtailPlugin::GetInstance()->LoadPipeline(mName + "/1",
                                                        content,
                                                        mContext.GetProjectName(),
                                                        mContext.GetLogstoreName(),
                                                        mContext.GetRegion(),
                                                        mContext.GetLogstoreKey())) {
            LOG_ERROR(mContext.GetLogger(),
                      ("failed to init pipeline", "Go pipeline is invalid, see logtail_plugin.LOG for detail")(
                          "Go pipeline num", "1")("Go pipeline content", content)("config", mName));
            LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
                                                   "Go pipeline is invalid, content: " + content + ", config: " + mName,
                                                   mContext.GetProjectName(),
                                                   mContext.GetLogstoreName(),
                                                   mContext.GetRegion());
            return false;
        }
    }
    return true;
}

std::string Pipeline::GetNowPluginID() {
    return std::to_string(mPluginID.load());
}

PluginInstance::PluginMeta Pipeline::GenNextPluginMeta(bool lastOne) {
    mPluginID.fetch_add(1);
    int32_t childNodeID = mPluginID.load();
    if (lastOne) {
        childNodeID = -1;
    } else {
        childNodeID += 1;
    }
    return PluginInstance::PluginMeta(
        std::to_string(mPluginID.load()), std::to_string(mPluginID.load()), std::to_string(childNodeID));
}

} // namespace logtail
