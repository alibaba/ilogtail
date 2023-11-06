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
#include "common/FileSystemUtil.h"
#include <boost/utility/string_view.hpp>
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
using namespace logtail;
using StringView = boost::string_view;

DECLARE_FLAG_INT64(first_read_endure_bytes);

namespace logtail {

class LogFileReaderUnittest : public ::testing::Test {
    static const string LOG_BEGIN_TIME;
    static const string LOG_BEGIN_REGEX;
    static const string NEXT_LINE;
    static const size_t BUFFER_SIZE;
    static const size_t SIGNATURE_CHAR_COUNT;
    static string gRootDir;

public:
    std::string projectName = "projectName";
    std::string category = "logstoreName";
    std::string timeFormat = "";
    std::string topicFormat = "";
    std::string groupTopic = "";
    std::unique_ptr<char[]> expectedContent;
    static std::string logPathDir;
    static std::string utf16LEFile;
    static std::string utf16BEFile;
    static std::string utf8File;

    static void SetUpTestCase() {
        logPathDir = GetProcessExecutionDir();
        if (PATH_SEPARATOR[0] == logPathDir.back()) {
            logPathDir.resize(logPathDir.size() - 1);
        }
        logPathDir += PATH_SEPARATOR + "testDataSet" + PATH_SEPARATOR + "LogFileReaderUnittest";
        utf16LEFile = "utf16LE.log";
        utf16BEFile = "utf16BE.log";
        utf8File = "utf8.log"; // content of utf8.txt is equivalent to utf16LE.log and utf16BE.log
        cout << "logPathDir: " << logPathDir << endl;
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

    void TestReadUtf16LE();
    void TestReadUtf16BE();

    void TestLogSplit();
    std::string GenerateRawData(
        int32_t lines, vector<int32_t>& index, int32_t size = 100, bool singleLine = false, bool gbk = false);
};
std::string LogFileReaderUnittest::logPathDir;
std::string LogFileReaderUnittest::utf16LEFile;
std::string LogFileReaderUnittest::utf16BEFile;
std::string LogFileReaderUnittest::utf8File;

UNIT_TEST_CASE(LogFileReaderUnittest, TestReadUtf16LE);
UNIT_TEST_CASE(LogFileReaderUnittest, TestReadUtf16BE);


void LogFileReaderUnittest::TestReadUtf16LE() {
    { // buffer size big enough and match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16LEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), StringView(logBuffer.buffer, size).data());
    }
    { // buffer size big enough and match pattern, force read
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16LEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        char* expectedContentAll = expectedContent.get();
        size_t tmp = strlen(expectedContentAll);
        expectedContentAll[tmp + 1] = '\n';
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), StringView(logBuffer.buffer, size).data());
        expectedContentAll[tmp + 1] = '\0';
    }
    { // buffer size not big enough and not match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16LEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        LogFileReader::BUFFER_SIZE = 24;
        size_t BUFFER_SIZE_UTF8 = 15; // "ilogtail 为可"
        reader.SetLogBeginRegex("no matching pattern");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(std::string(expectedContent.get(), BUFFER_SIZE_UTF8).c_str(),
                                StringView(logBuffer.buffer, size).data());
    }
    { // buffer size not big enough and match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16LEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
    { // read twice, multiline
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16LEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 4;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read, first part should be read
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        auto lastFilePos = reader.mLastFilePos;
        // second read
        logBuffer = LogBuffer();
        truncateInfo = NULL;
        size = 0;
        bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = string(expectedContent.get());
        expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
    { // read twice, single line
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16LEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read, first part should be read
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // -1 for \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        // APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        // second read, second part should be read
        truncateInfo = NULL;
        size = 0;
        bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        // APSARA_TEST_EQUAL_FATAL(0UL, reader.mCache.size());
    }
    { // empty file
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16LEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        LogBuffer logBuffer;
        logBuffer.buffer = nullptr;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, 0, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_EQUAL_FATAL(nullptr, logBuffer.buffer);
    }
}

void LogFileReaderUnittest::TestReadUtf16BE() {
    { // buffer size big enough and match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16BEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), StringView(logBuffer.buffer, size).data());
    }
    { // buffer size big enough and match pattern, force read
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16BEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        char* expectedContentAll = expectedContent.get();
        size_t tmp = strlen(expectedContentAll);
        expectedContentAll[tmp + 1] = '\n';
        APSARA_TEST_STREQ_FATAL(expectedContent.get(), StringView(logBuffer.buffer, size).data());
        expectedContentAll[tmp + 1] = '\0';
    }
    { // buffer size not big enough and not match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16BEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        LogFileReader::BUFFER_SIZE = 24;
        size_t BUFFER_SIZE_UTF8 = 15; // "ilogtail 为可"
        reader.SetLogBeginRegex("no matching pattern");
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        APSARA_TEST_STREQ_FATAL(std::string(expectedContent.get(), BUFFER_SIZE_UTF8).c_str(),
                                StringView(logBuffer.buffer, size).data());
    }
    { // buffer size not big enough and match pattern
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16BEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
    { // read twice, multiline
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16BEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.SetLogBeginRegex("iLogtail.*");
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 4;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read, first part should be read
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1);
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        auto lastFilePos = reader.mLastFilePos;
        // second read
        logBuffer = LogBuffer();
        truncateInfo = NULL;
        size = 0;
        bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = string(expectedContent.get());
        expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
    { // read twice, single line
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16BEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.mDiscardUnmatch = false;
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        int64_t fileSize = 0;
        reader.CheckFileSignatureAndOffset(fileSize);
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read, first part should be read
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind("iLogtail") - 1); // -1 for \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        // APSARA_TEST_GE_FATAL(reader.mCache.size(), 0UL);
        // second read, second part should be read
        truncateInfo = NULL;
        size = 0;
        bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart = expectedPart.substr(expectedPart.rfind("iLogtail"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        // APSARA_TEST_EQUAL_FATAL(0UL, reader.mCache.size());
    }
    { // empty file
        CommonRegLogFileReader reader(projectName,
                                      category,
                                      logPathDir,
                                      utf16BEFile,
                                      INT32_FLAG(default_tail_limit_kb),
                                      timeFormat,
                                      topicFormat,
                                      groupTopic,
                                      FileEncoding::ENCODING_UTF16,
                                      false,
                                      false);
        reader.UpdateReaderManual();
        reader.InitReader(true, LogFileReader::BACKWARD_TO_BEGINNING);
        LogBuffer logBuffer;
        logBuffer.buffer = nullptr;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, 0, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        APSARA_TEST_EQUAL_FATAL(nullptr, logBuffer.buffer);
    }
}

APSARA_UNIT_TEST_CASE(LogFileReaderUnittest, TestLogSplit, 1);


const string LogFileReaderUnittest::LOG_BEGIN_TIME = "2012-12-12上午 : ";
const string LogFileReaderUnittest::LOG_BEGIN_REGEX = "\\d{4}-\\d{2}-\\d{2}上午.*";
const string LogFileReaderUnittest::NEXT_LINE = "\n";
const size_t LogFileReaderUnittest::BUFFER_SIZE = 1024 * 512;
const size_t LogFileReaderUnittest::SIGNATURE_CHAR_COUNT = 1024;
string LogFileReaderUnittest::gRootDir = "";

string
LogFileReaderUnittest::GenerateRawData(int32_t lines, vector<int32_t>& index, int32_t size, bool singleLine, bool gbk) {
    static string GBK_CHARCTERS[26] = {
#if defined(__linux__)
        "行",
        "请",
        "求",
        "环",
        "节",
        "里",
        "效",
        "率",
        "最",
        "高",
        "的",
        "部",
        "分",
        "这",
        "就",
        "让",
        "每",
        "个",
        "请",
        "求",
        "处",
        "理",
        "速",
        "度",
        "了",
        "多"
#elif defined(_MSC_VER)
        "\xe8\xa1\x8c",
        "\xe8\xaf\xb7",
        "\xe6\xb1\x82",
        "\xe7\x8e\xaf",
        "\xe8\x8a\x82",
        "\xe9\x87\x8c",
        "\xe6\x95\x88",
        "\xe7\x8e\x87",
        "\xe6\x9c\x80",
        "\xe9\xab\x98",
        "\xe7\x9a\x84",
        "\xe9\x83\xa8",
        "\xe5\x88\x86",
        "\xe8\xbf\x99",
        "\xe5\xb0\xb1",
        "\xe8\xae\xa9",
        "\xe6\xaf\x8f",
        "\xe4\xb8\xaa",
        "\xe8\xaf\xb7",
        "\xe6\xb1\x82",
        "\xe5\xa4\x84",
        "\xe7\x90\x86",
        "\xe9\x80\x9f",
        "\xe5\xba\xa6",
        "\xe4\xba\x86",
        "\xe5\xa4\x9a"
#endif
    };
    int32_t prefixSize = (int32_t)LOG_BEGIN_TIME.size();
    if (size < prefixSize + 10)
        size = prefixSize + 10;
    string rawLog;
    for (int32_t i = 0; i < lines; ++i) {
        index.push_back(rawLog.size());
        rawLog += LOG_BEGIN_TIME;
        for (int32_t j = 0; j < size - prefixSize - 1; ++j) {
            int32_t mod = rand() % 26;
            int32_t modValue = rand() % 10;
            if (!singleLine) {
                if (gbk) {
                    if (modValue == 0)
                        rawLog.append("\n");
                    else if (modValue / 2 == 0)
                        rawLog.append(GBK_CHARCTERS[mod]);
                    else
                        rawLog.append(1, char('A' + mod));
                } else {
                    if (modValue == 0)
                        rawLog.append("\n");
                    else
                        rawLog.append(1, char('A' + mod));
                }
            } else {
                if (gbk) {
                    if (modValue / 2 == 0)
                        rawLog.append(GBK_CHARCTERS[mod]);
                    else
                        rawLog.append(1, char('A' + mod));
                } else
                    rawLog.append(1, char('A' + mod));
            }
        }
        rawLog.append(NEXT_LINE);
    }
    return rawLog;
}

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
    vector<int32_t> splitIndex = logFileReader.LogSplit(buffer, multipleLogSize, lineFeed);
    APSARA_TEST_EQUAL(index.size(), splitIndex.size());
    for (size_t i = 0; i < index.size() && i < splitIndex.size(); ++i) {
        APSARA_TEST_EQUAL(index[i], splitIndex[i]);
    }
    APSARA_TEST_TRUE(lineFeed >= (int32_t)splitIndex.size());
    delete[] buffer;

    // case 1: single line
    {
        string singleLog = LOG_BEGIN_TIME + "single log";
        int32_t singleLogSize = singleLog.size();
        buffer = new char[singleLogSize + 1];
        strcpy(buffer, singleLog.c_str());
        buffer[singleLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, singleLogSize, lineFeed);
        APSARA_TEST_EQUAL(1, splitIndex.size());
        APSARA_TEST_EQUAL(0, splitIndex[0]);
        APSARA_TEST_EQUAL(1, lineFeed);
        delete[] buffer;
    }

    // case 2: invalid regex beginning
    {
        string invalidLog = "xx" + LOG_BEGIN_TIME + "invalid log 1\nyy" + LOG_BEGIN_TIME + "invalid log 2\n";
        int32_t invalidLogSize = invalidLog.size();
        buffer = new char[invalidLogSize + 1];
        strcpy(buffer, invalidLog.c_str());
        buffer[invalidLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed);
        APSARA_TEST_EQUAL(0, splitIndex.size());
        APSARA_TEST_EQUAL(2, lineFeed);
        delete[] buffer;
    }


    // case 3: invalid one + valid one, no discard unmatch
    {
        CommonRegLogFileReader logFileReader(
            "100_proj", "", "", "", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false, false);
        logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
        string invalidLog = "xx" + LOG_BEGIN_TIME + "invalid log 1\n" + LOG_BEGIN_TIME + "invalid log 2\n";
        int32_t invalidLogSize = invalidLog.size();
        buffer = new char[invalidLogSize + 1];
        strcpy(buffer, invalidLog.c_str());
        buffer[invalidLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed);
        APSARA_TEST_EQUAL(2, splitIndex.size());
        APSARA_TEST_EQUAL(0, splitIndex[0]);
        APSARA_TEST_EQUAL(35, splitIndex[1]);
        APSARA_TEST_EQUAL(2, lineFeed);
        delete[] buffer;
    }

    // case 4: invalid one + valid one, discard unmatch
    {
        string invalidLog = "xx" + LOG_BEGIN_TIME + "invalid log 1\n" + LOG_BEGIN_TIME + "invalid log 2\n";
        int32_t invalidLogSize = invalidLog.size();
        buffer = new char[invalidLogSize + 1];
        strcpy(buffer, invalidLog.c_str());
        buffer[invalidLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed);
        APSARA_TEST_EQUAL(1, splitIndex.size());
        APSARA_TEST_EQUAL(35, splitIndex[0]);
        APSARA_TEST_EQUAL(2, lineFeed);
        delete[] buffer;
    }

    // case 5: valid one + invalid one, no discard unmatch
    {
        CommonRegLogFileReader logFileReader(
            "100_proj", "", "", "", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false, false);
        logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
        string invalidLog = LOG_BEGIN_TIME + "invalid log 1\nyy" + LOG_BEGIN_TIME + "invalid log 2\n";
        int32_t invalidLogSize = invalidLog.size();
        buffer = new char[invalidLogSize + 1];
        strcpy(buffer, invalidLog.c_str());
        buffer[invalidLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed);
        APSARA_TEST_EQUAL(1, splitIndex.size());
        APSARA_TEST_EQUAL(0, splitIndex[0]);
        APSARA_TEST_EQUAL(2, lineFeed);
        delete[] buffer;
    }

    // case 6: valid one + invalid one, discard unmatch
    {
        string invalidLog = LOG_BEGIN_TIME + "invalid log 1\nyy" + LOG_BEGIN_TIME + "invalid log 2\n";
        int32_t invalidLogSize = invalidLog.size();
        buffer = new char[invalidLogSize + 1];
        strcpy(buffer, invalidLog.c_str());
        buffer[invalidLogSize - 1] = '\0';
        splitIndex = logFileReader.LogSplit(buffer, invalidLogSize, lineFeed);
        APSARA_TEST_EQUAL(1, splitIndex.size());
        APSARA_TEST_EQUAL(0, splitIndex[0]);
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
