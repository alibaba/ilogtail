// Copyright 2022 iLogtail Authors
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

#include "unittest/Unittest.h"
#include "log_pb/sls_logs.pb.h"
#include "logger/Logger.h"
#include "common/StringTools.h"
#include "parser/LogParser.h"
#include "reader/LogFileReader.h"
#include <iostream>
#include "common/util.h"
#include "common/TimeUtil.h"
#include <ctime>
#include <cstdlib>
#include "config_manager/ConfigManager.h"
#include <chrono>
#include <cmath>

using namespace std;
using namespace sls_logs;

namespace logtail {

Logger::logger gLogger;

class LogParserUnittest : public ::testing::Test {
public:
public:
    void TestLogParserParseLogTime();
    void TestRegexLogLineParser();

    static void SetUpTestCase() // void Setup()
    {
        // mock data is all timeout, so shut up discard_old flag
        BOOL_FLAG(ilogtail_discard_old_data) = false;
        LOG_INFO(gLogger, ("LogParserUnittest", "setup"));
    }
    static void TearDownTestCase() // void CleanUp()
    {
        LOG_INFO(gLogger, ("LogParserUnittest", "cleanup"));
    }
};

APSARA_UNIT_TEST_CASE(LogParserUnittest, TestLogParserParseLogTime, 0);
APSARA_UNIT_TEST_CASE(LogParserUnittest, TestRegexLogLineParser, 1);

void LogParserUnittest::TestLogParserParseLogTime() {
    struct Case {
        std::string inputTimeStr;
        std::string fmtStr;
        time_t exceptedLogTime;
        uint64_t exceptedPreciseTimestamp;
    };

    vector<Case> inputTimes = {
        {"2012-01-01 15:05:00.123456", "%Y-%m-%d %H:%M:%S.%f", 1325430300, 1325430300000000},
    };


    // Always parse
    auto& c = inputTimes[0];
    LogtailTime outTime;
    ParseLogError error;
    uint64_t preciseTimestamp = 0;
    PreciseTimestampConfig preciseTimestampConfig;
    preciseTimestampConfig.enabled = true;
    preciseTimestampConfig.key = "key";
    preciseTimestampConfig.unit = TimeStampUnit::MICROSECOND;
    // std::string timeStr = "2012-01-01 15:05:00";

    // {
    //     auto start = std::chrono::high_resolution_clock::now();
    //     for (int i = 0; i < 100000; ++i) {
    //         GetPreciseTimestamp(outTime.tv_sec, c.inputTimeStr.substr(timeStr.length()).c_str(), preciseTimestampConfig, 0);
    //     }
    //     auto end = std::chrono::high_resolution_clock::now();
    //     auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    //     std::cout << "precise timestamp cost time: " << time_span.count() << " ms" << std::endl;
    // }

    // {
    //     auto start = std::chrono::high_resolution_clock::now();
    //     for (int i = 0; i < 100000; ++i) {
    //         outTime.tv_nsec = ParseNanosecondAtEnd(c.inputTimeStr.substr(timeStr.length()).c_str());
    //         GetPreciseTimestampFromLogtailTime(outTime, preciseTimestampConfig, 0);
    //     }
    //     auto end = std::chrono::high_resolution_clock::now();
    //     auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    //     std::cout << "strptime cost time: " << time_span.count() << " ms" << std::endl;
    // }

    // {
    //     std::string inputTimeStr = "15:05:07.123456 2012-01-01";
    //     std::string timeFormat = "%H:%M:%S.%f %Y-%m-%d";
    //     std::string emptyString = "asd";
    //     bool ret = LogParser::ParseLogTime("TestData",
    //                                        emptyString,
    //                                        outTime,
    //                                        preciseTimestamp,
    //                                        inputTimeStr,
    //                                        timeFormat.c_str(),
    //                                        preciseTimestampConfig,
    //                                        -1,
    //                                        "",
    //                                        "",
    //                                        "",
    //                                        "",
    //                                        error,
    //                                        0);
    //     APSARA_TEST_EQUAL(ret, true);
    //     std::cout << emptyString << std::endl;
    //     APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
    //     APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp);
    // }

    // auto start = std::chrono::high_resolution_clock::now();
    // for (size_t i = 0; i < 60; ++i) {
    //     std::string second = "2012-01-01 15:05:" + (i < 10 ? "0" + to_string(i) : to_string(i));
    //     for (size_t j = 0; j < 500000; ++j) {
    //         std::string emptyString = "asd";
    //         std::string inputTimeStr = second + "." + to_string(j);
    //         bool ret = LogParser::ParseLogTime("TestData",
    //                                            emptyString,
    //                                            outTime,
    //                                            preciseTimestamp,
    //                                            inputTimeStr,
    //                                            c.fmtStr.c_str(),
    //                                            preciseTimestampConfig,
    //                                            -1,
    //                                            "",
    //                                            "",
    //                                            "",
    //                                            "",
    //                                            error,
    //                                            0);
    //         // APSARA_TEST_EQUAL(ret, true);
    //         // APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime);
    //         // APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp + i);
    //     }
    // }
    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // std::cout << "Always parse cost time: " << time_span.count() << " ms" << std::endl;

    BOOL_FLAG(ilogtail_discard_old_data) = false;
    auto start2 = std::chrono::high_resolution_clock::now();
    std::string preTimeStr = "2012-01-01 15:04:59";
    for (size_t i = 0; i < 60; ++i) {
        std::string second = "2012-01-01 15:05:" + (i < 10 ? "0" + to_string(i) : to_string(i));
        for (size_t j = 0; j < 500000; ++j) {
            std::string inputTimeStr = second + "." + to_string(j);
            bool ret = LogParser::ParseLogTime("TestData",
                                            preTimeStr,
                                            outTime,
                                            preciseTimestamp,
                                            inputTimeStr,
                                            c.fmtStr.c_str(),
                                            preciseTimestampConfig,
                                            -1,
                                            "",
                                            "",
                                            "",
                                            "",
                                            error,
                                            0);
            // APSARA_TEST_EQUAL(ret, true);
            // APSARA_TEST_EQUAL(preTimeStr, second);
            // APSARA_TEST_EQUAL(outTime.tv_sec, c.exceptedLogTime + i);
            // APSARA_TEST_EQUAL(preciseTimestamp, c.exceptedPreciseTimestamp + i * 1000000 + (int)(j * pow(10, 5 - j / 10)));
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto time_span2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    std::cout << "Second parse cost time: " << time_span2.count() << " ms" << std::endl;
}

void LogParserUnittest::TestRegexLogLineParser() {
    // PreciseTimestampConfig preciseTimestampConfig;
    // preciseTimestampConfig.enabled = true;
    // preciseTimestampConfig.key = "precise_timestamp_key";

    // const char* timeFormat = "%Y-%m-%d/%H:%M:%S.%f";
    // boost::regex rightReg = boost::regex("([\\d\\.]+) \\S+ \\S+ \\[(\\S+) \\S+\\] \"(\\w+) ([^\\\"]*)\" ([\\d\\.]+) (\\d+) (\\d+) (\\d+|-) \"([^\\\"]*)\" \"([^\\\"]*)\"");
    // std::string logFirstPart = "127.0.0.1 - - [";
    // std::string logSecondPart = " +0800] \"POST /PutData?Category=YunOsAccountOpLog\" 0.024 18204 200 42 \"-\" \"aliyun-sdk-java\"";
    // vector<string> keys;
    // keys.push_back("ip");
    // keys.push_back("time");
    // keys.push_back("method");
    // keys.push_back("url");
    // keys.push_back("request_time");
    // keys.push_back("request_length");
    // keys.push_back("status");
    // keys.push_back("length");
    // keys.push_back("ref_url");
    // keys.push_back("browser");

    // // case 0: reg parse failed.
    // string metricName = "ace";
    // ParseLogError error;
    // uint32_t logGroupSize;
    // bool flag;

    // auto start = std::chrono::high_resolution_clock::now();
    // for (size_t i = 0; i < 60; ++i) {
    //     LogGroup logGroup;
    //     std::string second = "2023-07-24/11:22:" + (i < 10 ? "0" + to_string(i) : to_string(i));
    //     for (size_t j = 0; j < 500000; ++j) {
    //         LogtailTime logTime;
    //         std::string bufferStr = logFirstPart + second + "." + to_string(j) + logSecondPart;
    //         std::string invalidLastLog = "asdfasdf.";
    //         flag = LogParser::RegexLogLineParser(bufferStr.c_str(),
    //                                             rightReg,
    //                                             logGroup,
    //                                             true,
    //                                             keys,
    //                                             metricName,
    //                                             timeFormat,
    //                                             preciseTimestampConfig,
    //                                             1,
    //                                             invalidLastLog,
    //                                             logTime,
    //                                             -1,
    //                                             "",
    //                                             "",
    //                                             "",
    //                                             error,
    //                                             logGroupSize,
    //                                             0);
    //         // APSARA_TEST_EQUAL(flag, true);
    //     }
    // }
    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // std::cout << "Always regex parse cost time: " << time_span.count() << " ms" << std::endl;
    
    // BOOL_FLAG(ilogtail_discard_old_data) = false;
    // auto start2 = std::chrono::high_resolution_clock::now();
    // std::string lastLogTimeStr = "2023-07-24/11:21:59";
    // for (size_t i = 0; i < 60; ++i) {
    //     std::string second = "2023-07-24/11:22:" + (i < 10 ? "0" + to_string(i) : to_string(i));
    //     LogGroup logGroup;
    //     for (size_t j = 0; j < 500000; ++j) {
    //         LogtailTime logTime;
    //         std::string bufferStr = logFirstPart + second + "." + to_string(j) + logSecondPart;
    //         flag = LogParser::RegexLogLineParser(bufferStr.c_str(),
    //                                              rightReg,
    //                                              logGroup,
    //                                              true,
    //                                              keys,
    //                                              metricName,
    //                                              timeFormat,
    //                                              preciseTimestampConfig,
    //                                              1,
    //                                              lastLogTimeStr,
    //                                              logTime,
    //                                              -1,
    //                                              "",
    //                                              "",
    //                                              "",
    //                                              error,
    //                                              logGroupSize,
    //                                              0);
    //         // APSARA_TEST_EQUAL(flag, true);
    //         // APSARA_TEST_EQUAL(lastLogTimeStr, second);
    //     }
    // }
    // auto end2 = std::chrono::high_resolution_clock::now();
    // auto time_span2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    // std::cout << "Second regex parse cost time: " << time_span2.count() << " ms" << std::endl;
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    logtail::gLogger = logtail::Logger::Instance().GetLogger("/logger/global/test");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}