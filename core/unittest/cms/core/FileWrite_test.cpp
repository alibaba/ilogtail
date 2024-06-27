#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // fix problems under windows: 1) apr_sockaddr_t::sin6 2) LPMSG
#endif
#include <gtest/gtest.h>
#include "core/FileWrite.h"

#include <string>

#include "common/ScopeGuard.h"
#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"
#include "common/Config.h"

using namespace common;
using namespace std;
using namespace argus;

class CoreFileWriteTest : public testing::Test {
protected:
    void SetUp() override {
        oldBaseDir = SingletonConfig::Instance()->getBaseDir();
        SingletonConfig::Instance()->setBaseDir(TEST_CONF_PATH);

        p_shared = new FileWrite(10000, 5);
    }

    void TearDown() override {
        Delete(p_shared);
        SingletonConfig::Instance()->setBaseDir(oldBaseDir);
    }

    FileWrite *p_shared = nullptr;
    fs::path oldBaseDir;
};

TEST_F(CoreFileWriteTest, setup) {
    EXPECT_FALSE(p_shared->setup("test.log"));
    // string pwd=string(getcwd(nullptr, 0));
    string file = (TEST_CONF_PATH / "local_data/logs/.arugsagent.log").string();
    EXPECT_TRUE(p_shared->setup(file));
    remove(file.c_str());
    file = (TEST_CONF_PATH / "local_data/logs/.arugsagent.log").string();
    file += std::string(512, '1');
    // for (int i = 0; i < 256; i++) {
    //     file += "11";
    // }
    EXPECT_FALSE(p_shared->setup(file));
}

TEST_F(CoreFileWriteTest, shouldRotate) {
    // string pwd=string(getcwd(nullptr, 0));
    string file = (TEST_CONF_PATH / "conf/logs/pingagent.log").string();
    EXPECT_EQ(1, p_shared->setup(file));
    EXPECT_EQ(1, p_shared->shouldRotate());
    file = (TEST_CONF_PATH / "conf/logs/argusagent.log_no").string();
    EXPECT_EQ(1, p_shared->setup(file));
    EXPECT_EQ(0, p_shared->shouldRotate());
    remove(file.c_str());
}

TEST_F(CoreFileWriteTest, doRecord) {
    // string pwd=string(getcwd(nullptr, 0));
    string file = (TEST_CONF_PATH / "conf/logs/argusagent.log").string();
    file += std::string(512, '1');

    EXPECT_EQ(0, p_shared->setup(file));
    string error;
    EXPECT_EQ(-1, p_shared->doRecord("test", 4, error));
}

int getLogFileNum(const string &dirPath, const string &filename) {
    int fileNum = 0;
    for (auto& entry : fs::directory_iterator(dirPath)) {
        std::string name = entry.path().filename().string();
#ifdef ENABLE_COVERAGE
		LogDebug("{}[{}] {}", __FUNCTION__, fileNum, name);
#endif
        if (name.find(filename) != std::string::npos) {
            fileNum++;
        }
    }
    return fileNum;
    //apr_pool_t *memPool = nullptr;
    //apr_pool_create(&memPool, nullptr);
    //ResourceGuard rg([memPool]() { if (memPool) { apr_pool_destroy(memPool); }});
    //int fileNum = 0;
    //apr_dir_t *dir = nullptr;
    //if (apr_dir_open(&dir, dirPath.c_str(), memPool) != APR_SUCCESS) {
    //    return 0;
    //}
    //apr_finfo_t finfo;
    //int finfoFlags = APR_FINFO_TYPE | APR_FINFO_NAME | APR_FINFO_CTIME | APR_FINFO_MTIME;
    //apr_status_t apr_ret = 0;
    //while (APR_ENOENT != apr_ret) {
    //    apr_ret = apr_dir_read(&finfo, finfoFlags, dir);
    //    if (APR_ENOENT == apr_ret) {
    //        break;
    //    }
    //    if (nullptr != strstr(finfo.name, filename.c_str())) {
    //        fileNum++;

    //    }
    //}
    //apr_dir_close(dir);
    //return fileNum;
}

int removeFiles(const fs::path &filePrefix) {
    LogDebug("remove: {}*", filePrefix.string());
    int count = 0;
    for (auto &de: fs::directory_iterator{filePrefix.parent_path()}) {
        bool shoot = HasPrefix(de.path().filename().string(), filePrefix.filename().string());
        LogDebug("[{}]: {}", (shoot ? "rm  " : "skip"), de.path().filename().string());
        if (shoot) {
            fs::remove(de.path());
            count++;
        }
    }
    return count;
}

TEST_F(CoreFileWriteTest, rotateLog) {
    //string pwd=string(getcwd(nullptr, 0));
    fs::path deleteFile = TEST_CONF_PATH / "conf/logs/rotateLog/pingagent.log";
    removeFiles(deleteFile);
    //string cmd="rm -f  "+deleteFile;
    //EXPECT_EQ(0, ExecCmd(cmd));
    fs::path oldFile = TEST_CONF_PATH / "conf/logs/pingagent.log";
    // fs::path file= TEST_CONF_PATH / "conf/logs/rotateLog/pingagent.log";
    const fs::path &file = deleteFile;
    //cmd="cp "+oldFile+" "+file;
    //EXPECT_EQ(0,ExecCmd(cmd));
    ASSERT_TRUE(boost::filesystem::copy_file(oldFile, file));

    EXPECT_EQ(1, p_shared->setup(file.string()));
    EXPECT_EQ(0, p_shared->rotateLog());
    std::string pwd = (TEST_CONF_PATH / "conf/logs/rotateLog").string();
    EXPECT_EQ(2, getLogFileNum(pwd, "pingagent.log"));
}

TEST_F(CoreFileWriteTest, rotateLog1) {
    // string pwd=string(getcwd(nullptr, 0));

    fs::path deleteFile = (TEST_CONF_PATH / "conf/logs/rotateLog/pingagent.log");
    // string cmd="rm -f  "+deleteFile;
    // EXPECT_EQ(0,ExecCmd(cmd));
    removeFiles(deleteFile);

    fs::path oldFile = TEST_CONF_PATH / "conf/logs/pingagent.log";
    const fs::path &file = deleteFile; // TEST_CONF_PATH / "conf/logs/rotateLog/pingagent.log";
    // cmd="cp "+oldFile+" "+file;
    // EXPECT_EQ(0,ExecCmd(cmd));
    ASSERT_TRUE(fs::copy_file(oldFile, file));
    fs::path file1 = TEST_CONF_PATH / "conf/logs/rotateLog/pingagent.logt1";
    // string cmd1="cp "+oldFile+" "+file1;
    // EXPECT_EQ(0,ExecCmd(cmd1));
    ASSERT_TRUE(fs::copy_file(oldFile, file1));
    fs::path file2 = TEST_CONF_PATH / "conf/logs/rotateLog/pingagent.logt2";
    // string cmd2="cp "+oldFile+" "+file2;
    // EXPECT_EQ(0,ExecCmd(cmd2));
    ASSERT_TRUE(fs::copy_file(oldFile, file2));
    fs::path file3 = TEST_CONF_PATH / "conf/logs/rotateLog/pingagent.logt3";
    // string cmd3="cp "+oldFile+" "+file3;
    // EXPECT_EQ(0,ExecCmd(cmd3));
    ASSERT_TRUE(fs::copy_file(oldFile, file3));
    fs::path pwd = TEST_CONF_PATH / "conf/logs/rotateLog";
    EXPECT_EQ(4, getLogFileNum(pwd.string(), "pingagent.log"));
    p_shared->m_count = 3;

    EXPECT_EQ(1, p_shared->setup(file.string()));
    string error;
    EXPECT_EQ(5, p_shared->doRecord("test\n", 5, error));
    EXPECT_EQ(0, p_shared->rotateLog());
    EXPECT_EQ(4, getLogFileNum(pwd.string(), "pingagent.log"));
    EXPECT_EQ(5, p_shared->doRecord("test\n", 5, error));
}
