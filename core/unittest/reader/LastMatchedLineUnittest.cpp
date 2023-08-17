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

#include "unittest/Unittest.h"
#include "reader/CommonRegLogFileReader.h"
#include "reader/SourceBuffer.h"
#include "common/FileSystemUtil.h"

namespace logtail {

const std::string LOG_BEGIN_STRING = "Exception in thread \"main\" java.lang.NullPointerException";
const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
const std::string LOG_CONTINUE_REGEX = R"(\s+at\s.*)";
const std::string LOG_END_STRING = "    ...23 more";
const std::string LOG_END_REGEX = R"(\s*\.\.\.\d+ more)";
const std::string LOG_UNMATCH = "unmatch log";
const std::string UNMATCH_DISCARD = "discard";
const std::string UNMATCH_SINGLELINE = "singleline";
const std::string UNMATCH_APPEND = "append";
const std::string UNMATCH_PREPEND = "prepend";

class LastMatchedLineUnittest : public ::testing::Test {
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

    std::string projectName = "projectName";
    std::string category = "logstoreName";
    std::string timeFormat = "";
    std::string topicFormat = "";
    std::string groupTopic = "";
    std::unique_ptr<char[]> expectedContent;
    static std::string logPathDir;
    static std::string gbkFile;
    static std::string utf8File;
};

UNIT_TEST_CASE(LastMatchedLineUnittest, TestSingleline);
UNIT_TEST_CASE(LastMatchedLineUnittest, TestMultiline);

std::string LastMatchedLineUnittest::logPathDir;
std::string LastMatchedLineUnittest::gbkFile;
std::string LastMatchedLineUnittest::utf8File;

void LastMatchedLineUnittest::TestSingleline() {
    CommonRegLogFileReader logFileReader(projectName,
                                         category,
                                         logPathDir,
                                         utf8File,
                                         INT32_FLAG(default_tail_limit_kb),
                                         timeFormat,
                                         topicFormat,
                                         groupTopic);
    { // case single line
        std::string line1 = "first.";
        std::string line2 = "second.";
        std::string line3 = "third.";
        std::string expectMatch = line1 + '\n' + line2 + '\n' + line3 + '\n';
        std::string testLog = expectMatch;
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(expectMatch.size(), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case single line, buffer size not big enough (no new line at the end of line)
        std::string line1 = "first.";
        std::string line2 = "second.";
        std::string line3 = "third_part.";
        std::string expectMatch = line1 + '\n' + line2 + '\n';
        std::string testLog = expectMatch + line3;
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(expectMatch.size(), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case single line, cannot be split, buffer size not big enough (no new line at the end of line)
        std::string testLog = "first.";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(0, matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), "");
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
}

void LastMatchedLineUnittest::TestMultiline() {
    CommonRegLogFileReader logFileReader(projectName,
                                         category,
                                         logPathDir,
                                         utf8File,
                                         INT32_FLAG(default_tail_limit_kb),
                                         timeFormat,
                                         topicFormat,
                                         groupTopic);
    int32_t rollbackLineFeedCount = 0;
    { // case multi line
        logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, "", "", "append");
        std::vector<int32_t> index;
        std::string firstLog = LOG_BEGIN_STRING + "first.\nmultiline1\nmultiline2";
        std::string secondLog = LOG_BEGIN_STRING + "second.\nmultiline1\nmultiline2";
        std::string expectMatch = firstLog + '\n';
        std::string testLog = expectMatch + secondLog + '\n';
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
    { // case multi line, buffer size not big enough (no new line at the end of line)
        std::vector<int32_t> index;
        std::string firstLog = LOG_BEGIN_STRING + "first.\nmultiline1\nmultiline2";
        std::string secondLog = LOG_BEGIN_STRING + "second.\nmultiline1\nmultiline2";
        std::string expectMatch = firstLog + '\n';
        std::string testLog = expectMatch + secondLog;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
    { // case multi line not match
        std::string testLog2 = "log begin does not match.\nlog begin does not match.\nlog begin does not match.\n";
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog2.data()), testLog2.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(0, matchSize);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
    { // case multi line not match, buffer size not big enough (no new line at the end of line)
        std::string testLog2 = "log begin does not match.\nlog begin does not match.\nlog begin does not";
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog2.data()), testLog2.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(0, matchSize);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
}

class LastMatchedLineDiscardUnmatchUnittest : public ::testing::Test {
public:
    void TestLastMatchedLineWithBeginContinue();
    void TestLastMatchedLineWithBeginEnd();
    void TestLastMatchedLineWithBegin();
    void TestLastMatchedLineWithContinueEnd();
    void TestLastMatchedLineWithEnd();
};

UNIT_TEST_CASE(LastMatchedLineDiscardUnmatchUnittest, TestLastMatchedLineWithBeginContinue);
UNIT_TEST_CASE(LastMatchedLineDiscardUnmatchUnittest, TestLastMatchedLineWithBeginEnd);
UNIT_TEST_CASE(LastMatchedLineDiscardUnmatchUnittest, TestLastMatchedLineWithBegin);
UNIT_TEST_CASE(LastMatchedLineDiscardUnmatchUnittest, TestLastMatchedLineWithContinueEnd);
UNIT_TEST_CASE(LastMatchedLineDiscardUnmatchUnittest, TestLastMatchedLineWithEnd);

void LastMatchedLineDiscardUnmatchUnittest::TestLastMatchedLineWithBeginContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, LOG_CONTINUE_REGEX, "", UNMATCH_DISCARD);
    { // case: end with begin continue
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING+ '\n' ;
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(2, rollbackLineFeedCount);
    }
    { // case: end with begin
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n';
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineDiscardUnmatchUnittest::TestLastMatchedLineWithBeginEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, "", LOG_END_REGEX, UNMATCH_DISCARD);
    { // case: end with begin end
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_UNMATCH + "\n" + LOG_END_STRING + '\n';
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case: end with begin
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n';
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineDiscardUnmatchUnittest::TestLastMatchedLineWithBegin() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, "", "", UNMATCH_DISCARD);
    { // case: end with begin
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n';
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineDiscardUnmatchUnittest::TestLastMatchedLineWithContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy("", LOG_CONTINUE_REGEX, LOG_END_REGEX, UNMATCH_DISCARD);
    { // case: end with continue end
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + '\n';
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case: end with continue
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + '\n';
        std::string testLog = expectMatch + LOG_CONTINUE_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineDiscardUnmatchUnittest::TestLastMatchedLineWithEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy("", "", LOG_END_REGEX, UNMATCH_DISCARD);
    { // case: end with end
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_UNMATCH + "\n" + LOG_END_STRING + '\n';
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch =  LOG_UNMATCH + "\n" + LOG_UNMATCH + "\n" + LOG_END_STRING + '\n' + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

class LastMatchedLineSinglelineUnmatchUnittest : public ::testing::Test {
public:
    void TestLastMatchedLineWithBeginContinue();
    void TestLastMatchedLineWithBeginEnd();
    void TestLastMatchedLineWithBegin();
    void TestLastMatchedLineWithContinueEnd();
    void TestLastMatchedLineWithEnd();
};

UNIT_TEST_CASE(LastMatchedLineSinglelineUnmatchUnittest, TestLastMatchedLineWithBeginContinue);
UNIT_TEST_CASE(LastMatchedLineSinglelineUnmatchUnittest, TestLastMatchedLineWithBeginEnd);
UNIT_TEST_CASE(LastMatchedLineSinglelineUnmatchUnittest, TestLastMatchedLineWithBegin);
UNIT_TEST_CASE(LastMatchedLineSinglelineUnmatchUnittest, TestLastMatchedLineWithContinueEnd);
UNIT_TEST_CASE(LastMatchedLineSinglelineUnmatchUnittest, TestLastMatchedLineWithEnd);

void LastMatchedLineSinglelineUnmatchUnittest::TestLastMatchedLineWithBeginContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, LOG_CONTINUE_REGEX, "", UNMATCH_SINGLELINE);
    { // case: end with begin continue
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING+ '\n' ;
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(2, rollbackLineFeedCount);
    }
    { // case: end with begin
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n';
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineSinglelineUnmatchUnittest::TestLastMatchedLineWithBeginEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, "", LOG_END_REGEX, UNMATCH_SINGLELINE);
    { // case: end with begin end
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_UNMATCH + "\n" + LOG_END_STRING + '\n';
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case: end with begin
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n';
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineSinglelineUnmatchUnittest::TestLastMatchedLineWithBegin() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, "", "", UNMATCH_SINGLELINE);
    { // case: end with begin
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n';
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineSinglelineUnmatchUnittest::TestLastMatchedLineWithContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy("", LOG_CONTINUE_REGEX, LOG_END_REGEX, UNMATCH_SINGLELINE);
    { // case: end with continue end
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + '\n';
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case: end with continue
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + '\n';
        std::string testLog = expectMatch + LOG_CONTINUE_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

void LastMatchedLineSinglelineUnmatchUnittest::TestLastMatchedLineWithEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy("", "", LOG_END_REGEX, UNMATCH_SINGLELINE);
    { // case: end with end
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_UNMATCH + "\n" + LOG_END_STRING + '\n';
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch =  LOG_UNMATCH + "\n" + LOG_UNMATCH + "\n" + LOG_END_STRING + '\n' + LOG_UNMATCH + "\n";
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
}

class LastMatchedLineAppendUnmatchUnittest : public ::testing::Test {
public:
    void TestLastMatchedLineWithBegin();
};

UNIT_TEST_CASE(LastMatchedLineAppendUnmatchUnittest, TestLastMatchedLineWithBegin);

void LastMatchedLineAppendUnmatchUnittest::TestLastMatchedLineWithBegin() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy(LOG_BEGIN_REGEX, "", "", UNMATCH_APPEND);
    { // case: end with begin
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_UNMATCH + "\n";
        std::string testLog = expectMatch + LOG_BEGIN_STRING + "\n" + LOG_UNMATCH + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(2, rollbackLineFeedCount);
    }
}

class LastMatchedLinePrependUnmatchUnittest : public ::testing::Test {
public:
    void TestLastMatchedLineWithEnd();
};

UNIT_TEST_CASE(LastMatchedLinePrependUnmatchUnittest, TestLastMatchedLineWithEnd);

void LastMatchedLinePrependUnmatchUnittest::TestLastMatchedLineWithEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogMultilinePolicy("", "", LOG_END_REGEX, UNMATCH_PREPEND);
    { // case: end with end
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_END_STRING + '\n';
        std::string testLog = std::string(expectMatch.data());
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(0, rollbackLineFeedCount);
    }
    { // case: end with unmatch
        std::string expectMatch =  LOG_UNMATCH + "\n" + LOG_UNMATCH + "\n" + LOG_END_STRING + '\n';
        std::string testLog = expectMatch + LOG_UNMATCH + "\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}