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

    void TestLastContainerdTextLineNoMerge();
    void TestLastContainerdTextLineMerge();
    void TestLastContainerdTextLineNoMergeSingleLine();

    std::unique_ptr<char[]> expectedContent;
    FileReaderOptions readerOpts;
    PipelineContext ctx;
    static std::string logPathDir;
    static std::string gbkFile;
    static std::string utf8File;
};

UNIT_TEST_CASE(LastMatchedContainerdTextLineUnittest, TestLastContainerdTextLineNoMerge);
UNIT_TEST_CASE(LastMatchedContainerdTextLineUnittest, TestLastContainerdTextLineMerge);
UNIT_TEST_CASE(LastMatchedContainerdTextLineUnittest, TestLastContainerdTextLineNoMergeSingleLine);

std::string LastMatchedContainerdTextLineUnittest::logPathDir;
std::string LastMatchedContainerdTextLineUnittest::gbkFile;
std::string LastMatchedContainerdTextLineUnittest::utf8File;

void LastMatchedContainerdTextLineUnittest::TestLastContainerdTextLineNoMergeSingleLine() {
    MultilineOptions multilineOpts;
    LogFileReader logFileReader(
        logPathDir, utf8File, DevInode(), std::make_pair(&readerOpts, &ctx), std::make_pair(&multilineOpts, &ctx));
    logFileReader.mFileLogFormat = LogFileReader::LogFormat::CONTAINERD_TEXT;
    // 异常情况+有回车
    {
        // case: PartLogFlag存在，第三个空格存在但空格后无内容
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout P \n";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("", line.data.to_string());
        }
        // case: PartLogFlag存在，第三个空格不存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout P\n";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("P", line.data.to_string());
        }
        // case: PartLogFlag不存在，第二个空格存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout \n";
            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("", line.data.to_string());
        }
        // case: 第二个空格不存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout\n";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("2024-01-05T23:28:06.818486411+08:00 stdout", line.data.to_string());
        }
        // case: 第一个空格不存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00stdout\n";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("2024-01-05T23:28:06.818486411+08:00stdout", line.data.to_string());
        }
    }
    // 异常情况+无回车
    {
        // case: PartLogFlag存在，第三个空格存在但空格后无内容
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout P ";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("P", line.data.to_string());
        }
        // case: PartLogFlag存在，第三个空格不存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout P";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("", line.data.to_string());
        }
        // case: PartLogFlag不存在，第二个空格存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout ";
            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("2024-01-05T23:28:06.818486411+08:00 stdout", line.data.to_string());
        }
        // case: 第二个空格不存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00 stdout";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("2024-01-05T23:28:06.818486411+08:00 stdou", line.data.to_string());
        }
        // case: 第一个空格不存在
        {
            std::string testLog = "2024-01-05T23:28:06.818486411+08:00stdout";

            int32_t endPs = testLog.size() - 1;
            int32_t begin = endPs - 1;
            LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
            APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
            APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
            APSARA_TEST_EQUAL("2024-01-05T23:28:06.818486411+08:00stdou", line.data.to_string());
        }
    }
    // case: F + P + P
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("45", line.data.to_string());
    }
    // case: F + P + P + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("456", line.data.to_string());
    }

    // case: F + P + P + F
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("78", line.data.to_string());
    }
    // case: F + P + P + F + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("789", line.data.to_string());
    }

    // F + errorLog
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "78", line.data.to_string());
    }
    // F + errorLog + '\n'
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "789", line.data.to_string());
    }

    // case: P + P + F
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("78", line.data.to_string());
    }
    // case: P + P + F + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("789", line.data.to_string());
    }

    // case: P + P + error
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "78", line.data.to_string());
    }
    // case: P + P + error + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "789", line.data.to_string());
    }
    // case: P + P
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("45", line.data.to_string());
    }
    // case: P + P + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false, true);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("456", line.data.to_string());
    }
}

void LastMatchedContainerdTextLineUnittest::TestLastContainerdTextLineNoMerge() {
    MultilineOptions multilineOpts;
    LogFileReader logFileReader(
        logPathDir, utf8File, DevInode(), std::make_pair(&readerOpts, &ctx), std::make_pair(&multilineOpts, &ctx));
    logFileReader.mFileLogFormat = LogFileReader::LogFormat::CONTAINERD_TEXT;
    // case: F + P + P
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456";
        std::string expectedLog = LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("789", line.data.to_string());
    }
    // case: F + P + P + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n";
        std::string expectedLog = LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("789", line.data.to_string());
    }

    // case: F + P + P + F
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("78", line.data.to_string());
    }
    // case: F + P + P + F + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("789", line.data.to_string());
    }

    // F + errorLog
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789";
        std::string expectedLog = LOG_FULL + "456\n" + LOG_ERROR + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "78", line.data.to_string());
    }
    // F + errorLog + '\n'
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";
        std::string expectedLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "789", line.data.to_string());
    }

    // case: P + P + F
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("78", line.data.to_string());
    }
    // case: P + P + F + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("789", line.data.to_string());
    }

    // case: P + P + error
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789";
        std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "78", line.data.to_string());
    }
    // case: P + P + error + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";
        std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(0, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "789", line.data.to_string());
    }
    // case: P + P
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456";
        std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(0, line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("", line.data.to_string());
    }
    // case: P + P + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n";
        std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, false);
        APSARA_TEST_EQUAL(0, line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("", line.data.to_string());
    }
}

void LastMatchedContainerdTextLineUnittest::TestLastContainerdTextLineMerge() {
    MultilineOptions multilineOpts;
    LogFileReader logFileReader(
        logPathDir, utf8File, DevInode(), std::make_pair(&readerOpts, &ctx), std::make_pair(&multilineOpts, &ctx));
    logFileReader.mFileLogFormat = LogFileReader::LogFormat::CONTAINERD_TEXT;
    // case: F + P + P
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456";
        std::string expectedLog = LOG_FULL + "789\n" + LOG_PART + "123\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("12345", line.data.to_string());
    }
    // case: F + P + P + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n";
        std::string expectedLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("123456", line.data.to_string());
    }

    // case: F + P + P + F
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";
        std::string expectedLog = LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(3, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("12345678", line.data.to_string());
    }
    // case: F + P + P + F + '\n'
    {
        std::string testLog = LOG_FULL + "789\n" + LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(3, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("123456789", line.data.to_string());
    }

    // F + errorLog
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789";
        std::string expectedLog = LOG_FULL + "456\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "78", line.data.to_string());
    }
    // F + errorLog + '\n'
    {
        std::string testLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";
        std::string expectedLog = LOG_FULL + "456\n" + LOG_ERROR + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(1, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL(LOG_ERROR + "789", line.data.to_string());
    }

    // case: P + P + F
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(3, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("12345678", line.data.to_string());
    }
    // case: P + P + F + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_FULL + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(testLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(3, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("123456789", line.data.to_string());
    }

    // case: P + P + error
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(3, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("123456" + LOG_ERROR + "78", line.data.to_string());
    }
    // case: P + P + error + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";
        std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n" + LOG_ERROR + "789\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(int(expectedLog.size()), line.lineEnd + 1);
        APSARA_TEST_EQUAL(3, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("123456" + LOG_ERROR + "789", line.data.to_string());
    }
    // case: P + P
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(0, line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("12345", line.data.to_string());
    }
    // case: P + P + '\n'
    {
        std::string testLog = LOG_PART + "123\n" + LOG_PART + "456\n";
        std::string expectedLog = LOG_PART + "123\n" + LOG_PART + "456\n";

        int32_t endPs = testLog.size() - 1;
        int32_t begin = endPs - 1;
        LineInfo line = logFileReader.GetLastLineData(const_cast<char*>(testLog.data()), begin, endPs, true);
        // APSARA_TEST_EQUAL(0, line.lineEnd + 1);
        APSARA_TEST_EQUAL(2, line.rollbackLineFeedCount);
        APSARA_TEST_EQUAL("123456", line.data.to_string());
    }
}

} // namespace logtail

UNIT_TEST_MAIN
