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

#include "processor/ProcessorParseTimestampNative.h"

#include "app_config/AppConfig.h"
#include "common/LogtailCommonFlags.h"

namespace logtail {
bool ProcessorParseTimestampNative::Init(const ComponentConfig& config, PipelineContext& context) {
    mTimeFormat = config.mTimeFormat;
    mTimeKey = config.mTimeKey;
    mSpecifiedYear = config.mAdvancedConfig.mSpecifiedYear;
    mLegacyPreciseTimestampConfig.enabled = config.mAdvancedConfig.mEnablePreciseTimestamp;
    mLegacyPreciseTimestampConfig.key = config.mAdvancedConfig.mPreciseTimestampKey;
    mLegacyPreciseTimestampConfig.unit = config.mAdvancedConfig.mPreciseTimestampUnit;
    mLogTimeZoneOffsetSecond = config.mLogTimeZoneOffsetSecond;

    mContext = context;
    mParseTimeFailures = &(context.GetProcessProfile().parseTimeFailures);
    mHistoryFailures = &(context.GetProcessProfile().historyFailures);
    return true;
}

void ProcessorParseTimestampNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    const StringView& logPath = logGroup.GetMetadata("source");
    StringView timeStrCache;
    EventsContainer& events = logGroup.ModifiableEvents();
    // works good normally. poor performance if most data need to be discarded.
    for (auto it = events.begin(); it != events.end();) {
        if (ProcessorParseTimestampNative::ProcessEvent(logPath, *it, timeStrCache)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
    return;
}

bool ProcessorParseTimestampNative::ProcessEvent(StringView logPath, PipelineEventPtr& e, StringView& timeStrCache) {
    if (!e.Is<LogEvent>()) {
        return true;
    }
    LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mTimeKey)) {
        return true;
    }
    const StringView& timeStr = sourceEvent.GetContent(mTimeKey);
    time_t logTime = 0;
    uint64_t preciseTimestamp = 0;
    bool parseSuccess = ParseLogTime(timeStr, logPath, logTime, preciseTimestamp, timeStrCache);
    if (!parseSuccess) {
        return true;
    }
    if (logTime <= 0
        || (BOOL_FLAG(ilogtail_discard_old_data) && (time(NULL) - logTime) > INT32_FLAG(ilogtail_discard_interval))) {
        if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
            if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                LOG_WARNING(
                    sLogger,
                    ("discard history data", timeStr)("timestamp", logTime)("project", mContext.GetProjectName())(
                        "logstore", mContext.GetLogstoreName())("file", logPath));
            }
            LogtailAlarm::GetInstance()->SendAlarm(OUTDATED_LOG_ALARM,
                                                   std::string("logTime: ") + ToString(logTime),
                                                   mContext.GetProjectName(),
                                                   mContext.GetLogstoreName(),
                                                   mContext.GetRegion());
        }
        ++mHistoryFailures;
        return false;
    }
    sourceEvent.SetTimestamp(logTime);
    if (mLegacyPreciseTimestampConfig.enabled) {
        int preciseTimestampLen = 0;
        do {
            ++preciseTimestampLen;
        } while ((preciseTimestamp /= 10) > 0);
        StringBuffer sb = sourceEvent.GetSourceBuffer()->AllocateStringBuffer(preciseTimestampLen + 1);
        snprintf(sb.data, sb.capacity, "%lu", preciseTimestamp);
        sourceEvent.SetContentNoCopy(mLegacyPreciseTimestampConfig.key, StringView(sb.data, sb.size));
    }
    return true;
}

bool ProcessorParseTimestampNative::ParseLogTime(const StringView& curTimeStr, // str to parse
                                                 const StringView& logPath,
                                                 time_t& logTime,
                                                 uint64_t& preciseTimestamp,
                                                 StringView& timeStr // cache
) {
    if (!curTimeStr.starts_with(timeStr)) {
        // current implementation requires curTimeStr to be a C-style zero terminated string
        // have to copy it.
        // TODO: use better strptime which accepts string length
        char curTimeBuf[128] = "";
        size_t curTimeBufSize = std::min(127UL, curTimeStr.size());
        memcpy(curTimeBuf, curTimeStr.data(), curTimeBufSize);
        curTimeBuf[curTimeBufSize] = '\0';
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        // In order to handle timestamp not in seconds, curTimeStr will be truncated,
        // only the front 10 charaters will be used.
        // NOTE: This method can only work until 2286/11/21 1:46:39 (9999999999).
        bool keepTimeStr = (strcmp("%s", mTimeFormat.c_str()) != 0);
        const char* strptimeResult = NULL;
        if (keepTimeStr) {
            strptimeResult = Strptime(curTimeBuf, mTimeFormat.c_str(), &tm, mSpecifiedYear);
        } else {
            size_t substrsize = std::min(10UL, curTimeBufSize);
            char tmp = curTimeBuf[substrsize];
            curTimeBuf[substrsize] = '\0';
            strptimeResult = Strptime(curTimeBuf, mTimeFormat.c_str(), &tm);
            curTimeBuf[substrsize] = tmp;
        }
        if (NULL == strptimeResult) {
            if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                    LOG_WARNING(
                        sLogger,
                        ("parse time fail", curTimeStr)("project", mContext.GetProjectName())(
                            "logstore", mContext.GetLogstoreName())("file", logPath)("keep time str", keepTimeStr));
                }
                LogtailAlarm::GetInstance()->SendAlarm(PARSE_TIME_FAIL_ALARM,
                                                       curTimeStr.to_string() + " " + mTimeFormat
                                                           + " flag: " + std::to_string(keepTimeStr),
                                                       mContext.GetProjectName(),
                                                       mContext.GetLogstoreName(),
                                                       mContext.GetRegion());
            }

            ++mParseTimeFailures;
            return false;
        }
        tm.tm_isdst = -1;
        logTime = mktime(&tm);
        timeStr = StringView(curTimeStr.data(), strptimeResult - curTimeBuf);

        if (mLegacyPreciseTimestampConfig.enabled) {
            preciseTimestamp = GetPreciseTimestamp(logTime,
                                                   strptimeResult,
                                                   curTimeBufSize - timeStr.size(),
                                                   mLegacyPreciseTimestampConfig,
                                                   mLogTimeZoneOffsetSecond);
        }
    } else {
        if (mLegacyPreciseTimestampConfig.enabled) {
            preciseTimestamp = GetPreciseTimestamp(logTime,
                                                   curTimeStr.data() + timeStr.size(),
                                                   curTimeStr.size() - timeStr.size(),
                                                   mLegacyPreciseTimestampConfig,
                                                   mLogTimeZoneOffsetSecond);
        }
    }

    logTime -= mLogTimeZoneOffsetSecond;
    return true;
}

} // namespace logtail