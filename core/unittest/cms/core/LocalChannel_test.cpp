//
// Created by 韩呈杰 on 2024/1/15.
//
#include <gtest/gtest.h>
#include "core/LocalChannel.h"

#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "common/ScopeGuard.h"
#include "common/ChronoLiteral.h"

using namespace argus;

TEST(CoreLocalChannelTest, addMessage) {
    LocalChannel lc;
    lc.addMessage("1", std::chrono::system_clock::now(), 0, "", "", true, "");
    EXPECT_EQ(lc.msgList.size(), size_t(1));
    EXPECT_EQ(lc.m_discardCount, size_t(0));

    lc.m_maxCount = 1;
    lc.addMessage("2", std::chrono::system_clock::now(), 0, "", "", true, ""); // 失败
    EXPECT_EQ(lc.msgList.size(), size_t(1));
    EXPECT_EQ(lc.m_discardCount, size_t(1));
    EXPECT_EQ(lc.msgList.front().moduleName, "2");

    // result.size > mRecordSize直接丢弃
    lc.mRecordSize = 0;
    lc.addMessage("3", std::chrono::system_clock::now(), 0, "1", "", true, ""); // 失败
    EXPECT_EQ(lc.msgList.size(), size_t(1));
    EXPECT_EQ(lc.m_discardCount, size_t(1));
    EXPECT_EQ(lc.msgList.front().moduleName, "2");
}

TEST(CoreLocalChannelTest, rotateFile) {
    auto removeFile = [](const std::string &file) {
        boost::system::error_code ec;
        if (fs::exists(file)) {
            fs::remove(file, ec);
            if (ec.failed()) {
                LogError("remove <{}> failed: {}", file, ec.message());
            } else {
                LogInfo("removeFile ok: {}", file);
            }
        } else {
            LogInfo("not exist: {}", file);
        }
    };

    fs::path file = TEST_CONF_PATH / "conf" / "tmp" / "LocalChannel" / "rotateFile.tmp";
    removeFile(file.string());

    defer(removeFile(file.string()));
    EXPECT_FALSE(LocalChannel::shouldRotate(file.string(), std::chrono::system_clock::time_point{}));

    boost::system::error_code ec;
    fs::create_directories(file.parent_path(), ec);
    ASSERT_TRUE(fs::exists(file.parent_path()));

    LocalChannel lc;
    EXPECT_EQ(0, lc.outPut2File("hello world", std::chrono::system_clock::now(), file.string()));

    auto now = std::chrono::system_clock::now() - 25_h;
    EXPECT_TRUE(LocalChannel::shouldRotate(file.string(), now));

    std::string newFile = LocalChannel::rotateFile(file.string(), now);
    EXPECT_FALSE(newFile.empty());
    EXPECT_TRUE(fs::exists(newFile));
    defer(removeFile(newFile));

    now = std::chrono::system_clock::now();
    lc.m_deleteTimestamp = now;
    EXPECT_EQ(0, lc.removeOldFiles(newFile, now + 1_h));
    EXPECT_EQ(1, lc.removeOldFiles(newFile, now + 100 * 24_h));
}

TEST(CoreLocalChannelTest, dealMsg) {
    struct MockLocalChannel : LocalChannel {
        std::string fname;
        int outPut2File(const std::string &fileContent,
                        const std::chrono::system_clock::time_point &timeStamp,
                        const std::string &fileName) override {
            fname = fileName;
            EXPECT_EQ(*fileContent.rbegin(), '\n');
            std::cout << "<" << fileName << ">:" << std::endl;
            std::cout << "[" << timeStamp << "]: " << fileContent;
            return 0;
        }
    };

    LocalFileMsg msg;
    msg.moduleName = "sayHi";
    msg.msg = "hello world";
    msg.timeStamp = std::chrono::system_clock::now();

    MockLocalChannel lc;
    lc.dealMsg(msg);
    EXPECT_FALSE(lc.fname.empty());
}

TEST(CoreLocalChannelTest, removeOldFiles) {
    LocalChannel lc;
    lc.m_deleteTimestamp = std::chrono::system_clock::now();
    EXPECT_EQ(-1, lc.removeOldFiles("", lc.m_deleteTimestamp - 1_h));

    lc.m_deleteTimestamp -= 24_h;
    EXPECT_EQ(-2, lc.removeOldFiles("hello", std::chrono::system_clock::now()));
}
