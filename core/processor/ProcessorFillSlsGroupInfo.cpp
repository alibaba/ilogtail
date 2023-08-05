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

#include "processor/ProcessorFillSlsGroupInfo.h"

#include "common/Constants.h"
#include "reader/LogFileReader.h"
namespace logtail {

bool ProcessorFillSlsGroupInfo::Init(const ComponentConfig& config, PipelineContext& context) {
    mConfigName = config.mConfigName;
    mTopicFormat = config.mTopicFormat;
    mGroupTopic = config.mGroupTopic;
    if ("customized" == config.mTopicFormat) {
        mCustomizedTopic = config.mCustomizedTopic;
    }
    mContext = context;
    return true;
}

void ProcessorFillSlsGroupInfo::Process(PipelineEventGroup& logGroup) {
    LogBuffer* logBuffer = dynamic_cast<LogBuffer*>(logGroup.GetSourceBuffer().get());
    std::string agentTag = ConfigManager::GetInstance()->GetUserDefinedIdSet();
    if (!agentTag.empty()) {
        logGroup.SetMetadata(EVENT_META_AGENT_TAG, ConfigManager::GetInstance()->GetUserDefinedIdSet());
    }
    logGroup.SetMetadataNoCopy(EVENT_META_HOST_IP, LogFileProfiler::mIpAddr);
    logGroup.SetMetadataNoCopy(EVENT_META_HOST_NAME, LogFileProfiler::mHostname);
    logGroup.SetMetadataNoCopy(EVENT_META_LOG_FILE_PATH, logBuffer->logFileReader->GetConvertedPath());
    logGroup.SetMetadataNoCopy(EVENT_META_LOG_FILE_PATH_RESOLVED, logBuffer->logFileReader->GetRealLogPath());
    /* inode和topic还有docker的tags在reader中混杂在一起，后续剥离开后再改
    logGroup.SetMetadata(EVENT_META_LOG_FILE_INODE, std::to_string(logBuffer->logFileReader->GetDevInode().inode));
    // topic
    const std::string lowerConfig = ToLowerCaseString(mTopicFormat);
    std::string topicName;
    if (lowerConfig == "customized") {
        topicName = mCustomizedTopic;
    } else if (lowerConfig == "global_topic") {
        static LogtailGlobalPara* sGlobalPara = LogtailGlobalPara::Instance();
        topicName = sGlobalPara->GetTopic();
    } else if (lowerConfig == "group_topic") {
        topicName = mGroupTopic;
    } else if (lowerConfig != "none" && lowerConfig != "default") {
        std::vector<sls_logs::LogTag> extraTags;
        topicName = GetTopicName(mTopicFormat, logBuffer->logFileReader->GetConvertedPath(), extraTags);
    }
    if (!topicName.empty()) {
        logGroup.SetMetadata(EVENT_META_LOG_TOPIC, topicName);
    }
    */
    return;
}

std::string ProcessorFillSlsGroupInfo::GetTopicName(const std::string& topicConfig,
                                                    const std::string& path,
                                                    std::vector<sls_logs::LogTag>& extraTags) {
    std::string finalPath = path;
    size_t len = finalPath.size();
    // ignore the ".1" like suffix when the log file is roll back
    if (len > 2 && finalPath[len - 2] == '.' && finalPath[len - 1] > '0' && finalPath[len - 1] < '9') {
    finalPath = finalPath.substr(0, len - 2);
    }

    {
    std::string res;
    // use xpressive
    std::vector<std::string> keys;
    std::vector<std::string> values;
    if (ExtractTopics(finalPath, topicConfig, keys, values)) {
        size_t matchedSize = values.size();
        if (matchedSize == (size_t)1) {
            // != default topic name
            if (keys[0] != "__topic_1__") {
                sls_logs::LogTag tag;
                tag.set_key(keys[0]);
                tag.set_value(values[0]);
                extraTags.push_back(tag);
            }
            return values[0];
        } else {
            for (size_t i = 0; i < matchedSize; ++i) {
                if (res.empty()) {
                    res = values[i];
                } else {
                    res = res + "_" + values[i];
                }
                sls_logs::LogTag tag;
                tag.set_key(keys[i]);
                tag.set_value(values[i]);
                extraTags.push_back(tag);
            }
        }
        return res;
    }
    }

    boost::match_results<const char*> what;
    std::string res, exception;
    // catch exception
    boost::regex topicRegex;
    try {
    topicRegex = boost::regex(topicConfig.c_str());
    if (BoostRegexMatch(finalPath.c_str(), finalPath.length(), topicRegex, exception, what, boost::match_default)) {
        size_t matchedSize = what.size();
        for (size_t i = 1; i < matchedSize; ++i) {
            if (res.empty()) {
                res = what[i];
            } else {
                res = res + "_" + what[i];
            }
            if (matchedSize > 2) {
                sls_logs::LogTag tag;
                tag.set_key(std::string("__topic_") + ToString(i) + "__");
                tag.set_value(what[i]);
                extraTags.push_back(tag);
            }
        }
    } else {
        if (!exception.empty())
            LOG_ERROR(mContext.GetLogger(),
                      ("extract topic by regex", "fail")("exception", exception)("project", mContext.GetProjectName())(
                          "logstore", mContext.GetLogstoreName())("path", finalPath)("regx", topicConfig));
        else
            LOG_WARNING(mContext.GetLogger(),
                        ("extract topic by regex", "fail")("project", mContext.GetProjectName())(
                            "logstore", mContext.GetLogstoreName())("path", finalPath)("regx", topicConfig));

        LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
                                               std::string("extract topic by regex fail, exception:") + exception
                                                   + ", path:" + finalPath + ", regex:" + topicConfig,
                                               mContext.GetProjectName(),
                                               mContext.GetLogstoreName(),
                                               mContext.GetRegion());
    }
    } catch (...) {
    LOG_ERROR(mContext.GetLogger(),
              ("extract topic by regex", "fail")("exception", exception)("project", mContext.GetProjectName())(
                  "logstore", mContext.GetLogstoreName())("path", finalPath)("regx", topicConfig));
    LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
                                           std::string("extract topic by regex fail, exception:") + exception
                                               + ", path:" + finalPath + ", regex:" + topicConfig,
                                           mContext.GetProjectName(),
                                           mContext.GetLogstoreName(),
                                           mContext.GetRegion());
    }

    return res;
}

} // namespace logtail