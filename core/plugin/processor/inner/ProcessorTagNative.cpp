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

#include "plugin/processor/inner/ProcessorTagNative.h"

#include <vector>

#include "app_config/AppConfig.h"
#include "application/Application.h"
#include "common/Flags.h"
#include "common/ParamExtractor.h"
#include "pipeline/Pipeline.h"
#include "protobuf/sls/sls_logs.pb.h"
#ifdef __ENTERPRISE__
#include "config/provider/EnterpriseConfigProvider.h"
#endif

DECLARE_FLAG_STRING(ALIYUN_LOG_FILE_TAGS);

using namespace std;

namespace logtail {

const string ProcessorTagNative::sName = "processor_tag_native";

bool ProcessorTagNative::Init(const Json::Value& config) {
    mPipelineMetaTagKey = mContext->GetGlobalConfig().mPipelineMetaTagKey;
#ifdef __ENTERPRISE__
    mEnableAgentEnvMetaTagControl = mContext->GetGlobalConfig().mEnableAgentEnvMetaTagControl;
    mAgentEnvMetaTagKey = mContext->GetGlobalConfig().mAgentEnvMetaTagKey;
#endif
    return true;
}

void ProcessorTagNative::Process(PipelineEventGroup& logGroup) {
#ifdef __ENTERPRISE__
    string agentTag = EnterpriseConfigProvider::GetInstance()->GetUserDefinedIdSet();
    if (!agentTag.empty()) {
        auto sb = logGroup.GetSourceBuffer()->CopyString(agentTag);
        addTagIfRequired(logGroup, "AGENT_TAG", AGENT_TAG_DEFAULT_KEY, StringView(sb.data, sb.size));
    }
#endif

    if (!STRING_FLAG(ALIYUN_LOG_FILE_TAGS).empty()) {
        vector<sls_logs::LogTag>& fileTags = AppConfig::GetInstance()->GetFileTags();
        if (!fileTags.empty()) { // reloadable, so we must get it every time and copy value
            for (size_t i = 0; i < fileTags.size(); ++i) {
                logGroup.SetTag(fileTags[i].key(), fileTags[i].value());
            }
        }
    }

    if (mContext->GetPipeline().IsFlushingThroughGoPipeline()) {
        return;
    }

    addTagIfRequired(logGroup, "HOST_NAME", TagKeyDefaultValue[TagKey::HOST_NAME], LogFileProfiler::mHostname);
#ifndef __ENTERPRISE__
    addTagIfRequired(logGroup, "HOST_IP", HOST_IP_DEFAULT_KEY, LogFileProfiler::mIpAddr);
#endif

    static const vector<sls_logs::LogTag>& sEnvTags = AppConfig::GetInstance()->GetEnvTags();
    if (!sEnvTags.empty()) {
        for (size_t i = 0; i < sEnvTags.size(); ++i) {
#ifdef __ENTERPRISE__
            if (mEnableAgentEnvMetaTagControl) {
                auto envTagKey = sEnvTags[i].key();
                if (mAgentEnvMetaTagKey.find(envTagKey) != mAgentEnvMetaTagKey.end()) {
                    if (!mAgentEnvMetaTagKey[envTagKey].empty()) {
                        logGroup.SetTagNoCopy(mAgentEnvMetaTagKey[envTagKey], sEnvTags[i].value());
                    }
                }
                continue;
            }
#endif
            logGroup.SetTagNoCopy(sEnvTags[i].key(), sEnvTags[i].value());
        }
    }
}

bool ProcessorTagNative::IsSupportedEvent(const PipelineEventPtr& /*e*/) const {
    return true;
}

void ProcessorTagNative::addTagIfRequired(PipelineEventGroup& logGroup,
                                          const std::string& configKey,
                                          const std::string& defaultKey,
                                          const StringView& value) const {
    auto it = mPipelineMetaTagKey.find(configKey);
    if (it != mPipelineMetaTagKey.end()) {
        if (!it->second.empty()) {
            if (it->second == DEFAULT_CONFIG_TAG_KEY_VALUE) {
                logGroup.SetTagNoCopy(defaultKey, value);
            } else {
                logGroup.SetTagNoCopy(it->second, value);
            }
        }
        // emtpy value means not set
    } else {
        logGroup.SetTagNoCopy(defaultKey, value);
    }
}

} // namespace logtail
