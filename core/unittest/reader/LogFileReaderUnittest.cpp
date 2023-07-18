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
        LogFileReader::BUFFER_SIZE = 14;
        size_t BUFFER_SIZE_UTF8 = 15; // "ilogtail 为可"
        reader.SetLogMultilinePolicy("no matching pattern", ".*", ".*", "");
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*", "");
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*", "");
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
        reader.SetLogMultilinePolicy("no matching pattern", ".*", ".*", "");
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*", "");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 13;
        LogBuffer logBuffer;
        bool moreData = false;
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
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
        reader.SetLogMultilinePolicy("iLogtail.*", ".*", ".*", "");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 13;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
        // second read
        reader.ReadUTF8(logBuffer, fileSize, moreData);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), logBuffer.rawBuffer.data());
    }
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
