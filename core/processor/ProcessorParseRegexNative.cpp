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
#include "app_config/AppConfig.h"
#include "monitor/MetricConstants.h"
#include "plugin/instance/ProcessorInstance.h"
#include "common/ParamExtractor.h"

using namespace std;

namespace logtail {
const std::string ProcessorParseRegexNative::sName = "processor_parse_regex_native";
const std::string ProcessorParseRegexNative::UNMATCH_LOG_KEY = "__raw_log__";

bool ProcessorParseRegexNative::Init(const Json::Value& config) {
    std::string errorMsg;
    if (!GetMandatoryStringParam(config, "SourceKey", mSourceKey, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }
    if (!GetMandatoryStringParam(config, "Regex", mRegex, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }
    if (!GetMandatoryListParam(config, "Keys", mKeys, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }

    if (!GetOptionalBoolParam(config, "KeepingSourceWhenParseFail", mKeepingSourceWhenParseFail, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            mContext->GetLogger(), errorMsg, mKeepingSourceWhenParseFail, sName, mContext->GetConfigName());
    }
    if (!GetOptionalBoolParam(config, "KeepingSourceWhenParseSucceed", mKeepingSourceWhenParseSucceed, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            mContext->GetLogger(), errorMsg, mKeepingSourceWhenParseSucceed, sName, mContext->GetConfigName());
    }
    if (!GetOptionalStringParam(config, "RenamedSourceKey", mRenamedSourceKey, errorMsg)) {
        mRenamedSourceKey = mSourceKey;
        PARAM_WARNING_DEFAULT(mContext->GetLogger(), errorMsg, mRenamedSourceKey, sName, mContext->GetConfigName());
    }
    if (!GetOptionalBoolParam(config, "CopingRawLog", mCopingRawLog, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(), errorMsg, mCopingRawLog, sName, mContext->GetConfigName());
    }

    AddUserDefinedFormat();

    if (mKeepingSourceWhenParseSucceed && mRenamedSourceKey == mSourceKey) {
        mSourceKeyOverwritten = true;
    }

    mParseFailures = &(GetContext().GetProcessProfile().parseFailures);
    mRegexMatchFailures = &(GetContext().GetProcessProfile().regexMatchFailures);
    mLogGroupSize = &(GetContext().GetProcessProfile().logGroupSize);

    mProcParseInSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_IN_SIZE_BYTES);
    mProcParseOutSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_OUT_SIZE_BYTES);
    mProcDiscardRecordsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_DISCARD_RECORDS_TOTAL);
    mProcParseErrorTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_ERROR_TOTAL);
    mProcKeyCountNotMatchErrorTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_KEY_COUNT_NOT_MATCH_ERROR_TOTAL);
    return true;
}

void ProcessorParseRegexNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty() || mUserDefinedFormat.empty()) {
        return;
    }
    const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
    EventsContainer& events = logGroup.MutableEvents();
    // works good normally. poor performance if most data need to be discarded.
    for (auto it = events.begin(); it != events.end();) {
        if (ProcessorParseRegexNative::ProcessEvent(logPath, *it)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
    return;
}

bool ProcessorParseRegexNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

bool ProcessorParseRegexNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    auto rawContent = sourceEvent.GetContent(mSourceKey);
    bool parseSuccess = true;
    for (uint32_t i = 0; i < mUserDefinedFormat.size(); ++i) { // support multiple patterns
        const UserDefinedFormat& format = mUserDefinedFormat[i];
        if (format.mIsWholeLineMode) {
            parseSuccess
                = WholeLineModeParser(sourceEvent, format.mKeys.empty() ? DEFAULT_CONTENT_KEY : format.mKeys[0]);
        } else {
            parseSuccess = RegexLogLineParser(sourceEvent, format.mReg, format.mKeys, logPath);
        }
        if (parseSuccess) {
            break;
        }
    }
    if (!parseSuccess && (mKeepingSourceWhenParseFail || mCopingRawLog)) {
        AddLog(UNMATCH_LOG_KEY, // __raw_log__
               rawContent,
               sourceEvent); // legacy behavior, should use sourceKey
    }
    if (parseSuccess || mKeepingSourceWhenParseFail) {
        if (mKeepingSourceWhenParseSucceed && (!parseSuccess || !mRawLogTagOverwritten)) {
            AddLog(mRenamedSourceKey, rawContent, sourceEvent); // __raw__
        }
        if (parseSuccess && !mSourceKeyOverwritten) {
            sourceEvent.DelContent(mSourceKey);
        }
        return true;
    }
    mProcDiscardRecordsTotal->Add(1);
    return false;
}

void ProcessorParseRegexNative::AddUserDefinedFormat() {
    for (auto& it : mKeys) {
        if (it == mSourceKey) {
            mSourceKeyOverwritten = true;
        }
        if (it == mRenamedSourceKey) {
            mRawLogTagOverwritten = true;
        }
    }
    boost::regex reg(mRegex);
    bool isWholeLineMode = mRegex == "(.*)";
    mUserDefinedFormat.push_back(UserDefinedFormat(reg, mKeys, isWholeLineMode));
}

bool ProcessorParseRegexNative::WholeLineModeParser(LogEvent& sourceEvent, const std::string& key) {
    StringView buffer = sourceEvent.GetContent(mSourceKey);
    AddLog(StringView(key), buffer, sourceEvent);
    mProcParseInSizeBytes->Add(buffer.size());
    return true;
}

void ProcessorParseRegexNative::AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent) {
    targetEvent.SetContentNoCopy(key, value);
    size_t keyValueSize = key.size() + value.size();
    *mLogGroupSize += keyValueSize + 5;
    mProcParseOutSizeBytes->Add(keyValueSize);
}

bool ProcessorParseRegexNative::RegexLogLineParser(LogEvent& sourceEvent,
                                                   const boost::regex& reg,
                                                   const std::vector<std::string>& keys,
                                                   const StringView& logPath) {
    boost::match_results<const char*> what;
    std::string exception;
    StringView buffer = sourceEvent.GetContent(mSourceKey);
    bool parseSuccess = true;
    mProcParseInSizeBytes->Add(buffer.size());
    if (!BoostRegexMatch(buffer.data(), buffer.size(), reg, exception, what, boost::match_default)) {
        if (!exception.empty()) {
            if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                if (GetContext().GetAlarm().IsLowLevelAlarmValid()) {
                    LOG_ERROR(GetContext().GetLogger(),
                              ("parse regex log fail", buffer)("exception", exception)("project",
                                                                                       GetContext().GetProjectName())(
                                  "logstore", GetContext().GetLogstoreName())("file", logPath));
                }
                GetContext().GetAlarm().SendAlarm(REGEX_MATCH_ALARM,
                                                  "errorlog:" + buffer.to_string() + " | exception:" + exception,
                                                  GetContext().GetProjectName(),
                                                  GetContext().GetLogstoreName(),
                                                  GetContext().GetRegion());
            }
        } else {
            if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                if (GetContext().GetAlarm().IsLowLevelAlarmValid()) {
                    LOG_WARNING(GetContext().GetLogger(),
                                ("parse regex log fail", buffer)("project", GetContext().GetProjectName())(
                                    "logstore", GetContext().GetLogstoreName())("file", logPath));
                }
                GetContext().GetAlarm().SendAlarm(REGEX_MATCH_ALARM,
                                                  std::string("errorlog:") + buffer.to_string(),
                                                  GetContext().GetProjectName(),
                                                  GetContext().GetLogstoreName(),
                                                  GetContext().GetRegion());
            }
        }
        ++(*mRegexMatchFailures);
        ++(*mParseFailures);
        mProcParseErrorTotal->Add(1);
        parseSuccess = false;
    } else if (what.size() <= keys.size()) {
        if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
            if (GetContext().GetAlarm().IsLowLevelAlarmValid()) {
                LOG_WARNING(GetContext().GetLogger(),
                            ("parse key count not match",
                             what.size())("parse regex log fail", buffer)("project", GetContext().GetProjectName())(
                                "logstore", GetContext().GetLogstoreName())("file", logPath));
            }
            GetContext().GetAlarm().SendAlarm(REGEX_MATCH_ALARM,
                                              "parse key count not match" + ToString(what.size())
                                                  + "errorlog:" + buffer.to_string(),
                                              GetContext().GetProjectName(),
                                              GetContext().GetLogstoreName(),
                                              GetContext().GetRegion());
        }
        ++(*mRegexMatchFailures);
        ++(*mParseFailures);
        mProcKeyCountNotMatchErrorTotal->Add(1);
        parseSuccess = false;
    }
    if (!parseSuccess) {
        return false;
    }

    for (uint32_t i = 0; i < keys.size(); i++) {
        AddLog(keys[i], StringView(what[i + 1].begin(), what[i + 1].length()), sourceEvent);
    }
    return true;
}

} // namespace logtail