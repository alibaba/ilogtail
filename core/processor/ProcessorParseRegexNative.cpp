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

#include "processor/ProcessorParseRegexNative.h"

#include "parser/LogParser.h" // for UNMATCH_LOG_KEY
#include "common/Constants.h"

namespace logtail {

bool ProcessorParseRegexNative::Init(const ComponentConfig& config, PipelineContext& context) {
    mSourceKey = DEFAULT_CONTENT_KEY;
    mDiscardUnmatch = config.mDiscardUnmatch;
    mUploadRawLog = config.mUploadRawLog;
    mRawLogTag = config.mAdvancedConfig.mRawLogTag;
    std::list<std::string>::iterator regitr = config.mRegs->begin();
    std::list<std::string>::iterator keyitr = config.mKeys->begin();
    for (; regitr != config.mRegs->end(); ++regitr, ++keyitr) {
        AddUserDefinedFormat(*regitr, *keyitr);
        if (*keyitr == mSourceKey) {
            mSourceKeyOverwritten = true;
        }
        if (*keyitr == mRawLogTag) {
            mRawLogTagOverwritten = true;
        }
    }
    mContext = context;
    mParseFailures = &(context.GetProcessProfile().parseFailures);
    mRegexMatchFailures = &(context.GetProcessProfile().regexMatchFailures);
    mLogGroupSize = &(context.GetProcessProfile().logGroupSize);
    return true;
}

void ProcessorParseRegexNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    const StringView& logPath = logGroup.GetMetadata("source");
    EventsContainer& events = logGroup.ModifiableEvents();
    // works good normally. poor performance if most data need to be discarded.
    for (auto it = events.begin(); it != events.end();) {
        if (ProcessorParseRegexNative::ProcessEvent(logGroup, logPath, *it)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
    return;
}

bool ProcessorParseRegexNative::ProcessEvent(PipelineEventGroup& logGroup,
                                             const StringView& logPath,
                                             PipelineEventPtr& e) {
    if (!e.Is<LogEvent>()) {
        return true;
    }
    LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    bool res = true;
    for (uint32_t i = 0; i < mUserDefinedFormat.size(); ++i) { // support multiple patterns
        const UserDefinedFormat& format = mUserDefinedFormat[i];
        if (format.mIsWholeLineMode) {
            res = WholeLineModeParser(
                sourceEvent, format.mKeys.empty() ? DEFAULT_CONTENT_KEY : format.mKeys[0], sourceEvent);
        } else {
            res = RegexLogLineParser(sourceEvent, format.mReg, format.mKeys, logPath, sourceEvent);
        }
        if (res) {
            break;
        }
    }
    if (!res && !mDiscardUnmatch) {
        std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(logGroup.GetSourceBuffer());
        AddLog(LogParser::UNMATCH_LOG_KEY, // __raw_log__
               sourceEvent.GetContent(mSourceKey),
               *targetEvent); // legacy behavior, should use sourceKey
    }
    if (res && !mSourceKeyOverwritten) {
        sourceEvent.DelContent(mSourceKey);
    }
    if (res || !mDiscardUnmatch) {
        if (mUploadRawLog && (!res || !mRawLogTagOverwritten)) {
            AddLog(mRawLogTag, sourceEvent.GetContent(mSourceKey), sourceEvent); // __raw__
        }
        return true;
    }
    return false;
}

void ProcessorParseRegexNative::AddUserDefinedFormat(const std::string& regStr, const std::string& keys) {
    std::vector<std::string> keyParts = StringSpliter(keys, ",");
    boost::regex reg(regStr);
    bool isWholeLineMode = regStr == "(.*)";
    mUserDefinedFormat.push_back(UserDefinedFormat(reg, keyParts, isWholeLineMode));
}

bool ProcessorParseRegexNative::WholeLineModeParser(const LogEvent& sourceEvent,
                                                    const std::string& key,
                                                    LogEvent& targetEvent) {
    targetEvent.SetTimestamp(sourceEvent.GetTimestamp());
    AddLog(StringView(key), sourceEvent.GetContent(mSourceKey), targetEvent);
    return true;
}

void ProcessorParseRegexNative::AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent) {
    targetEvent.SetContentNoCopy(key, value);
    *mLogGroupSize += key.size() + value.size() + 5;
}

bool ProcessorParseRegexNative::RegexLogLineParser(const LogEvent& sourceEvent,
                                                   const boost::regex& reg,
                                                   const std::vector<std::string>& keys,
                                                   const StringView& logPath,
                                                   LogEvent& targetEvent) {
    boost::match_results<const char*> what;
    std::string exception;
    StringView buffer = sourceEvent.GetContent(mSourceKey);
    bool parseSuccess = true;
    if (!BoostRegexMatch(buffer.data(), buffer.size(), reg, exception, what, boost::match_default)) {
        if (!exception.empty()) {
            if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                    LOG_ERROR(
                        mContext.GetLogger(),
                        ("parse regex log fail", buffer)("exception", exception)("project", mContext.GetProjectName())(
                            "logstore", mContext.GetLogstoreName())("file", logPath));
                }
                LogtailAlarm::GetInstance()->SendAlarm(REGEX_MATCH_ALARM,
                                                       "errorlog:" + buffer.to_string() + " | exception:" + exception,
                                                       mContext.GetProjectName(),
                                                       mContext.GetLogstoreName(),
                                                       mContext.GetRegion());
            }
        } else {
            if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                    LOG_WARNING(mContext.GetLogger(),
                                ("parse regex log fail", buffer)("project", mContext.GetProjectName())(
                                    "logstore", mContext.GetLogstoreName())("file", logPath));
                }
                LogtailAlarm::GetInstance()->SendAlarm(REGEX_MATCH_ALARM,
                                                       std::string("errorlog:") + buffer.to_string(),
                                                       mContext.GetProjectName(),
                                                       mContext.GetLogstoreName(),
                                                       mContext.GetRegion());
            }
        }

        ++(*mParseFailures);
        parseSuccess = false;
    } else if (what.size() <= keys.size()) {
        if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
            if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                LOG_WARNING(
                    mContext.GetLogger(),
                    ("parse key count not match", what.size())("parse regex log fail", buffer)(
                        "project", mContext.GetProjectName())("logstore", mContext.GetLogstoreName())("file", logPath));
            }
            LogtailAlarm::GetInstance()->SendAlarm(REGEX_MATCH_ALARM,
                                                   "parse key count not match" + ToString(what.size())
                                                       + "errorlog:" + buffer.to_string(),
                                                   mContext.GetProjectName(),
                                                   mContext.GetLogstoreName(),
                                                   mContext.GetRegion());
        }
        ++(*mParseFailures);
        parseSuccess = false;
    }
    if (!parseSuccess) {
        return false;
    }

    targetEvent.SetTimestamp(sourceEvent.GetTimestamp());
    for (uint32_t i = 0; i < keys.size(); i++) {
        AddLog(keys[i], StringView(what[i + 1].begin(), what[i + 1].length()), targetEvent);
    }
    return true;
}

} // namespace logtail