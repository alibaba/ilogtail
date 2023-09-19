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

#include "processor/ProcessorFilterNative.h"
#include "common/Constants.h"
#include "models/LogEvent.h"
#include "plugin/ProcessorInstance.h"
#include "config_manager/ConfigManager.h"


namespace logtail {

bool ProcessorFilterNative::Init(const ComponentConfig& componentConfig) {
    const PipelineConfig& mConfig = componentConfig.GetConfig();

    if (mConfig.mAdvancedConfig.mFilterExpressionRoot.get() != nullptr) {
        mFilterExpressionRoot = mConfig.mAdvancedConfig.mFilterExpressionRoot;
    }
    if (mConfig.mFilterRule) {
        mFilterRule = mConfig.mFilterRule;
    }
    mLogType = mConfig.mLogType;
    mDiscardNoneUtf8 = mConfig.mDiscardNoneUtf8;

    mParseFailures = &(GetContext().GetProcessProfile().parseFailures);
    SetMetricsRecordRef(Name(), componentConfig.GetId());
    mProcParseErrorTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_ERROR_TOTAL);

    // old InitFilter
    Json::Value jsonRoot; // will contains the root value after parsing.
    ParseConfResult userLogRes = ParseConfig(GetContext().GetConfigName(), jsonRoot);
    if (userLogRes != CONFIG_OK) {
        if (userLogRes == CONFIG_NOT_EXIST)
            LOG_DEBUG(sLogger, (mConfig.mConfigName, "not found, uninitialized Filter"));
        if (userLogRes == CONFIG_INVALID_FORMAT)
            LOG_ERROR(sLogger,
                      ("load user config for filter fail, file content is not valid json", mConfig.mConfigName));
        return false;
    }
    if (jsonRoot.isMember("filters") == false) {
        return true;
    }
    const Json::Value& filters = jsonRoot["filters"];
    uint32_t filterSize = filters.size();
    try {
        for (uint32_t index = 0; index < filterSize; index++) {
            const Json::Value& item = filters[index];
            std::string projectName = item["project_name"].asString();
            std::string category = item["category"].asString();
            const Json::Value& keys = item["keys"];
            const Json::Value& regs = item["regs"];
            if (keys.size() != regs.size()) {
                LOG_ERROR(sLogger,
                          ("invalid filter config", "ignore it")("project", projectName)("logstore", category));
                return false;
            }
            // default filter relation is AND
            LogFilterRule* filterRule = new LogFilterRule();
            for (uint32_t i = 0; i < keys.size(); i++) {
                filterRule->FilterKeys.push_back(keys[i].asString());
                filterRule->FilterRegs.push_back(boost::regex(regs[i].asString()));
            }
            mFilters[projectName + "_" + category] = filterRule;
        }
    } catch (...) {
        LOG_ERROR(sLogger, ("Can't parse the filter config", ""));
        return false;
    }

    return true;
}

void ProcessorFilterNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }

    const StringView& logPath = logGroup.GetMetadata(EVENT_META_LOG_FILE_PATH_RESOLVED);
    EventsContainer& events = logGroup.MutableEvents();

    for (auto it = events.begin(); it != events.end();) {
        if (ProcessEvent(logPath, *it)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
}

bool ProcessorFilterNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return true;
    }

    auto& sourceEvent = e.Cast<LogEvent>();
    bool res;

    if (mFilterExpressionRoot && mFilterExpressionRoot.get() != nullptr) {
        res = Filter(sourceEvent, mFilterExpressionRoot);
    } else if (mFilterRule) {
        res = Filter(sourceEvent, mFilterRule.get());
    } else {
        res = Filter(sourceEvent);
    }
    if (res) {
        if ((mLogType != STREAM_LOG && mLogType != PLUGIN_LOG) && mDiscardNoneUtf8) {
            LogContents& contents = sourceEvent.GetMutableContents();
            for (auto& content : contents) {
                if (IsNoneUtf8(content.second.to_string())) {
                    StringBuffer valueBuffer
                        = sourceEvent.GetSourceBuffer()->AllocateStringBuffer(content.second.size() + 1);

                    const std::string value = content.second.to_string();
                    FilterNoneUtf8(value);

                    strcpy(valueBuffer.data, value.c_str());
                    valueBuffer.size = value.size();

                    contents[content.first] = StringView(valueBuffer.data, valueBuffer.size);
                }
                if (IsNoneUtf8(content.first.to_string())) {
                    StringBuffer keyBuffer
                        = sourceEvent.GetSourceBuffer()->AllocateStringBuffer(content.first.size() + 1);
                    const std::string key = content.first.to_string();
                    FilterNoneUtf8(key);
                    strcpy(keyBuffer.data, key.c_str());
                    keyBuffer.size = key.size();

                    StringBuffer valueBuffer
                        = sourceEvent.GetSourceBuffer()->AllocateStringBuffer(content.second.size() + 1);
                    const std::string value = content.second.to_string();
                    strcpy(valueBuffer.data, value.c_str());
                    valueBuffer.size = value.size();

                    contents[StringView(keyBuffer.data, keyBuffer.size)]
                        = StringView(valueBuffer.data, valueBuffer.size);
                    contents.erase(content.first);
                }
            }
        }
    }
    if (!res) {
        ++(*mParseFailures);
        mProcParseErrorTotal->Add(1);
    }

    return res;
}


bool ProcessorFilterNative::IsSupportedEvent(const PipelineEventPtr& e) {
    return e.Is<LogEvent>();
}

bool ProcessorFilterNative::Filter(LogEvent& sourceEvent, const BaseFilterNodePtr& node) {
    // null node, all logs are passed
    if (node.get() == nullptr) {
        return true;
    }

    const LogContents& contents = sourceEvent.GetContents();
    return node->Match(contents, &GetContext());
}


bool ProcessorFilterNative::Filter(LogEvent& sourceEvent, const LogFilterRule* filterRule) {
    const LogContents& contents = sourceEvent.GetContents();
    if (contents.empty()) {
        return false;
    }

    // null filterRule, all logs are passed
    if (filterRule == NULL) {
        return true;
    }


    try {
        return IsMatched(contents, *filterRule);
    } catch (...) {
        LOG_ERROR(sLogger, ("filter error ", ""));
        return false;
    }
}

bool ProcessorFilterNative::Filter(LogEvent& sourceEvent) {
    const LogContents& contents = sourceEvent.GetContents();
    if (contents.empty()) {
        return false;
    }

    std::string key(GetContext().GetProjectName() + "_" + GetContext().GetLogstoreName());
    std::unordered_map<std::string, LogFilterRule*>::iterator it = mFilters.find(key);
    if (it == mFilters.end()) {
        return true;
    }

    const LogFilterRule& rule = *(it->second);
    return IsMatched(contents, rule);
}

bool ProcessorFilterNative::IsMatched(const LogContents& contents, const LogFilterRule& rule) {
    const std::vector<std::string>& keys = rule.FilterKeys;
    const std::vector<boost::regex> regs = rule.FilterRegs;
    std::string exception;
    for (uint32_t i = 0; i < keys.size(); i++) {
        bool found = false;
        for (auto content : contents) {
            const std::string& key = content.first.to_string();
            const std::string& value = content.second.to_string();
            if (key == keys[i]) {
                found = true;
                exception.clear();
                if (!BoostRegexMatch(value.c_str(), value.size(), regs[i], exception)) {
                    if (!exception.empty()) {
                        LOG_ERROR(sLogger, ("regex_match in Filter fail", exception));

                        if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                            LogtailAlarm::GetInstance()->SendAlarm(REGEX_MATCH_ALARM,
                                                                   "regex_match in Filter fail:" + exception,
                                                                   GetContext().GetProjectName(),
                                                                   GetContext().GetLogstoreName(),
                                                                   GetContext().GetRegion());
                        }
                    }
                    return false;
                }
                break;
            }
        }
        if (false == found) {
            return false;
        }
    }
    return true;
}

static const char UTF8_BYTE_PREFIX = 0x80;
static const char UTF8_BYTE_MASK = 0xc0;

void ProcessorFilterNative::FilterNoneUtf8(const std::string& strSrc) {
    std::string* str = const_cast<std::string*>(&strSrc);

#define FILL_BLUNK_AND_CONTINUE_IF_TRUE(stat) \
    if (stat) { \
        *iter = ' '; \
        ++iter; \
        continue; \
    };

    std::string::iterator iter = str->begin();
    while (iter != str->end()) {
        uint16_t unicode = 0;
        char c;
        if ((*iter & 0x80) == 0x00) // one byte
        {
            /**
             * mapping rule: 0000 0000 - 0000 007F | 0xxxxxxx
             */
            // nothing to check
        } else if ((*iter & 0xe0) == 0xc0) // two bytes
        {
            /**
             * mapping rule: 0000 0080 - 0000 07FF | 110xxxxx 10xxxxxx
             */
            c = *iter;
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(iter + 1 == str->end());
            /* check whether the byte is in format 10xxxxxx */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 1) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            /* get unicode value */
            unicode = (((c & 0x1f) << 6) | (*(iter + 1) & 0x3f));
            /* validate unicode range */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(!(unicode >= 0x80 && unicode <= 0x7ff));
            ++iter;
        } else if ((*iter & 0xf0) == 0xe0) // three bytes
        {
            /**
             * mapping rule: 0000 0800 - 0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
             */
            c = *iter;
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((iter + 1 == str->end()) || (iter + 2 == str->end()));
            /* check whether the byte is in format 10xxxxxx */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 1) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 2) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            /* get unicode value */
            unicode = (((c & 0x0f) << 12) | ((*(iter + 1) & 0x3f) << 6) | (*(iter + 2) & 0x3f));
            /* validate unicode range */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(!(unicode >= 0x800 /* && unicode <= 0xffff */));
            ++iter;
            ++iter;
        } else if ((*iter & 0xf8) == 0xf0) // four bytes
        {
            /**
             * mapping rule: 0001 0000 - 0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
             */
            c = *iter;
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((iter + 1 == str->end()) || (iter + 2 == str->end())
                                            || (iter + 3 == str->end()));
            /* check whether the byte is in format 10xxxxxx */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 1) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 2) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 3) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            /* get unicode value */
            uint32_t unicode = 0x00000000; // for 4 bytes utf-8 encoding
            unicode = ((c & 0x07) << 18) | ((*(iter + 1) & 0x3f) << 12) | ((*(iter + 2) & 0x3f) << 6)
                | ((*(iter + 3) & 0x3f));
            /* validate unicode range */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(!(unicode >= 0x00010000 && unicode <= 0x0010ffff));
            ++iter;
            ++iter;
            ++iter;
        } else {
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(true);
        }
        ++iter;
    }
#undef FILL_BLUNK_AND_CONTINUE_IF_TRUE
}

bool ProcessorFilterNative::IsNoneUtf8(const std::string& strSrc) {
    std::string* str = const_cast<std::string*>(&strSrc);

#define FILL_BLUNK_AND_CONTINUE_IF_TRUE(stat) \
    if (stat) { \
        *iter = ' '; \
        ++iter; \
        return true; \
    };

    std::string::iterator iter = str->begin();
    while (iter != str->end()) {
        uint16_t unicode = 0;
        char c;
        if ((*iter & 0x80) == 0x00) // one byte
        {
            /**
             * mapping rule: 0000 0000 - 0000 007F | 0xxxxxxx
             */
            // nothing to check
        } else if ((*iter & 0xe0) == 0xc0) // two bytes
        {
            /**
             * mapping rule: 0000 0080 - 0000 07FF | 110xxxxx 10xxxxxx
             */
            c = *iter;
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(iter + 1 == str->end());
            /* check whether the byte is in format 10xxxxxx */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 1) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            /* get unicode value */
            unicode = (((c & 0x1f) << 6) | (*(iter + 1) & 0x3f));
            /* validate unicode range */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(!(unicode >= 0x80 && unicode <= 0x7ff));
            ++iter;
        } else if ((*iter & 0xf0) == 0xe0) // three bytes
        {
            /**
             * mapping rule: 0000 0800 - 0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
             */
            c = *iter;
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((iter + 1 == str->end()) || (iter + 2 == str->end()));
            /* check whether the byte is in format 10xxxxxx */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 1) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 2) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            /* get unicode value */
            unicode = (((c & 0x0f) << 12) | ((*(iter + 1) & 0x3f) << 6) | (*(iter + 2) & 0x3f));
            /* validate unicode range */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(!(unicode >= 0x800 /* && unicode <= 0xffff */));
            ++iter;
            ++iter;
        } else if ((*iter & 0xf8) == 0xf0) // four bytes
        {
            /**
             * mapping rule: 0001 0000 - 0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
             */
            c = *iter;
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((iter + 1 == str->end()) || (iter + 2 == str->end())
                                            || (iter + 3 == str->end()));
            /* check whether the byte is in format 10xxxxxx */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 1) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 2) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            FILL_BLUNK_AND_CONTINUE_IF_TRUE((*(iter + 3) & UTF8_BYTE_MASK) != UTF8_BYTE_PREFIX);
            /* get unicode value */
            uint32_t unicode = 0x00000000; // for 4 bytes utf-8 encoding
            unicode = ((c & 0x07) << 18) | ((*(iter + 1) & 0x3f) << 12) | ((*(iter + 2) & 0x3f) << 6)
                | ((*(iter + 3) & 0x3f));
            /* validate unicode range */
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(!(unicode >= 0x00010000 && unicode <= 0x0010ffff));
            ++iter;
            ++iter;
            ++iter;
        } else {
            FILL_BLUNK_AND_CONTINUE_IF_TRUE(true);
        }
        ++iter;
    }
#undef FILL_BLUNK_AND_CONTINUE_IF_TRUE
    return false;
}

} // namespace logtail