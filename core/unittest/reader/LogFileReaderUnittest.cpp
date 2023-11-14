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
#include "checkpoint/CheckPointManager.h"
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

    static void TearDownTestCase() {}

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
        reader.CheckFileSignatureAndOffset(true);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, reader.mLogFileOp.GetFileSize(), moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), logBuffer.rawBuffer.data());
    }
    { // buffer size big enough and match pattern, force read
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
        reader.CheckFileSignatureAndOffset(true);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, reader.mLogFileOp.GetFileSize(), moreData, false);
        APSARA_TEST_FALSE_FATAL(moreData);
        char* expectedContentAll = expectedContent.get();
        size_t tmp = strlen(expectedContentAll);
        expectedContentAll[tmp + 1] = '\n';
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), logBuffer.rawBuffer.data());
        expectedContentAll[tmp + 1] = '\0';
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
        LogFileReader::BUFFER_SIZE = 14;
        size_t BUFFER_SIZE_UTF8 = 15; // "ilogtail 为可"
        reader.SetLogMultilinePolicy("no matching pattern", ".*", ".*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        reader.CheckFileSignatureAndOffset(true);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, reader.mLogFileOp.GetFileSize(), moreData);
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*");
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = reader.mLogFileOp.GetFileSize();
        reader.CheckFileSignatureAndOffset(true);
        LogFileReader::BUFFER_SIZE = fileSize - 11;
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
    }
    { // read twice, multiline
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*");
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = reader.mLogFileOp.GetFileSize();
        reader.CheckFileSignatureAndOffset(true);
        LogFileReader::BUFFER_SIZE = fileSize - 11;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read, first part should be read
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        auto lastFilePos = reader.mLastFilePos;
        // second read, end of second part cannot be determined, nothing read
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        APSARA_TEST_EQUAL_FATAL(lastFilePos, reader.mLastFilePos);
    }
    { // read twice, single line
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
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = reader.mLogFileOp.GetFileSize();
        reader.CheckFileSignatureAndOffset(true);
        LogFileReader::BUFFER_SIZE = fileSize - 11;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read, first part should be read
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // -1 for \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        // second read, second part should be read
        reader.ReadGBK(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        APSARA_TEST_EQUAL_FATAL(0UL, reader.mCache.size());
    }
    { // empty file
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
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadGBK(logBuffer, 0, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(NULL, logBuffer.rawBuffer.data());
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
        reader.CheckFileSignatureAndOffset(true);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, reader.mLogFileOp.GetFileSize(), moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), logBuffer.rawBuffer.data());
    }
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
        reader.CheckFileSignatureAndOffset(true);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, reader.mLogFileOp.GetFileSize(), moreData, false);
        APSARA_TEST_FALSE_FATAL(moreData);
        char* expectedContentAll = expectedContent.get();
        size_t tmp = strlen(expectedContentAll);
        expectedContentAll[tmp + 1] = '\n';
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), logBuffer.rawBuffer.data());
        expectedContentAll[tmp + 1] = '\0';
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
        reader.SetLogMultilinePolicy("no matching pattern", ".*", ".*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        reader.CheckFileSignatureAndOffset(true);
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, reader.mLogFileOp.GetFileSize(), moreData);
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = reader.mLogFileOp.GetFileSize();
        reader.CheckFileSignatureAndOffset(true);
        LogFileReader::BUFFER_SIZE = fileSize - 13;
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
    }
    { // read twice, multiline
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = reader.mLogFileOp.GetFileSize();
        reader.CheckFileSignatureAndOffset(true);
        LogFileReader::BUFFER_SIZE = fileSize - 13;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // -1 for \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        auto lastFilePos = reader.mLastFilePos;
        // second read, end of second part cannot be determined, nothing read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        APSARA_TEST_EQUAL_FATAL(lastFilePos, reader.mLastFilePos);
    }
    { // read twice, singleline
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
        int64_t fileSize = reader.mLogFileOp.GetFileSize();
        reader.CheckFileSignatureAndOffset(true);
        LogFileReader::BUFFER_SIZE = fileSize - 13;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        // second read, second part should be read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        APSARA_TEST_EQUAL_FATAL(0UL, reader.mCache.size());
    }
    { // empty
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
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, 0, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(NULL, logBuffer.rawBuffer.data());
    }
}

class LogMultiBytesUnittest : public ::testing::Test {
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

    static void TearDownTestCase() {}

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
    void TestAlignLastCharacterUTF8();
    void TestAlignLastCharacterGBK();
    void TestReadUTF8();
    void TestReadGBK();

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

UNIT_TEST_CASE(LogMultiBytesUnittest, TestAlignLastCharacterUTF8);
UNIT_TEST_CASE(LogMultiBytesUnittest, TestAlignLastCharacterGBK);
UNIT_TEST_CASE(LogMultiBytesUnittest, TestReadUTF8);
UNIT_TEST_CASE(LogMultiBytesUnittest, TestReadGBK);

std::string LogMultiBytesUnittest::logPathDir;
std::string LogMultiBytesUnittest::gbkFile;
std::string LogMultiBytesUnittest::utf8File;

void LogMultiBytesUnittest::TestAlignLastCharacterUTF8() {
    { // case: no align
        CommonRegLogFileReader logFileReader(projectName,
                                             category,
                                             "",
                                             "",
                                             INT32_FLAG(default_tail_limit_kb),
                                             timeFormat,
                                             topicFormat,
                                             groupTopic,
                                             FileEncoding::ENCODING_UTF8,
                                             false,
                                             false);
        std::string expectedLog = "为可观测场景而";
        std::string testLog = expectedLog + "生";
        size_t result = logFileReader.AlignLastCharacter(const_cast<char*>(testLog.data()), expectedLog.size());
        APSARA_TEST_EQUAL_FATAL(expectedLog.size(), result);
    }
    { // case: cut off
        CommonRegLogFileReader logFileReader(projectName,
                                             category,
                                             "",
                                             "",
                                             INT32_FLAG(default_tail_limit_kb),
                                             timeFormat,
                                             topicFormat,
                                             groupTopic,
                                             FileEncoding::ENCODING_UTF8,
                                             false,
                                             false);
        std::string expectedLog = "为可观测场景而";
        std::string testLog = expectedLog + "生";
        size_t result = logFileReader.AlignLastCharacter(const_cast<char*>(testLog.data()), expectedLog.size() + 1);
        APSARA_TEST_EQUAL_FATAL(expectedLog.size(), result);
    }
}

void LogMultiBytesUnittest::TestAlignLastCharacterGBK() {
    CommonRegLogFileReader logFileReader(projectName,
                                         category,
                                         "",
                                         "",
                                         INT32_FLAG(default_tail_limit_kb),
                                         timeFormat,
                                         topicFormat,
                                         groupTopic,
                                         FileEncoding::ENCODING_GBK,
                                         false,
                                         false);
    { // case: no align
        std::string expectedLog
            = "\xce\xaa\xbf\xc9\xb9\xdb\xb2\xe2\xb3\xa1\xbe\xb0\xb6\xf8"; // equal to "为可观测场景而"
        std::string testLog = expectedLog + "\xc9";
        size_t result = logFileReader.AlignLastCharacter(const_cast<char*>(testLog.data()), expectedLog.size());
        APSARA_TEST_EQUAL_FATAL(expectedLog.size(), result);
    }
    { // case: GBK
        std::string expectedLog
            = "\xce\xaa\xbf\xc9\xb9\xdb\xb2\xe2\xb3\xa1\xbe\xb0\xb6\xf8"; // equal to "为可观测场景而"
        std::string testLog = expectedLog + "\xc9";
        size_t result = logFileReader.AlignLastCharacter(const_cast<char*>(testLog.data()), expectedLog.size() + 1);
        APSARA_TEST_EQUAL_FATAL(expectedLog.size(), result);
    }
}

void LogMultiBytesUnittest::TestReadUTF8() {
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
    LogFileReader::BUFFER_SIZE = 13; // equal to "iLogtail 为" plus one illegal byte
    reader.UpdateReaderManual();
    reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
    int64_t fileSize = reader.mLogFileOp.GetFileSize();
    reader.CheckFileSignatureAndOffset(true);
    LogBuffer logBuffer;
    bool moreData = false;
    reader.ReadUTF8(logBuffer, fileSize, moreData);
    std::string expectedPart(expectedContent.get());
    expectedPart = expectedPart.substr(0, LogFileReader::BUFFER_SIZE - 1);
    APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
}

void LogMultiBytesUnittest::TestReadGBK() {
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
    LogFileReader::BUFFER_SIZE = 12; // equal to "iLogtail 为" plus one illegal byte
    size_t BUFFER_SIZE_UTF8 = 12; // "ilogtail 为可"
    reader.UpdateReaderManual();
    reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
    int64_t fileSize = reader.mLogFileOp.GetFileSize();
    reader.CheckFileSignatureAndOffset(true);
    LogBuffer logBuffer;
    bool moreData = false;
    reader.ReadGBK(logBuffer, fileSize, moreData);
    APSARA_TEST_TRUE_FATAL(moreData);
    std::string expectedPart(expectedContent.get());
    expectedPart = expectedPart.substr(0, BUFFER_SIZE_UTF8);
    APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
}

class LogFileReaderCheckpointUnittest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        logPathDir = GetProcessExecutionDir();
        if (PATH_SEPARATOR[0] == logPathDir.back()) {
            logPathDir.resize(logPathDir.size() - 1);
        }
        logPathDir += PATH_SEPARATOR + "testDataSet" + PATH_SEPARATOR + "LogFileReaderUnittest";
        utf8File = "utf8.txt"; // content of utf8.txt is equivalent to gbk.txt
    }

    void TearDown() override {
        CheckPointManager::Instance()->RemoveAllCheckPoint();
        LogFileReader::BUFFER_SIZE = 1024 * 512;
    }

    void TestDumpMetaToMem();

    std::string projectName = "projectName";
    std::string category = "logstoreName";
    std::string timeFormat = "";
    std::string topicFormat = "";
    std::string groupTopic = "";
    static std::string logPathDir;
    static std::string utf8File;
};

UNIT_TEST_CASE(LogFileReaderCheckpointUnittest, TestDumpMetaToMem);

std::string LogFileReaderCheckpointUnittest::logPathDir;
std::string LogFileReaderCheckpointUnittest::utf8File;

void LogFileReaderCheckpointUnittest::TestDumpMetaToMem() {
    { // read twice with checkpoint, singleline
        CommonRegLogFileReader reader1(projectName,
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
        reader1.UpdateReaderManual();
        reader1.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = reader1.mLogFileOp.GetFileSize();
        reader1.CheckFileSignatureAndOffset(true);
        LogFileReader::BUFFER_SIZE = fileSize - 13;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        reader1.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        APSARA_TEST_GE_FATAL(reader1.mCache.size(), 0UL);
        reader1.DumpMetaToMem(false);
        // second read
        CommonRegLogFileReader reader2(projectName,
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
        reader2.UpdateReaderManual();
        reader2.InitReader(false, LogFileReader::BACKWARD_TO_BEGINNING);
        reader2.CheckFileSignatureAndOffset(true);
        APSARA_TEST_EQUAL_FATAL(reader1.mLastFilePos, reader2.mLastFilePos);
        APSARA_TEST_EQUAL_FATAL(reader1.mCache, reader2.mCache); // cache should recoverd from checkpoint
        reader2.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_EQUAL_FATAL(0UL, reader2.mCache.size());
        reader1.DumpMetaToMem(false);
    }
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
