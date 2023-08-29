// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdlib>
#include "unittest/Unittest.h"
#include "common/JsonUtil.h"
#include "config/Config.h"
#include "processor/ProcessorParseTimestampNative.h"

namespace logtail {

class ProcessorParseTimestampNativeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
    }

    void TestInit();
    void TestProcessNoFormat();
    void TestProcessEventRegularFormat();
    void TestProcessEventRegularFormatFailed();
    void TestProcessEventHistoryDiscard();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorParseTimestampNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorParseTimestampNativeUnittest, TestProcessNoFormat);
UNIT_TEST_CASE(ProcessorParseTimestampNativeUnittest, TestProcessEventRegularFormat);
UNIT_TEST_CASE(ProcessorParseTimestampNativeUnittest, TestProcessEventRegularFormatFailed);
UNIT_TEST_CASE(ProcessorParseTimestampNativeUnittest, TestProcessEventHistoryDiscard);

void ProcessorParseTimestampNativeUnittest::TestInit() {
    Config config;
    config.mTimeFormat = "%Y-%m-%d %H:%M:%S";
    config.mTimeKey = "time";
    config.mAdvancedConfig.mSpecifiedYear = 0;
    config.mAdvancedConfig.mEnablePreciseTimestamp = false;
    config.mAdvancedConfig.mPreciseTimestampKey = "precise_timestamp";
    config.mAdvancedConfig.mPreciseTimestampUnit = TimeStampUnit::MILLISECOND;
    config.mLogTimeZoneOffsetSecond = 28800;

    ProcessorParseTimestampNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

void ProcessorParseTimestampNativeUnittest::TestProcessNoFormat() {
    // make config
    Config config;
    config.mTimeFormat = "";
    config.mTimeKey = "time";
    config.mLogTimeZoneOffsetSecond = 28800;
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "time" : "2023-07-30 23:00:00"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseTimestampNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    processor.Process(eventGroup);
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(inJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseTimestampNativeUnittest::TestProcessEventRegularFormat() {
    // make config
    Config config;
    config.mTimeFormat = "%Y-%m-%d %H:%M:%S";
    config.mTimeKey = "time";
    config.mLogTimeZoneOffsetSecond = 28800;
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    auto logEvent = PipelineEventPtr(LogEvent::CreateEvent(sourceBuffer));
    time_t now = time(nullptr); // will not be discarded by history timeout
    char timebuff[32] = "";
    std::tm* now_tm = std::localtime(&now);
    strftime(timebuff, sizeof(timebuff), config.mTimeFormat.c_str(), now_tm);
    std::stringstream inJsonSs;
    inJsonSs << R"({
        "contents" :
        {
            "time" : ")"
             << timebuff << R"("
        },
        "timestamp" : 12345678901,
        "type" : 1
    })";
    logEvent->FromJsonString(inJsonSs.str());
    // run function
    ProcessorParseTimestampNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    logtail::StringView timeStrCache;
    LogtailTime logTime = {0, 0};
    APSARA_TEST_TRUE_FATAL(processor.ProcessEvent("/var/log/message", logEvent, logTime, timeStrCache));
    // judge result
    std::string outJson = logEvent->ToJsonString();
    std::stringstream expectJsonSs;
    expectJsonSs << R"({
        "contents" :
        {
            "time" : ")"
                 << timebuff << R"("
        },
        "timestamp" : )"
                 << now - config.mLogTimeZoneOffsetSecond << R"(,
        "type" : 1
    })"; // TODO: adjust time zone
    APSARA_TEST_EQUAL_FATAL(CompactJson(expectJsonSs.str()), CompactJson(outJson));
}

void ProcessorParseTimestampNativeUnittest::TestProcessEventRegularFormatFailed() {
    // make config
    Config config;
    config.mTimeFormat = "%Y-%m-%d %H:%M:%S";
    config.mTimeKey = "time";
    config.mLogTimeZoneOffsetSecond = 28800;
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    auto logEvent = PipelineEventPtr(LogEvent::CreateEvent(sourceBuffer));
    time_t now
        = time(nullptr) - INT32_FLAG(ilogtail_discard_interval) - 1; // will parse fail so timeout should not matter
    char timebuff[32] = "";
    std::tm* now_tm = std::localtime(&now);
    strftime(timebuff, sizeof(timebuff), "%Y-%m-%d", now_tm);
    std::stringstream inJsonSs;
    inJsonSs << R"({
        "contents" :
        {
            "time" : ")"
             << timebuff << R"("
        },
        "timestamp" : 12345678901,
        "type" : 1
    })";
    std::string inJson = inJsonSs.str();
    logEvent->FromJsonString(inJson);
    // run function
    ProcessorParseTimestampNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    logtail::StringView timeStrCache;
    LogtailTime logTime = {0, 0};
    APSARA_TEST_TRUE_FATAL(processor.ProcessEvent("/var/log/message", logEvent, logTime, timeStrCache));
    // judge result
    std::string outJson = logEvent->ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(inJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseTimestampNativeUnittest::TestProcessEventHistoryDiscard() {
    // make config
    Config config;
    config.mTimeFormat = "%Y-%m-%d %H:%M:%S";
    config.mTimeKey = "time";
    config.mLogTimeZoneOffsetSecond = 28800;
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    auto logEvent = PipelineEventPtr(LogEvent::CreateEvent(sourceBuffer));
    time_t now = time(nullptr) - INT32_FLAG(ilogtail_discard_interval) - 1; // will be discarded by history timeout
    char timebuff[32] = "";
    std::tm* now_tm = std::localtime(&now);
    strftime(timebuff, sizeof(timebuff), config.mTimeFormat.c_str(), now_tm);
    std::stringstream inJsonSs;
    inJsonSs << R"({
        "contents" :
        {
            "time" : ")"
             << timebuff << R"("
        },
        "timestamp" : 12345678901,
        "type" : 1
    })";
    logEvent->FromJsonString(inJsonSs.str());
    // run function
    ProcessorParseTimestampNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    logtail::StringView timeStrCache;
    LogtailTime logTime = {0, 0};
    APSARA_TEST_FALSE_FATAL(processor.ProcessEvent("/var/log/message", logEvent, logTime, timeStrCache));
    // check observablity
    APSARA_TEST_EQUAL_FATAL(1, processor.GetContext().GetProcessProfile().historyFailures);
}

class ProcessorParseLogTimeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
    }

    void TestParseLogTime();
    void TestParseLogTimeSecondCache();
    void TestAdjustTimeZone();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorParseLogTimeUnittest, TestParseLogTime);
UNIT_TEST_CASE(ProcessorParseLogTimeUnittest, TestParseLogTimeSecondCache);
UNIT_TEST_CASE(ProcessorParseLogTimeUnittest, TestAdjustTimeZone);

void ProcessorParseLogTimeUnittest::TestParseLogTime() {
    struct Case {
        std::string inputTimeStr;
        std::string fmtStr;
        time_t exceptedLogTime;
        long exceptedLogTimeNanosecond;
        uint64_t exceptedPreciseTimestamp;
    };

    std::vector<Case> inputTimes = {
        {"2017-1-11 15:05:07.012", "%Y-%m-%d %H:%M:%S.%f", 1484147107, 12000000, 1484147107012},
        {"2017-1-11 15:05:07.012", "%Y-%m-%d %H:%M:%S.%f", 1484147107, 12000000,1484147107012},
        {"[2017-1-11 15:05:07.0123]", "[%Y-%m-%d %H:%M:%S.%f", 1484147107, 12300000,1484147107012},
        {"11 Jan 17 15:05 MST", "%d %b %y %H:%M", 1484147100, 0, 1484147100000},
        {"11 Jan 17 15:05 -0700", "%d %b %y %H:%M", 1484147100, 0, 1484147100000},
        {"Tuesday, 11-Jan-17 15:05:07.0123 MST", "%A, %d-%b-%y %H:%M:%S.%f", 1484147107, 12300000, 1484147107012},
        {"Tuesday, 11 Jan 2017 15:05:07 MST", "%A, %d %b %Y %H:%M:%S", 1484147107, 0, 1484147107000},
        {"2017-01-11T15:05:07Z08:00", "%Y-%m-%dT%H:%M:%S", 1484147107, 0, 1484147107000},
        {"2017-01-11T15:05:07.012999999Z07:00", "%Y-%m-%dT%H:%M:%S.%f", 1484147107, 12999999, 1484147107012},
        {"1484147107", "%s", 1484147107, 0, 1484147107000},
        {"1484147107123", "%s", 1484147107, 123000000, 1484147107123},
        {"15:05:07.012 2017-1-11", "%H:%M:%S.%f %Y-%m-%d", 1484147107, 12000000, 1484147107012},
        {"2017-1-11 15:05:07.012 +0700 (UTC)", "%Y-%m-%d %H:%M:%S.%f %z (%Z)", 1484147107, 12000000, 1484147107012},
        // Compatibility Test
        {"2017-1-11 15:05:07.012", "%Y-%m-%d %H:%M:%S", 1484147107, 0, 1484147107012},
    };
    Config config;
    config.mTimeKey = "time";
    config.mLogTimeZoneOffsetSecond = -GetLocalTimeZoneOffsetSecond();
    config.mAdvancedConfig.mEnablePreciseTimestamp = true;
    config.mAdvancedConfig.mPreciseTimestampUnit = TimeStampUnit::MILLISECOND;
    for (size_t i = 0; i < inputTimes.size(); ++i) {
        auto& c = inputTimes[i];
        config.mTimeFormat = c.fmtStr;
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        StringView timeStrCache;
        bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
        EXPECT_EQ(ret, true) << "failed: " + c.inputTimeStr;
        EXPECT_EQ(outTime.tv_sec, c.exceptedLogTime) << "failed: " + c.inputTimeStr;
        EXPECT_EQ(outTime.tv_nsec, c.exceptedLogTimeNanosecond) << "failed: " + c.inputTimeStr;
        EXPECT_EQ(preciseTimestamp, c.exceptedPreciseTimestamp) << "failed: " + c.inputTimeStr;
    }
}

void ProcessorParseLogTimeUnittest::TestParseLogTimeSecondCache() {
    struct Case {
        std::string inputTimeStr;
        time_t exceptedLogTime;
        long exceptedLogTimeNanosecond;
        uint64_t exceptedPreciseTimestamp;

        Case(std::string _inputTimeStr,
             time_t _exceptedLogTime,
             long _exceptedLogTimeNanosecond,
             uint64_t _exceptedPreciseTimestamp)
            : inputTimeStr(_inputTimeStr),
              exceptedLogTime(_exceptedLogTime),
              exceptedLogTimeNanosecond(_exceptedLogTimeNanosecond),
              exceptedPreciseTimestamp(_exceptedPreciseTimestamp) {}
    };
    Config config;
    config.mTimeKey = "time";
    config.mLogTimeZoneOffsetSecond = -GetLocalTimeZoneOffsetSecond();
    config.mAdvancedConfig.mEnablePreciseTimestamp = true;
    config.mAdvancedConfig.mPreciseTimestampUnit = TimeStampUnit::MICROSECOND;
    { // case: second
        config.mTimeFormat = "%Y-%m-%d %H:%M:%S";
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));

        time_t expectLogTimeBase = 1325430300;
        long expectLogTimeNanosecondBase = 1325430300000000;
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        std::vector<Case> inputTimes;
        for (size_t i = 0; i < 5; ++i) {
            std::string second = "2012-01-01 15:05:" + (i < 10 ? "0" + std::to_string(i) : std::to_string(i));
            for (size_t j = 0; j < 5; ++j) {
                inputTimes.emplace_back(std::string(second.data()), expectLogTimeBase + i, 0, expectLogTimeNanosecondBase + i * 1000000);
            }
        }

        StringView timeStrCache = "2012-01-01 15:04:59";
        for (size_t i = 0; i < inputTimes.size(); ++i) {
            auto c = inputTimes[i];
            bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
            APSARA_TEST_EQUAL(ret, true);
            APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
            APSARA_TEST_EQUAL(outTime.tv_nsec, c.exceptedLogTimeNanosecond);
            APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
        }
    }
    { // case: nanosecond
        config.mTimeFormat = "%Y-%m-%d %H:%M:%S.%f";
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));

        time_t expectLogTimeBase = 1325430300;
        long expectLogTimeNanosecondBase = 1325430300000000;
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        std::vector<Case> inputTimes;
        for (size_t i = 0; i < 5; ++i) {
            std::string second = "2012-01-01 15:05:" + (i < 10 ? "0" + std::to_string(i) : std::to_string(i));
            for (size_t j = 0; j < 5; ++j) {
                inputTimes.emplace_back(second + "." + std::to_string(j), expectLogTimeBase + i, j * 100000000, expectLogTimeNanosecondBase + i * 1000000 + j * 100000);
            }
        }
        StringView timeStrCache = "2012-01-01 15:04:59";
        for (size_t i = 0; i < inputTimes.size(); ++i) {
            auto c = inputTimes[i];
            bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
            APSARA_TEST_EQUAL(ret, true);
            APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
            APSARA_TEST_EQUAL(outTime.tv_nsec, c.exceptedLogTimeNanosecond);
            APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
        }
    }
    { // case: timestamp second
        config.mTimeFormat = "%s";
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));

        time_t expectLogTimeBase = 1484147107;
        long expectLogTimeNanosecondBase = 1484147107000000;
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        std::vector<Case> inputTimes;
        for (size_t i = 0; i < 5; ++i) {
            std::string second = std::to_string(1484147107 + i);
            for (size_t j = 0; j < 5; ++j) {
                inputTimes.emplace_back(std::string(second.data()), expectLogTimeBase + i, 0, expectLogTimeNanosecondBase + i * 1000000);
            }
        }
        StringView timeStrCache = "1484147106";
        for (size_t i = 0; i < inputTimes.size(); ++i) {
            auto c = inputTimes[i];
            bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
            APSARA_TEST_EQUAL(ret, true);
            APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
            APSARA_TEST_EQUAL(outTime.tv_nsec, c.exceptedLogTimeNanosecond);
            APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
        }
    }
    { // case: timestamp nanosecond
        config.mTimeFormat = "%s";
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));

        time_t expectLogTimeBase = 1484147107;
        long expectLogTimeNanosecondBase = 1484147107000000;
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        std::vector<Case> inputTimes;
        for (size_t i = 0; i < 5; ++i) {
            std::string second = std::to_string(1484147107 + i);
            for (size_t j = 0; j < 5; ++j) {
                inputTimes.emplace_back(second + std::to_string(j), expectLogTimeBase + i, j * 100000000, expectLogTimeNanosecondBase + i * 1000000 + j * 100000);
            }
        }
        StringView timeStrCache = "1484147106";
        for (size_t i = 0; i < inputTimes.size(); ++i) {
            auto c = inputTimes[i];
            bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
            APSARA_TEST_EQUAL(ret, true);
            APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
            APSARA_TEST_EQUAL(outTime.tv_nsec, c.exceptedLogTimeNanosecond);
            APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
        }
    }
    { // case: nanosecond in the middle
        config.mTimeFormat = "%H:%M:%S.%f %Y-%m-%d";
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));

        time_t expectLogTimeBase = 1325430300;
        long expectLogTimeNanosecondBase = 1325430300000000;
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        std::vector<Case> inputTimes;
        for (size_t i = 0; i < 5; ++i) {
            std::string second = "15:05:" + (i < 10 ? "0" + std::to_string(i) : std::to_string(i));
            for (size_t j = 0; j < 5; ++j) {
                inputTimes.emplace_back(second + "." + std::to_string(j) + " 2012-01-01", expectLogTimeBase + i, j * 100000000, expectLogTimeNanosecondBase + i * 1000000 + j * 100000);
            }
        }
        StringView timeStrCache = "15:04:59.0 2012-01-01";
        for (size_t i = 0; i < inputTimes.size(); ++i) {
            auto c = inputTimes[i];
            bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
            APSARA_TEST_EQUAL(ret, true);
            APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
            APSARA_TEST_EQUAL(outTime.tv_nsec, c.exceptedLogTimeNanosecond);
            APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
        }
    }
}

void ProcessorParseLogTimeUnittest::TestAdjustTimeZone() {
    struct Case {
        std::string inputTimeStr;
        time_t exceptedLogTime;
        long exceptedLogTimeNanosecond;
        uint64_t exceptedPreciseTimestamp;

        Case(std::string _inputTimeStr,
             time_t _exceptedLogTime,
             long _exceptedLogTimeNanosecond,
             uint64_t _exceptedPreciseTimestamp)
            : inputTimeStr(_inputTimeStr),
              exceptedLogTime(_exceptedLogTime),
              exceptedLogTimeNanosecond(_exceptedLogTimeNanosecond),
              exceptedPreciseTimestamp(_exceptedPreciseTimestamp) {}
    };
    Config config;
    config.mTimeKey = "time";
    config.mAdvancedConfig.mEnablePreciseTimestamp = true;
    config.mAdvancedConfig.mPreciseTimestampUnit = TimeStampUnit::MICROSECOND;
    { // case: UTC
        config.mLogTimeZoneOffsetSecond = -GetLocalTimeZoneOffsetSecond();
        config.mTimeFormat = "%Y-%m-%d %H:%M:%S.%f";
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));

        time_t expectLogTimeBase = 1325430300;
        long expectLogTimeNanosecondBase = 1325430300000000;
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        std::vector<Case> inputTimes;
        for (size_t i = 0; i < 5; ++i) {
            std::string second = "2012-01-01 15:05:" + (i < 10 ? "0" + std::to_string(i) : std::to_string(i));
            for (size_t j = 0; j < 5; ++j) {
                inputTimes.emplace_back(second + "." + std::to_string(j), expectLogTimeBase + i, j * 100000000, expectLogTimeNanosecondBase + i * 1000000 + j * 100000);
            }
        }
        StringView timeStrCache = "2012-01-01 15:04:59";
        for (size_t i = 0; i < inputTimes.size(); ++i) {
            auto c = inputTimes[i];
            bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
            APSARA_TEST_EQUAL(ret, true);
            APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
            APSARA_TEST_EQUAL(outTime.tv_nsec, c.exceptedLogTimeNanosecond);
            APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
        }
    }
    { // case: +7
        config.mLogTimeZoneOffsetSecond = 7 * 3600 - GetLocalTimeZoneOffsetSecond();
        config.mTimeFormat = "%Y-%m-%d %H:%M:%S.%f";
        // run function
        ProcessorParseTimestampNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));

        time_t expectLogTimeBase = 1325405100;
        long expectLogTimeNanosecondBase = 1325405100000000;
        LogtailTime outTime = {0, 0};
        uint64_t preciseTimestamp = 0;
        std::vector<Case> inputTimes;
        for (size_t i = 0; i < 5; ++i) {
            std::string second = "2012-01-01 15:05:" + (i < 10 ? "0" + std::to_string(i) : std::to_string(i));
            for (size_t j = 0; j < 5; ++j) {
                inputTimes.emplace_back(second + "." + std::to_string(j), expectLogTimeBase + i, j * 100000000, expectLogTimeNanosecondBase + i * 1000000 + j * 100000);
            }
        }
        StringView timeStrCache = "2012-01-01 15:04:59";
        for (size_t i = 0; i < inputTimes.size(); ++i) {
            auto c = inputTimes[i];
            bool ret = processor.ParseLogTime(c.inputTimeStr, "/var/log/message", outTime, preciseTimestamp, timeStrCache);
            APSARA_TEST_EQUAL(ret, true);
            APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
            APSARA_TEST_EQUAL(outTime.tv_nsec, c.exceptedLogTimeNanosecond);
            APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
        }
    }
}

} // namespace logtail

UNIT_TEST_MAIN