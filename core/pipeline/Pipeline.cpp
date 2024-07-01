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

#include "batch/TimeoutFlushManager.h"
#include "common/Flags.h"
#include "common/ParamExtractor.h"
#include "flusher/FlusherSLS.h"
#include "go_pipeline/LogtailPlugin.h"
#include "input/InputFeedbackInterfaceRegistry.h"
#include "input/InputPrometheus.h"
#include "plugin/PluginRegistry.h"
#include "processor/ProcessorParseApsaraNative.h"
#include "queue/ProcessQueueManager.h"
#include "queue/QueueKeyManager.h"

DECLARE_FLAG_INT32(default_plugin_log_queue_size);

using namespace std;

namespace logtail {

void AddExtendedGlobalParamToGoPipeline(const Json::Value& extendedParams, Json::Value& pipeline) {
    if (!pipeline.isNull()) {
        Json::Value& global = pipeline["global"];
        for (auto itr = extendedParams.begin(); itr != extendedParams.end(); itr++) {
            global[itr.name()] = *itr;
        }
    }
}

bool Pipeline::Init(Config&& config) {
    mName = config.mName;
    mConfig = std::move(config.mDetail);
    mContext.SetConfigName(mName);
    mContext.SetCreateTime(config.mCreateTime);
    mContext.SetPipeline(*this);
    mContext.SetIsFirstProcessorJsonFlag(config.mIsFirstProcessorJson);

    // for special treatment below
    const InputFile* inputFile = nullptr;
    const InputContainerStdio* inputContainerStdio = nullptr;
    bool hasFlusherSLS = false;

#ifdef __ENTERPRISE__
    // to send alarm before flusherSLS is built, a temporary object is made, which will be overriden shortly after.
    unique_ptr<FlusherSLS> SLSTmp = unique_ptr<FlusherSLS>(new FlusherSLS());
    SLSTmp->mProject = config.mProject;
    SLSTmp->mLogstore = config.mLogstore;
    SLSTmp->mRegion = config.mRegion;
    mContext.SetSLSInfo(SLSTmp.get());
#endif

    uint32_t pluginIndex = 0;
    for (size_t i = 0; i < config.mInputs.size(); ++i) {
        const Json::Value& detail = *config.mInputs[i];
        string name = detail["Type"].asString();
        unique_ptr<InputInstance> input = PluginRegistry::GetInstance()->CreateInput(name, to_string(++pluginIndex));
        if (input) {
            Json::Value optionalGoPipeline;
            if (!input->Init(detail, mContext, pluginIndex, i, optionalGoPipeline)) {
                return false;
            }
            mInputs.emplace_back(std::move(input));
            if (!optionalGoPipeline.isNull()) {
                MergeGoPipeline(optionalGoPipeline, mGoPipelineWithInput);
            }
            // for special treatment below
            if (name == InputFile::sName) {
                inputFile = static_cast<const InputFile*>(mInputs[0]->GetPlugin());
            } else if (name == InputContainerStdio::sName) {
                inputContainerStdio = static_cast<const InputContainerStdio*>(mInputs[0]->GetPlugin());
            }
        } else {
            AddPluginToGoPipeline(detail, "inputs", mGoPipelineWithInput);
        }
        ++mPluginCntMap["inputs"][name];
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
        ++mPluginCntMap["processors"][name];
    }

    for (auto detail : config.mAggregators) {
        if (ShouldAddPluginToGoPipelineWithInput()) {
            AddPluginToGoPipeline(*detail, "aggregators", mGoPipelineWithInput);
        } else {
            AddPluginToGoPipeline(*detail, "aggregators", mGoPipelineWithoutInput);
        }
        ++mPluginCntMap["aggregators"][(*detail)["Type"].asString()];
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
                hasFlusherSLS = true;
                mContext.SetSLSInfo(static_cast<const FlusherSLS*>(mFlushers.back()->GetPlugin()));
            }
        } else {
            if (ShouldAddPluginToGoPipelineWithInput()) {
                AddPluginToGoPipeline(*detail, "flushers", mGoPipelineWithInput);
            } else {
                AddPluginToGoPipeline(*detail, "flushers", mGoPipelineWithoutInput);
            }
        }
        ++mPluginCntMap["flushers"][name];
    }

    for (auto detail : config.mExtensions) {
        if (!mGoPipelineWithInput.isNull()) {
            AddPluginToGoPipeline(*detail, "extensions", mGoPipelineWithInput);
        }
        if (!mGoPipelineWithoutInput.isNull()) {
            AddPluginToGoPipeline(*detail, "extensions", mGoPipelineWithoutInput);
        }
        ++mPluginCntMap["extensions"][(*detail)["Type"].asString()];
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

    // special treatment for exactly once
    if (inputFile && inputFile->mExactlyOnceConcurrency > 0) {
        if (mInputs.size() > 1) {
            PARAM_ERROR_RETURN(mContext.GetLogger(),
                               mContext.GetAlarm(),
                               "exactly once enabled when input other than input_file is given",
                               noModule,
                               mName,
                               mContext.GetProjectName(),
                               mContext.GetLogstoreName(),
                               mContext.GetRegion());
        }
        if (mFlushers.size() > 1 || !hasFlusherSLS) {
            PARAM_ERROR_RETURN(mContext.GetLogger(),
                               mContext.GetAlarm(),
                               "exactly once enabled when flusher other than flusher_sls is given",
                               noModule,
                               mName,
                               mContext.GetProjectName(),
                               mContext.GetLogstoreName(),
                               mContext.GetRegion());
        }
        if (IsFlushingThroughGoPipeline()) {
            PARAM_ERROR_RETURN(mContext.GetLogger(),
                               mContext.GetAlarm(),
                               "exactly once enabled when not in native mode",
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

    // Process queue, not generated when exactly once is enabled
    if (!inputFile || inputFile->mExactlyOnceConcurrency == 0) {
        if (mContext.GetProcessQueueKey() == -1) {
            mContext.SetProcessQueueKey(QueueKeyManager::GetInstance()->GetKey(mName));
        }
        uint32_t priority = mContext.GetGlobalConfig().mProcessPriority == 0
            ? ProcessQueueManager::sMaxPriority
            : mContext.GetGlobalConfig().mProcessPriority - 1;
        ProcessQueueManager::GetInstance()->CreateOrUpdateQueue(mContext.GetProcessQueueKey(), priority);

        unordered_set<FeedbackInterface*> feedbackSet;
        for (const auto& input : mInputs) {
            FeedbackInterface* feedback
                = InputFeedbackInterfaceRegistry::GetInstance()->GetFeedbackInterface(input->Name());
            if (feedback != nullptr) {
                feedbackSet.insert(feedback);
            }
        }
        vector<FeedbackInterface*> feedbacks(feedbackSet.begin(), feedbackSet.end());
        ProcessQueueManager::GetInstance()->SetFeedbackInterface(mContext.GetProcessQueueKey(), feedbacks);

        vector<SingleLogstoreSenderManager<SenderQueueParam>*> senderQueues;
        for (const auto& flusher : mFlushers) {
            senderQueues.push_back(flusher->GetSenderQueue());
        }
        ProcessQueueManager::GetInstance()->SetDownStreamQueues(mContext.GetProcessQueueKey(), senderQueues);
    }

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

void Pipeline::Process(vector<PipelineEventGroup>& logGroupList, size_t inputIndex) {
    for (auto& p : mInputs[inputIndex]->GetInnerProcessors()) {
        p->Process(logGroupList);
    }
    for (auto& p : mProcessorLine) {
        p->Process(logGroupList);
    }
}

void Pipeline::Send(vector<PipelineEventGroup>&& groupList) {
    for (auto& group : groupList) {
        // TODO: special treatment for demo
        if (mInputs[0]->Name() == "input_ebpf_sockettraceprobe_observer") {
            if (!group.GetEvents().empty()) {
                if (group.GetEvents()[0].Is<MetricEvent>()) {
                    mFlushers[0]->Send(std::move(group));
                } else if (group.GetEvents()[0].Is<SpanEvent>()) {
                    mFlushers[1]->Send(std::move(group));
                }
            }
            continue;
        }
        // TODO: support route
        for (size_t i = 0; i < mFlushers.size(); ++i) {
            if (i + 1 != mFlushers.size()) {
                mFlushers[i]->Send(group.Copy());
            } else {
                mFlushers[i]->Send(std::move(group));
            }
        }
    }
}

void Pipeline::FlushBatch() {
    for (auto& flusher : mFlushers) {
        flusher->FlushAll();
    }
    TimeoutFlushManager::GetInstance()->ClearRecords(mName);
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

    if (!isRemoving) {
        FlushBatch();
    }

    if (!mGoPipelineWithoutInput.isNull()) {
        // TODO: 卸载该Go流水线
    }

    for (const auto& flusher : mFlushers) {
        flusher->Stop(isRemoving);
    }

    LOG_INFO(sLogger, ("pipeline stop", "succeeded")("config", mName));
}

void Pipeline::RemoveProcessQueue() const {
    ProcessQueueManager::GetInstance()->DeleteQueue(mContext.GetProcessQueueKey());
    QueueKeyManager::GetInstance()->RemoveKey(mContext.GetProcessQueueKey());
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

} // namespace logtail
