#include <gtest/gtest.h>
#include "common/FileUtils.h"
#include "common/Logger.h"

#include <vector>
#include <list>
#include <boost/algorithm/string/replace.hpp> // boost::replace_all_copy

#include "common/UnitTestEnv.h"
#include "common/Uuid.h"
#include "common/StringUtils.h"

using namespace std;
using namespace common;

class CommonFileUtilsTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(CommonFileUtilsTest, ReadFileLines) {
    vector<string> lines;
    EXPECT_EQ(1, FileUtils::ReadFileLines((TEST_CONF_PATH / "conf/RDMA/data").string(), lines));
    lines.clear();
    EXPECT_EQ(82, FileUtils::ReadFileLines((TEST_CONF_PATH / "conf/RDMA/rdma.data1").string(), lines));
}

TEST_F(CommonFileUtilsTest, WriteFileContent) {
    fs::path file = TEST_CONF_PATH / "tmp" / "test.txt";
    const char *szFile = file.string().c_str();
    const std::string fileContent = "testhelloworld";
    EXPECT_EQ(FileUtils::WriteFileContent(szFile, fileContent), (int)fileContent.size());
    EXPECT_EQ(fileContent.size(), fs::file_size(file));

    string result;
    EXPECT_EQ((int)fileContent.size(), FileUtils::ReadFileContent(szFile, result));
    EXPECT_EQ(result, fileContent);

    boost::system::error_code ec;
    fs::remove_all(file.parent_path(), ec);
    EXPECT_FALSE(fs::exists(file.remove_filename()));
}

TEST_F(CommonFileUtilsTest, isDir) {
#ifndef WIN32
    EXPECT_TRUE(FileUtils::isDir("/etc"));
    EXPECT_FALSE(FileUtils::isDir("/etc/hosts"));
#else
    EXPECT_TRUE(FileUtils::isDir(GetExecDir().string()));
    EXPECT_FALSE(fs::exists("/etc/hosts"));
    EXPECT_FALSE(FileUtils::isDir("/etc/hosts"));
#endif
}

TEST_F(CommonFileUtilsTest, isFile) {
    LogInfo("ExecDir : {}", GetExecDir().string());
    EXPECT_TRUE(fs::exists(GetExecDir()));
    EXPECT_FALSE(FileUtils::isFile(GetExecDir().string()));
    LogInfo("ExecPath: {}", GetExecPath().string());
    EXPECT_TRUE(FileUtils::isFile(GetExecPath().string()));
#ifndef WIN32
    EXPECT_FALSE(FileUtils::isFile("/etc"));
    EXPECT_TRUE(FileUtils::isFile("/etc/hosts"));
#endif
}

TEST_F(CommonFileUtilsTest, GetFileLines1) {
    std::list<std::string> lines;
    EXPECT_EQ(0, GetFileLines(__FILE__, lines));
    EXPECT_FALSE(lines.empty());
    const std::string expect = "#include";
    EXPECT_TRUE(lines.front().substr(0, expect.size()) == expect);
    for (int i = 0; i < 10; i++) {
        std::cout << "[" << i << "] " << lines.front() << std::endl;
        lines.pop_front();
    }
}

TEST_F(CommonFileUtilsTest, GetFileLines2) {
    std::list<std::string> lines;
    std::string errMsg;
    int n = GetFileLines(__FILE__ + std::string(".123"), lines, true, &errMsg);
    EXPECT_NE(0, n);
    EXPECT_FALSE(errMsg.empty());
    std::cout << errMsg << std::endl;
}

TEST_F(CommonFileUtilsTest, GetFileLines3) {
    std::string errMsg;
    std::vector<std::string> lines = GetFileLines(__FILE__ + std::string(".123"), true, errMsg);
    EXPECT_FALSE(errMsg.empty());
    EXPECT_TRUE(lines.empty());
    std::cout << errMsg << std::endl;
}

TEST_F(CommonFileUtilsTest, GetFileLines4) {
    std::string errMsg;
    std::vector<std::string> lines = GetFileLines(__FILE__, true, &errMsg);
    EXPECT_TRUE(errMsg.empty());
    if (!errMsg.empty()) {
        std::cout << errMsg << std::endl;
    }
    EXPECT_FALSE(lines.empty());
}

TEST_F(CommonFileUtilsTest, ReadFileContent) {
    {
        std::string errorMessage;
        EXPECT_EQ(ReadFileContent("/tmp/not-exist-" + NewUuid(), &errorMessage), "");
        EXPECT_NE(errorMessage, "");
    }
    {
        std::string errorMessage;
        EXPECT_TRUE(ReadFileBinary("/tmp/not-exist-" + NewUuid(), &errorMessage).empty());
        EXPECT_NE(errorMessage, "");
    }

    std::string s = ReadFileContent(__FILE__);
    std::cout << "File: " << __FILE__ << std::endl
              << "Size: " << convert(s.size(), true) << " bytes" << std::endl;
    EXPECT_NE(s.find("ReadFileContent(__FILE__)"), std::string::npos);
    s = boost::replace_all_copy(s, "\r", ""); // 屏蔽掉平台差异，windows下会在行尾追加\r

    std::cout << s.substr(0, 128) << std::endl;
    std::cout << s.substr(s.size() - 128) << std::endl;

    std::vector<char> bin = ReadFileBinary(__FILE__);
    std::string binS = boost::replace_all_copy(std::string{ bin.begin(), bin.end() }, "\r", "");
    EXPECT_EQ(binS.size(), s.size());
}

TEST_F(CommonFileUtilsTest, WriteFileContent2) {
#ifdef WIN32
    // 假设X盘不存在
#   define PATH_PREFIX "X:\\" + NewUuid()
#else
#   define PATH_PREFIX GetExecPath().string()
#endif
    EXPECT_FALSE(WriteFileContent(PATH_PREFIX + "/not-exist-" + NewUuid(), "123"));

    fs::path tmpFile = GetBaseDir() / "SystemInformationCollectorTest-WriteFileContent.txt";
    std::cout << "tmpFile: " << tmpFile << std::endl;
    EXPECT_TRUE(WriteFileContent(tmpFile, "123"));
    EXPECT_TRUE(boost::filesystem::exists(tmpFile));

    std::vector<std::string> lines = ReadFileLines(tmpFile);
    ASSERT_EQ(size_t(1), lines.size());
    EXPECT_EQ(lines, std::vector<std::string>{"123"});

    boost::filesystem::remove(tmpFile);
}

TEST_F(CommonFileUtilsTest, FileOperation) {
    {
        File file(TEST_SIC_CONF_PATH / "conf" / "diskstats", File::ModeReadText);
        EXPECT_GT(file.FileNo(), 0);
        EXPECT_FALSE(file.IsError());
    }
    fs::path tmpFile = TEST_SIC_CONF_PATH / "conf" / ("tmp-" + NewUuid() + ".txt");
    {
        File file(tmpFile, File::ModeWriteText);
        ASSERT_GT(file.FileNo(), 0);
        EXPECT_FALSE(file.IsError());
        EXPECT_EQ(size_t(5), file.Write("hello"));
    }
    fs::remove(tmpFile);
}
