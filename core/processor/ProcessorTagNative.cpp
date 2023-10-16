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

#include "processor/ProcessorTagNative.h"

#include "app_config/AppConfig.h"
#include "common/Constants.h"
#include "common/FileSystemUtil.h"
#include "reader/LogFileReader.h"
#include "plugin/instance/ProcessorInstance.h"
#include "monitor/MetricConstants.h"

namespace logtail {
const std::string ProcessorTagNative::sName = "processor_tag_native";

bool ProcessorTagNative::Init(const ComponentConfig& componentConfig) {
    const Config& config = componentConfig.GetConfig();
    mPluginProcessFlag = config.mPluginProcessFlag;
    return true;
}

void ProcessorTagNative::Process(PipelineEventGroup& logGroup) {
    // add file tags (like env tags but support reload)
    if (!STRING_FLAG(ALIYUN_LOG_FILE_TAGS).empty()) {
        std::vector<sls_logs::LogTag>& fileTags = ConfigManager::GetInstance()->GetFileTags();
        if (!fileTags.empty()) { // reloadable, so we must get it every time and copy value
            for (size_t i = 0; i < fileTags.size(); ++i) {
                logGroup.SetTag(fileTags[i].key(), fileTags[i].value());
            }
        }
    }

    // __path__
    const logtail::StringView& filePath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH);
    if (!filePath.empty()) {
        logGroup.SetTagNoCopy(LOG_RESERVED_KEY_PATH, filePath.substr(0, 511));
    }

    // __user_defined_id__
    const logtail::StringView& agent_tag = logGroup.GetMetadata(EventGroupMetaKey::AGENT_TAG);
    if (!agent_tag.empty()) {
        logGroup.SetTagNoCopy(LOG_RESERVED_KEY_USER_DEFINED_ID, agent_tag.substr(0, 99));
    }

    if (mPluginProcessFlag) {
        return;
    }

    // __hostname__
    logGroup.SetTagNoCopy(LOG_RESERVED_KEY_HOSTNAME, logGroup.GetMetadata(EventGroupMetaKey::HOST_NAME).substr(0, 99));

    // zone info for ant
    const std::string& alipayZone = AppConfig::GetInstance()->GetAlipayZone();
    if (!alipayZone.empty()) {
        logGroup.SetTagNoCopy(LOG_RESERVED_KEY_ALIPAY_ZONE, alipayZone);
    }

    // add env tags
    static const std::vector<sls_logs::LogTag>& sEnvTags = AppConfig::GetInstance()->GetEnvTags();
    if (!sEnvTags.empty()) {
        for (size_t i = 0; i < sEnvTags.size(); ++i) {
            logGroup.SetTagNoCopy(sEnvTags[i].key(), sEnvTags[i].value());
        }
    }
}

bool ProcessorTagNative::IsSupportedEvent(const PipelineEventPtr& /*e*/) const {
    return true;
}

} // namespace logtail