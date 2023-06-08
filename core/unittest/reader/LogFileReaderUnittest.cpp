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
#include <stdio.h>
#include <fstream>
#include "reader/CommonRegLogFileReader.h"
#include "reader/SourceBuffer.h"
#include "common/RuntimeUtil.h"
#include "common/FileSystemUtil.h"
#include "log_pb/sls_logs.pb.h"
#include "util.h"

DECLARE_FLAG_INT32(force_release_deleted_file_fd_timeout);

namespace logtail {

class LogFileReaderUnittest : public ::testing::Test {
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
    void TearDown() override { LogFileReader::BUFFER_SIZE = 1024 * 512; }
    void TestReadGBK();
    void TestReadUTF8();
    void TestLastMatchedLine();

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

UNIT_TEST_CASE(LogFileReaderUnittest, TestLastMatchedLine);
UNIT_TEST_CASE(LogFileReaderUnittest, TestReadGBK);
UNIT_TEST_CASE(LogFileReaderUnittest, TestReadUTF8);

std::string LogFileReaderUnittest::logPathDir;
std::string LogFileReaderUnittest::gbkFile;
std::string LogFileReaderUnittest::utf8File;

void LogFileReaderUnittest::TestReadGBK() {
    { // buffer size big enough and match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      gbkFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_GBK,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), logBuffer.rawBuffer.data());
    }
    { // buffer size not big enough and not match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      gbkFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_GBK,
                                      false,
                                      false);
        LogFileReader::BUFFER_SIZE = 13;
        size_t BUFFER_SIZE_UTF8 = 15; // "ilogtail 为可"
        reader.SetLogBeginRegex("no matching pattern");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(std::string(expectedContent.get(), BUFFER_SIZE_UTF8).c_str(),
                                logBuffer.rawBuffer.data());
    }
    { // buffer size not big enough and match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      gbkFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_GBK,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 11;
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
    }
    { // read twice
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      gbkFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_GBK,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 11;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        // second read
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
    }
}

void LogFileReaderUnittest::TestReadUTF8() {
    { // buffer size big enough and match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf8File,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF8,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), logBuffer.rawBuffer.data());
    }
    { // buffer size not big enough and not match pattern
        // should read buffer size
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf8File,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF8,
                                      false,
                                      false);
        LogFileReader::BUFFER_SIZE = 15;
        reader.SetLogBeginRegex("no matching pattern");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(std::string(expectedContent.get(), LogFileReader::BUFFER_SIZE).c_str(),
                                logBuffer.rawBuffer.data());
    }
    { // buffer size not big enough and match pattern
        // should read to match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf8File,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF8,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 11;
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        // TODO: Should read util the last matched line, but read the whole buffer
        // expect: expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        expectedPart = expectedPart.substr(0, LogFileReader::BUFFER_SIZE);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
    }
    { // read twice
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf8File,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF8,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 11;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        // TODO: expect: expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        expectedPart.resize(fileSize - 11);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        // second read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        // TODO: expect: expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        expectedPart = expectedPart.substr(fileSize - 11);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
    }
}

void LogFileReaderUnittest::TestLastMatchedLine() {
    CommonRegLogFileReader logFileReader(projectName,
                                         category,
                                         logPathDir,
                                         utf8File,
                                         INT32_FLAG(default_tail_limit_kb),
                                         timeFormat,
                                         topicFormat,
                                         groupTopic);
    const std::string LOG_BEGIN_TIME = "2012-12-12上午 : ";
    const std::string LOG_BEGIN_REGEX = R"(\d{4}-\d{2}-\d{2}上午.*)";
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
        std::string line1 = "first.";
        std::string expectMatch = line1;
        std::string testLog = expectMatch;
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(0, matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(1, rollbackLineFeedCount);
    }
    { // case multi line
        logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX); // can only be called once

        std::vector<int32_t> index;
        std::string firstLog = LOG_BEGIN_TIME + "first.\nmultiline1\nmultiline2";
        std::string secondLog = LOG_BEGIN_TIME + "second.\nmultiline1\nmultiline2";
        std::string expectMatch = firstLog + '\n';
        std::string testLog = expectMatch + secondLog + '\n';
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
    { // case multi line, buffer size not big enough (no new line at the end of line)
        std::vector<int32_t> index;
        std::string firstLog = LOG_BEGIN_TIME + "first.\nmultiline1\nmultiline2";
        std::string secondLog = LOG_BEGIN_TIME + "second.\nmultiline1\nmultiline2";
        std::string expectMatch = firstLog + '\n';
        std::string testLog = expectMatch + secondLog;
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog.data()), testLog.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(static_cast<int32_t>(expectMatch.size()), matchSize);
        APSARA_TEST_EQUAL_FATAL(std::string(testLog.data()), expectMatch);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
    { // case multi line not match
        std::string testLog2 = "log begin does not match.\nlog begin does not match.\nlog begin does not match.\n";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog2.data()), testLog2.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(0, matchSize);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
    { // case multi line not match, buffer size not big enough (no new line at the end of line)
        std::string testLog2 = "log begin does not match.\nlog begin does not match.\nlog begin does not";
        int32_t rollbackLineFeedCount = 0;
        int32_t matchSize
            = logFileReader.LastMatchedLine(const_cast<char*>(testLog2.data()), testLog2.size(), rollbackLineFeedCount);
        APSARA_TEST_EQUAL_FATAL(0, matchSize);
        APSARA_TEST_EQUAL_FATAL(3, rollbackLineFeedCount);
    }
}

class LogSplitUnittest : public ::testing::Test {
public:
    void TestLogSplitSingleLine();
    void TestLogSplitSingleLinePartNotMatch();
    void TestLogSplitSingleLinePartNotMatchNoDiscard();
    void TestLogSplitMultiLine();
    void TestLogSplitMultiLinePartNotMatch();
    void TestLogSplitMultiLinePartNotMatchNoDiscard();
    void TestLogSplitMultiLineAllNotmatch();
    void TestLogSplitMultiLineAllNotmatchNoDiscard();
    const std::string LOG_BEGIN_TIME = "2012-12-12上午 : ";
    const std::string LOG_BEGIN_REGEX = R"(\d{4}-\d{2}-\d{2}上午.*)";
};

UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitSingleLine);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitSingleLinePartNotMatch);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitSingleLinePartNotMatchNoDiscard);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLine);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLinePartNotMatch);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLinePartNotMatchNoDiscard);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLineAllNotmatch);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLineAllNotmatchNoDiscard);

void LogSplitUnittest::TestLogSplitSingleLine() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    std::string line1 = LOG_BEGIN_TIME + "first.";
    std::string line23 = "multiline1\nmultiline2";
    std::string line4 = LOG_BEGIN_TIME + "second.";
    std::string line56 = "multiline1\nmultiline2";
    std::string testLog = line1 + '\n' + line23 + '\n' + line4 + '\n' + line56;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(6UL, index.size());
    APSARA_TEST_EQUAL_FATAL(line1, index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(line4, index[3].to_string());
}

void LogSplitUnittest::TestLogSplitSingleLinePartNotMatch() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    std::string line1 = "first.";
    std::string testLog = line1;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(0UL, index.size());
    APSARA_TEST_EQUAL_FATAL(1UL, discard.size());
}

void LogSplitUnittest::TestLogSplitSingleLinePartNotMatchNoDiscard() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false);
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    std::string line1 = "first.";
    std::string testLog = line1;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(1UL, index.size());
    APSARA_TEST_EQUAL_FATAL(line1, index[0].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLine() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    std::string firstLog = LOG_BEGIN_TIME + "first.\nmultiline1\nmultiline2";
    std::string secondLog = LOG_BEGIN_TIME + "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(2UL, index.size());
    APSARA_TEST_EQUAL_FATAL(firstLog, index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(secondLog, index[1].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLinePartNotMatch() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = LOG_BEGIN_TIME + "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(1UL, index.size());
    APSARA_TEST_EQUAL_FATAL(secondLog, index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(1UL, discard.size());
    APSARA_TEST_EQUAL_FATAL(firstLog, discard[0].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLinePartNotMatchNoDiscard() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false);
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = LOG_BEGIN_TIME + "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(2UL, index.size());
    APSARA_TEST_EQUAL_FATAL(firstLog, index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(secondLog, index[1].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLineAllNotmatch() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(0UL, index.size());
    APSARA_TEST_EQUAL_FATAL(1UL, discard.size());
    APSARA_TEST_EQUAL_FATAL(testLog, discard[0].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLineAllNotmatchNoDiscard() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false);
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(1UL, index.size());
    APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
