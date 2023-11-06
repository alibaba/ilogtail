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
#include "reader/JsonLogFileReader.h"
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

class JsonFileReaderUnittest : public ::testing::Test {
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
        logPathDir += PATH_SEPARATOR + "testDataSet" + PATH_SEPARATOR + "JsonFileReaderUnittest";
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
};
std::string JsonFileReaderUnittest::logPathDir;
std::string JsonFileReaderUnittest::utf16LEFile;
std::string JsonFileReaderUnittest::utf16BEFile;
std::string JsonFileReaderUnittest::utf8File;

UNIT_TEST_CASE(JsonFileReaderUnittest, TestReadUtf16LE);
UNIT_TEST_CASE(JsonFileReaderUnittest, TestReadUtf16BE);

void JsonFileReaderUnittest::TestReadUtf16LE() {
    { // buffer size big enough and is json
        JsonLogFileReader reader(projectName,
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
        std::string recovered = StringView(logBuffer.buffer, size).data();
        std::replace(recovered.begin(), recovered.end(), '\0', '\n');

        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind(R"({"first")") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart, recovered.c_str());
    }
    { // buffer size not big enough to hold any json
      // should read buffer size
        JsonLogFileReader reader(projectName,
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
        size_t BUFFER_SIZE_UTF8 = 11; // {"first":"
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
        std::string expectedPart(expectedContent.get(), BUFFER_SIZE_UTF8); 
        std::string recovered(logBuffer.buffer, size);
        APSARA_TEST_EQUAL_FATAL(expectedPart,
                                recovered);
    }
    { // buffer size not big enough to hold all json
        // should read until last json
        JsonLogFileReader reader(projectName,
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
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind(R"({"first")") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
    { // read twice
        JsonLogFileReader reader(projectName,
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
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind(R"({"first")") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        // second read
        logBuffer = LogBuffer();
        truncateInfo = NULL;
        size = 0;
        bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart = expectedPart.substr(expectedPart.rfind(R"({"first")"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
}

void JsonFileReaderUnittest::TestReadUtf16BE() {
    { // buffer size big enough and is json
        JsonLogFileReader reader(projectName,
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
        std::string recovered = StringView(logBuffer.buffer, size).data();
        std::replace(recovered.begin(), recovered.end(), '\0', '\n');

        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind(R"({"first")") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart, recovered.c_str());
    }
    { // buffer size not big enough to hold any json
      // should read buffer size
        JsonLogFileReader reader(projectName,
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
        size_t BUFFER_SIZE_UTF8 = 11; // {"first":"
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
        std::string expectedPart(expectedContent.get(), BUFFER_SIZE_UTF8);
        std::string recovered(logBuffer.buffer, size);
        APSARA_TEST_EQUAL_FATAL(expectedPart,
                                recovered);
    }
    { // buffer size not big enough to hold all json
        // should read until last json
        JsonLogFileReader reader(projectName,
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
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind(R"({"first")") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
    { // read twice
        JsonLogFileReader reader(projectName,
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
        LogFileReader::BUFFER_SIZE = fileSize - 10;
        LogBuffer logBuffer;
        bool moreData = false;
        // first read
        TruncateInfo* truncateInfo = NULL;
        size_t size = 0;
        char*& bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_TRUE_FATAL(moreData);
        std::string expectedPart(expectedContent.get());
        expectedPart.resize(expectedPart.rfind(R"({"first")") - 1); // exclude tailing \n
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
        // second read
        logBuffer = LogBuffer();
        truncateInfo = NULL;
        size = 0;
        bufferptr = logBuffer.buffer;
        reader.ReadUTF16(bufferptr, &size, fileSize, moreData, truncateInfo);
        APSARA_TEST_FALSE_FATAL(moreData);
        expectedPart = expectedContent.get();
        expectedPart = expectedPart.substr(expectedPart.rfind(R"({"first")"));
        APSARA_TEST_STREQ_FATAL(expectedPart.c_str(), StringView(logBuffer.buffer, size).data());
    }
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
