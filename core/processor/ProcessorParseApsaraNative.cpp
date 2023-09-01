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

#include "processor/ProcessorParseApsaraNative.h"
#include "common/Constants.h"
#include "models/LogEvent.h"

namespace logtail {

bool ProcessorParseApsaraNative::Init(const ComponentConfig& config) {
    mSourceKey = DEFAULT_CONTENT_KEY;
    mDiscardUnmatch = config.mDiscardUnmatch;
    mUploadRawLog = config.mUploadRawLog;
    return true;
}

void ProcessorParseApsaraNative::Process(PipelineEventGroup& logGroup) {
    return;
}

bool ProcessorParseApsaraNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e, time_t& lastLogTime, StringView& timeStrCache) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    buffer = 

    int64_t logTime_in_micro = 0;
    time_t logTime = ApsaraEasyReadLogTimeParser(buffer.data(), timeStrCache, lastLogTime, logTime_in_micro);
    if (logTime <= 0) // this case will handle empty apsara log line
    {
        StringView bufOut(buffer);
        if (buffer.size() > (size_t)(1024)) {
            bufOut = buffer.substr(0, 1024);
        }
        if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
            if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                LOG_WARNING(sLogger,
                            ("discard error timeformat log", bufOut)("parsed time", logTime)("project", projectName)(
                                "logstore", category)("file", logPath));
            }
        }

        LogtailAlarm::GetInstance()->SendAlarm(
            PARSE_TIME_FAIL_ALARM, bufOut.to_string() + " $ " + ToString(logTime), projectName, category, region);
        error = PARSE_LOG_TIMEFORMAT_ERROR;

        if (!discardUnmatch) {
            AddUnmatchLog(buffer, logGroup, logGroupSize);
        }
        return false;
    }
    if (BOOL_FLAG(ilogtail_discard_old_data)
        && (time(NULL) - logTime + tzOffsetSecond) > INT32_FLAG(ilogtail_discard_interval)) {
        if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
            StringView bufOut(buffer);
            if (buffer.size() > (size_t)(1024)) {
                bufOut = buffer.substr(0, 1024);
            }
            if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                LOG_WARNING(sLogger,
                            ("discard history data, first 1k", bufOut)("parsed time", logTime)("project", projectName)(
                                "logstore", category)("file", logPath));
            }
            LogtailAlarm::GetInstance()->SendAlarm(OUTDATED_LOG_ALARM,
                                                   string("logTime: ") + ToString(logTime)
                                                       + ", log:" + bufOut.to_string(),
                                                   projectName,
                                                   category,
                                                   region);
        }

        error = PARSE_LOG_HISTORY_ERROR;
        return false;
    }

    Log* logPtr = logGroup.add_logs();
    SetLogTime(logPtr, logTime, logTime_in_micro * 1000 % 1000000000);
    int32_t beg_index = 0;
    int32_t colon_index = -1;
    int32_t index = -1;
    index = ParseApsaraBaseFields(buffer.data(), logPtr, logGroupSize);
    if (buffer.data()[index] != 0) {
        do {
            ++index;
            if (buffer.data()[index] == '\t' || buffer.data()[index] == '\0') {
                if (colon_index >= 0) {
                    AddLog(logPtr,
                           string(buffer.data() + beg_index, colon_index - beg_index),
                           string(buffer.data() + colon_index + 1, index - colon_index - 1),
                           logGroupSize);
                    colon_index = -1;
                }
                beg_index = index + 1;
            } else if (buffer.data()[index] == ':' && colon_index == -1) {
                colon_index = index;
            }
        } while (buffer.data()[index]);
    }
    if (adjustApsaraMicroTimezone) {
        logTime_in_micro = (int64_t)logTime_in_micro - (int64_t)tzOffsetSecond * (int64_t)1000000;
    }
    char s_micro[20] = {0};
#if defined(__linux__)
    sprintf(s_micro, "%ld", logTime_in_micro);
#elif defined(_MSC_VER)
    sprintf(s_micro, "%lld", logTime_in_micro);
#endif
    AddLog(logPtr, "microtime", string(s_micro), logGroupSize);
    return true;
    return true;
}

time_t ProcessorParseApsaraNative::ApsaraEasyReadLogTimeParser(const char* buffer, std::string& timeStr, time_t& lastLogTime, int64_t& microTime) {
    int beg_index = 0;
    if (buffer[beg_index] != '[') {
        return 0;
    }
    int curTime = 0;
    if (buffer[1] == '1') // for normal time, e.g 1378882630, starts with '1'
    {
        int i = 0;
        for (; i < 16; i++) // 16 = 10(second width) + 6(micro part)
        {
            const char& c = buffer[1 + i];
            if (c >= '0' && c <= '9') {
                if (i < 10) {
                    curTime = curTime * 10 + c - '0';
                }
                microTime = microTime * 10 + c - '0';
            } else {
                break;
            }
        }
        if (i >= 10) {
            while (i < 16) {
                microTime *= 10;
                i++;
            }
            return curTime;
        }
    }
    // test other date format case
    {
        if (IsPrefixString(buffer + beg_index + 1, timeStr) == true) {
            microTime = (int64_t)lastLogTime * 1000000 + GetApsaraLogMicroTime(buffer);
            return lastLogTime;
        }
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        long nanosecond = 0;
        int nanosecondLength = 0;
        if (NULL == strptime_ns(buffer + beg_index + 1, "%Y-%m-%d %H:%M:%S", &tm, &nanosecond, &nanosecondLength)) {
            LOG_WARNING(sLogger,
                        ("parse apsara log time", "fail")("string", buffer)("timeformat", "%Y-%m-%d %H:%M:%S"));
            return 0;
        }
        tm.tm_isdst = -1;
        lastLogTime = mktime(&tm);
        // if the time is valid (strptime not return NULL), the date value size must be 19 ,like '2013-09-11 03:11:05'
        timeStr = std::string(buffer + beg_index + 1, 19);

        microTime = (int64_t)lastLogTime * 1000000 + GetApsaraLogMicroTime(buffer);
        return lastLogTime;
    }
}

int32_t ProcessorParseApsaraNative::GetApsaraLogMicroTime(const char* buffer) {
    int begIndex = 0;
    char tmp[6];
    while (buffer[begIndex]) {
        if (buffer[begIndex] == '.') {
            begIndex++;
            break;
        }
        begIndex++;
    }
    int index = 0;
    while (buffer[begIndex + index] && index < 6) {
        if (buffer[begIndex + index] == ']') {
            break;
        }
        tmp[index] = buffer[begIndex + index];
        index++;
    }
    if (index < 6) {
        for (int i = index; i < 6; i++) {
            tmp[i] = '0';
        }
    }
    char* endPtr;
    return strtol(tmp, &endPtr, 10);
}

bool ProcessorParseApsaraNative::IsPrefixString(const char* all, const std::string& prefix) {
    if (prefix.size() == 0)
        return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (all[i] == '\0')
            return false;
        if (all[i] != prefix[i])
            return false;
    }
    return true;
}

bool ProcessorParseApsaraNative::IsSupportedEvent(const PipelineEventPtr& e) {
    return e.Is<LogEvent>();
}

} // namespace logtail