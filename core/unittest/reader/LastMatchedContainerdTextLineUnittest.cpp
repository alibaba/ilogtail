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

#include "common/FileSystemUtil.h"
#include "reader/LogFileReader.h"
#include "reader/SourceBuffer.h"
#include "unittest/Unittest.h"

namespace logtail {

const std::string LOG_PART = "2021-08-25T07:00:00.000000000Z stdout P ";
const std::string LOG_FULL = "2021-08-25T07:00:00.000000000Z stdout F ";
const std::string LOG_FULL_NOT_FOUND = "2021-08-25T07:00:00.000000000Z stdout ";
const std::string LOG_ERROR = "2021-08-25T07:00:00.000000000Z stdout";

const std::string LOG_BEGIN_STRING = "Exception in thread \"main\" java.lang.NullPointerException";
const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
const std::string LOG_CONTINUE_REGEX = R"(\s+at\s.*)";
const std::string LOG_END_STRING = "    ...23 more";
const std::string LOG_END_REGEX = R"(\s*\.\.\.\d+ more)";
const std::string LOG_UNMATCH = "unmatch log";

class LastMatchedContainerdTextLineUnittest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        logPathDir = GetProcessExecutionDir();
        if (PATH_SEPARATOR[0] == logPathDir.back()) {
            logPathDir.resize(logPathDir.size() - 1);
        }
        logPathDir += PATH_SEPARATOR + "testDataSet" + PATH_SEPARATOR + "LogFileReaderUnittest";
        gbkFile = "gbk.txt";
        utf8File = "utf8.txt"; // content of utf8.txt is equivalent to gbk.txt
    }

    static void TearDownTestCase() {
        remove(gbkFile.c_str());
        remove(utf8File.c_str());
    }

    void SetUp() override {
        std::string filepath = logPathDir + PATH_SEPARATOR + utf8File;
        std::unique_ptr<FILE, decltype(&std::fclose)> fp(std::fopen(filepath.c_str(), "r"), &std::fclose);
        if (!fp.get()) {
            return;
        }
        std::fseek(fp.get(), 0, SEEK_END);
        long filesize = std::ftell(fp.get());
        std::fseek(fp.get(), 0, SEEK_SET);
        expectedContent.reset(new char[filesize + 1]);
        fread(expectedContent.get(), filesize, 1, fp.get());
        expectedContent[filesize] = '\0';
        for (long i = filesize - 1; i >= 0; --i) {
            if (expectedContent[i] == '\n') {
                expectedContent[i] = 0;
                break;
            };
        }
    }

    void TestSingleline();
    void TestMultiline();
    void TestLastContainerdTextLinePos();
    void TestGetLastLineData();

    std::unique_ptr<char[]> expectedContent;
    FileReaderOptions readerOpts;
    PipelineContext ctx;
    static std::string logPathDir;
    static std::string gbkFile;
    static std::string utf8File;
};

UNIT_TEST_CASE(LastMatchedContainerdTextLineUnittest, TestLastContainerdTextLinePos);
UNIT_TEST_CASE(LastMatchedContainerdTextLineUnittest, TestGetLastLineData);

std::string LastMatchedContainerdTextLineUnittest::logPathDir;
std::string LastMatchedContainerdTextLineUnittest::gbkFile;
std::string LastMatchedContainerdTextLineUnittest::utf8File;

void LastMatchedContainerdTextLineUnittest::TestLastContainerdTextLinePos() {
    MultilineOptions multilineOpts;
    LogFileReader logFileReader(
        logPathDir, utf8File, DevInode(), std::make_pair(&readerOpts, &ctx), std::make_pair(&multilineOpts, &ctx));
    logFileReader.mFileLogFormat = LogFileReader::LogFormat::CONTAINERD_TEXT;
    // case: F + P + P
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456";
        std::string expectedLog = LOG_FULL + "789\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(expectedLog.size()), size);
        APSARA_TEST_EQUAL(2, rollbackLineFeedCount);
    }
    // case: F + P + P + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n";
        std::string expectedLog = LOG_FULL + "789\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(expectedLog.size()), size);
        APSARA_TEST_EQUAL(2, rollbackLineFeedCount);
    }

    // case: F + P + P + F
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";
        std::string expectedLog = LOG_FULL + "789\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(expectedLog.size()), size);
        APSARA_TEST_EQUAL(3, rollbackLineFeedCount);
    }
    // case: F + P + P + F + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(testLog.size()), size);
        APSARA_TEST_EQUAL(0, rollbackLineFeedCount);
    }

    // F + errorLog
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789";
        std::string expectedLog = LOG_FULL + "456\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(expectedLog.size()), size);
        APSARA_TEST_EQUAL(1, rollbackLineFeedCount);
    }
    // F + errorLog + '\n'
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";
        std::string expectedLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(expectedLog.size()), size);
        APSARA_TEST_EQUAL(0, rollbackLineFeedCount);
    }

    // case: P + P + F
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(0, size);
        APSARA_TEST_EQUAL(3, rollbackLineFeedCount);
    }
    // case: P + P + F + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(testLog.size()), size);
        APSARA_TEST_EQUAL(0, rollbackLineFeedCount);
    }

    // case: P + P + error
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(0, size);
        APSARA_TEST_EQUAL(3, rollbackLineFeedCount);
    }
    // case: P + P + error + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";
        std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t size = logFileReader.LastContainerdTextLinePos(
            const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL(int(expectedLog.size()), size);
        APSARA_TEST_EQUAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedContainerdTextLineUnittest::TestGetLastLineData() {
    MultilineOptions multilineOpts;
    LogFileReader logFileReader(
        logPathDir, utf8File, DevInode(), std::make_pair(&readerOpts, &ctx), std::make_pair(&multilineOpts, &ctx));
    logFileReader.mFileLogFormat = LogFileReader::LogFormat::CONTAINERD_TEXT;

    // case: F + P + P
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456";
        int endPs = testLog.size() - 1;
        int begPs = testLog.size() - 2;

        std::string expectedRemainingLogs = LOG_FULL + "789\n";
        std::string expectedLog = "12345";

        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begPs, endPs);
        APSARA_TEST_EQUAL(line.data.to_string(), expectedLog);
        APSARA_TEST_EQUAL(begPs, expectedRemainingLogs.size());
    }
    // case: F + P + P + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n";
        int endPs = testLog.size() - 1;
        int begPs = testLog.size() - 2;

        std::string expectedRemainingLogs = LOG_FULL + "789\n";
        std::string expectedLog = "123456";

        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begPs, endPs);
        APSARA_TEST_EQUAL(line.data.to_string(), expectedLog);
        APSARA_TEST_EQUAL(begPs, expectedRemainingLogs.size());
    }

    // case: F + P + P + F
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";
        int endPs = testLog.size() - 1;
        int begPs = testLog.size() - 2;

        std::string expectedRemainingLogs = LOG_FULL + "789\n";
        std::string expectedLog = "12345678";

        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begPs, endPs);
        APSARA_TEST_EQUAL(line.data.to_string(), expectedLog);
        APSARA_TEST_EQUAL(begPs, expectedRemainingLogs.size());
    }
    // case: F + P + P + F + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";
        int endPs = testLog.size() - 1;
        int begPs = testLog.size() - 2;

        std::string expectedRemainingLogs = LOG_FULL + "789\n";
        std::string expectedLog = "123456789";

        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begPs, endPs);
        APSARA_TEST_EQUAL(line.data.to_string(), expectedLog);
        APSARA_TEST_EQUAL(begPs, expectedRemainingLogs.size());
    }

    // F + errorLog
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789";
        int endPs = testLog.size() - 1;
        int begPs = testLog.size() - 2;

        std::string expectedRemainingLogs = LOG_FULL + "456\n";
        std::string expectedLog = LOG_ERROR + "78";

        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begPs, endPs);
        APSARA_TEST_EQUAL(line.data.to_string(), expectedLog);
        APSARA_TEST_EQUAL(begPs, expectedRemainingLogs.size());
    }
    // F + errorLog + '\n'
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";
        int endPs = testLog.size() - 1;
        int begPs = testLog.size() - 2;

        std::string expectedRemainingLogs = LOG_FULL + "456\n";
        std::string expectedLog = LOG_ERROR + "789";

        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begPs, endPs);
        APSARA_TEST_EQUAL(line.data.to_string(), expectedLog);
        APSARA_TEST_EQUAL(begPs, expectedRemainingLogs.size());
    }

    // case: P + P + F
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";
        int endPs = testLog.size() - 1;
        int begPs = testLog.size() - 2;

        std::string expectedRemainingLogs = "";
        std::string expectedLog = "12345678";

        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begPs, endPs);
        APSARA_TEST_EQUAL(line.data.to_string(), expectedLog);
        APSARA_TEST_EQUAL(begPs, expectedRemainingLogs.size());
    }
    // // case: P + P + F + '\n'
    // {
    //     std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";
    //     int32_t rollbackLineFeedCount = 0;
    //     int32_t size = logFileReader.LastContainerdTextLinePos(
    //         const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
    //     APSARA_TEST_EQUAL(int(testLog.size()), size);
    //     APSARA_TEST_EQUAL(0, rollbackLineFeedCount);
    // }

    // // case: P + P + error
    // {
    //     std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789";
    //     int32_t rollbackLineFeedCount = 0;
    //     int32_t size = logFileReader.LastContainerdTextLinePos(
    //         const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
    //     APSARA_TEST_EQUAL(0, size);
    //     APSARA_TEST_EQUAL(3, rollbackLineFeedCount);
    // }
    // // case: P + P + error + '\n'
    // {
    //     std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";
    //     std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";
    //     int32_t rollbackLineFeedCount = 0;
    //     int32_t size = logFileReader.LastContainerdTextLinePos(
    //         const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
    //     APSARA_TEST_EQUAL(int(expectedLog.size()), size);
    //     APSARA_TEST_EQUAL(0, rollbackLineFeedCount);
    // }
}

} // namespace logtail

UNIT_TEST_MAIN
