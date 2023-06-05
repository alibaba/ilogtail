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
#if defined(__linux__)
#include <unistd.h>
#include <iconv.h>
#elif defined(_MSC_VER)
#include <io.h>
#include <fcntl.h>
#endif
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "reader/LogFileReader.h"
#include "common/util.h"
#include "common/Flags.h"
#include "parser/DelimiterModeFsmParser.h"
#include "logger/Logger.h"
#include "log_pb/sls_logs.pb.h"
#if defined(__linux__)
#include "unittest/UnittestHelper.h"
#endif

using namespace std;
using namespace sls_logs;

DECLARE_FLAG_INT64(first_read_endure_bytes);

namespace logtail {

class LogFileReaderUnittest : public ::testing::Test {
    const std::string& PS = PATH_SEPARATOR;

    static const string LOG_BEGIN_TIME;
    static const string LOG_BEGIN_REGEX;
    static const string NEXT_LINE;
    static const size_t BUFFER_SIZE;
    static const size_t SIGNATURE_CHAR_COUNT;
    static string gRootDir;

#if defined(_MSC_VER)
    // Copy from Internet, to replace iconv on Windows.
    static std::string UTF8ToGBK(const char* strUTF8) {
        int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
        wchar_t* wszGBK = new wchar_t[len + 1];
        memset(wszGBK, 0, len * 2 + 2);
        MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, wszGBK, len);
        len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
        char* szGBK = new char[len + 1];
        memset(szGBK, 0, len + 1);
        WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
        string strTemp(szGBK);
        if (wszGBK)
            delete[] wszGBK;
        if (szGBK)
            delete[] szGBK;
        return strTemp;
    }
#endif

public:
    void TestLogSplit();
};

APSARA_UNIT_TEST_CASE(LogFileReaderUnittest, TestLogSplit, 1);


void LogFileReaderUnittest::TestLogSplit() {
    LOG_INFO(sLogger, ("TestLogSplit() begin", time(NULL)));
    // this is call after GetRawData(), so buffer must end with '\0'
    // case 0: multiple line
    CommonRegLogFileReader logFileReader("100_proj", "", "", "", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    vector<int32_t> index;
    string multipleLog = GenerateRawData(3, index, 10);
    int32_t multipleLogSize = multipleLog.size();
    char* buffer = new char[multipleLogSize + 1];
    strcpy(buffer, multipleLog.c_str());
    buffer[multipleLogSize - 1] = '\0';
    int32_t lineFeed;
    vector<int32_t> splitIndex = logFileReader.LogSplit(buffer, multipleLogSize, lineFeed, true);
    APSARA_TEST_EQUAL(index.size(), splitIndex.size());
    for (size_t i = 0; i < index.size() && i < splitIndex.size(); ++i) {
        APSARA_TEST_EQUAL(index[i], splitIndex[i]);
    }
    APSARA_TEST_TRUE(lineFeed >= (int32_t)splitIndex.size());
    delete[] buffer;

    // case 1: single line
    string singleLog = LOG_BEGIN_TIME + "single log";
    int32_t singleLogSize = singleLog.size();
    buffer = new char[singleLogSize + 1];
    strcpy(buffer, singleLog.c_str());
    buffer[singleLogSize - 1] = '\0';
    splitIndex = logFileReader.LogSplit(buffer, singleLogSize, lineFeed, true);
    APSARA_TEST_EQUAL(1, splitIndex.size());
    APSARA_TEST_EQUAL(0, splitIndex[0]);
    APSARA_TEST_EQUAL(1, lineFeed);
    delete[] buffer;

    // case 2: invalid regex beginning
    string invalidLog = "xx" + LOG_BEGIN_TIME + "invalid log 1\nyy" + LOG_BEGIN_TIME + "invalid log 2\n";
    int32_t invalidLogSize = invalidLog.size();
    buffer = new char[invalidLogSize + 1];
    strcpy(buffer, invalidLog.c_str());
    buffer[invalidLogSize - 1] = '\0';
    splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed, true);
    APSARA_TEST_EQUAL(0, splitIndex.size());
    APSARA_TEST_EQUAL(2, lineFeed);
    delete[] buffer;


    // case 3: invalid begin one regex beginning
    {
        string invalidLog = "xx" + LOG_BEGIN_TIME + "invalid log 1\n" + LOG_BEGIN_TIME + "invalid log 2\n";
        int32_t invalidLogSize = invalidLog.size();
        buffer = new char[invalidLogSize + 1];
        strcpy(buffer, invalidLog.c_str());
        buffer[invalidLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed, false);
        APSARA_TEST_EQUAL(2, splitIndex.size());
        APSARA_TEST_EQUAL(2, lineFeed);
        delete[] buffer;
    }

    // case 4: invalid end one regex beginning
    {
        string invalidLog = LOG_BEGIN_TIME + "invalid log 1\nyy" + LOG_BEGIN_TIME + "invalid log 2\n";
        int32_t invalidLogSize = invalidLog.size();
        buffer = new char[invalidLogSize + 1];
        strcpy(buffer, invalidLog.c_str());
        buffer[invalidLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed, true);
        APSARA_TEST_EQUAL(1, splitIndex.size());
        APSARA_TEST_EQUAL(2, lineFeed);
        delete[] buffer;
    }

    LOG_INFO(sLogger, ("TestLogSplit() end", time(NULL)));
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
