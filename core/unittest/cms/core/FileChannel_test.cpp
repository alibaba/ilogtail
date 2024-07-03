#include <gtest/gtest.h>
#include "core/FileChannel.h"

#include <iostream>

#include "common/Config.h"
#include "common/Chrono.h"
#include "common/FileUtils.h"

using namespace std;
using namespace common;
using namespace argus;

class FileChannelTest : public testing::Test {
protected:

    void SetUp() override {
        p_shared = new FileChannel();
    }

    void TearDown() override {
        Delete(p_shared);
    }

    FileChannel *p_shared = nullptr;
};

TEST_F(FileChannelTest, getMsgContent) {
    OutputMsg msg;
    msg.name = "name";
    msg.exitCode = 3;
    msg.result = "result";
    msg.time = std::chrono::FromSeconds(1544618388);
    string content;
    argus::FileChannel::getMsgContent(msg, content);
    EXPECT_EQ(content, "1544618388|name|3|result\n");
}

#ifndef WIN32
TEST_F(FileChannelTest, parseConf) {
    string conf = R"({"path":"/tmp/test/test.log", "size":1024000,"count":7})";
    FileConf fileConf;
    EXPECT_EQ(0, p_shared->parseConf(conf, fileConf));
    EXPECT_EQ(fileConf.path, "/tmp/test/test.log");
    EXPECT_EQ(fileConf.size, 1024000);
    EXPECT_EQ(fileConf.count, 7);
}

TEST_F(FileChannelTest, confCompare) {
    string conf1 = R"({"path":"/tmp/test/test.log", "size":1024000,"count":7})";
    string conf2 = R"({"size":1024000,"count":7,"path":"/tmp/test/test.log"})";
    FileConf fileConf1;
    FileConf fileConf2;
    EXPECT_EQ(0, p_shared->parseConf(conf1, fileConf1));
    EXPECT_EQ(0, p_shared->parseConf(conf2, fileConf2));
    EXPECT_EQ(0, p_shared->confCompare(fileConf1, fileConf2));
    conf1 = R"({"path":"/tmp/test/test.log", "size":1024000,"count":7})";
    conf2 = R"({"size":1024000,"count":8,"path":"/tmp/test/test.log"})";
    EXPECT_EQ(0, p_shared->parseConf(conf1, fileConf1));
    EXPECT_EQ(0, p_shared->parseConf(conf2, fileConf2));
    EXPECT_EQ(1, p_shared->confCompare(fileConf1, fileConf2));
    conf1 = R"({"path":"/tmp/test/test.log", "size":1024000,"count":7})";
    conf2 = R"({"size":1024000,"count":7,"path":"/tmp/test/test.log1"})";
    EXPECT_EQ(0, p_shared->parseConf(conf1, fileConf1));
    EXPECT_EQ(0, p_shared->parseConf(conf2, fileConf2));
    EXPECT_EQ(2, p_shared->confCompare(fileConf1, fileConf2));
}

TEST_F(FileChannelTest, getOutputFile) {
    string conf1 = R"({"path":"/tmp/test/test.log", "size":1024000,"count":7})";
    FileConf fileConf1;
    EXPECT_EQ(0, p_shared->parseConf(conf1, fileConf1));
    auto pFile1 = std::make_shared<OutputFile>();
    pFile1->fileConf = fileConf1;
    // pFile1->count=1;
    auto pWrite1 = std::make_shared<FileWrite>(fileConf1.size, fileConf1.count);
    pFile1->pFileWrite = pWrite1;
    p_shared->m_outputFileMap.Set(conf1, pFile1);
    //完全一样
    pFile1 = p_shared->getOutputFile(conf1);
    EXPECT_TRUE((bool) pFile1);
    // EXPECT_EQ(2,pFile1->count);

    //一样
    string conf2 = R"({"size":1024000,"count":7,"path":"/tmp/test/test.log"})";
    pFile1 = p_shared->getOutputFile(conf2);
    // EXPECT_NE((OutputFile *)0,pFile1);
    EXPECT_TRUE((bool) pFile1);
    // EXPECT_EQ(3,pFile1->count);

    //冲突
    string conf3 = R"({"size":102400,"count":7,"path":"/tmp/test/test.log"})";
    EXPECT_EQ(nullptr, p_shared->getOutputFile(conf3).get());
    //非json
    string conf4 = R"({"size":102400,"count":7"path":"/tmp/test/jsont/test.log"})";
    EXPECT_EQ(nullptr, p_shared->getOutputFile(conf4).get());
    //创建
    string conf5 = R"({"size":102400,"count":7,"path":"/tmp/test/test1.log"})";
    pFile1 = p_shared->getOutputFile(conf5);
    EXPECT_TRUE((bool) pFile1);
    // EXPECT_EQ(1,pFile1->count);
    //创建失败
    string conf6 = R"({"size":102400,"count":7"path":"/tmp/test/|test1.log|"})";
    EXPECT_EQ(nullptr, p_shared->getOutputFile(conf6).get());
}

TEST_F(FileChannelTest, clearMap) {
    string conf1 = R"({"path":"/tmp/test/test.log", "size":1024000,"count":7})";
    string conf2 = R"({"size":1024000,"count":7,"path":"/tmp/test/test.log"})";
    FileConf fileConf1;
    EXPECT_EQ(0, p_shared->parseConf(conf1, fileConf1));
    auto pFile1 = std::make_shared<OutputFile>();
    pFile1->fileConf = fileConf1;
    // pFile1->count=1;
    auto pWrite1 = std::make_shared<FileWrite>(fileConf1.size, fileConf1.count);
    pFile1->pFileWrite = pWrite1;
    std::chrono::seconds intv{2};
    pFile1->lastTime = std::chrono::system_clock::now() - intv;
    p_shared->m_outputFileMap.Set(conf1, pFile1);
    pFile1 = p_shared->getOutputFile(conf2);
    EXPECT_NE(nullptr, pFile1.get());
    pFile1->lastTime = std::chrono::system_clock::now() - intv;
    p_shared->m_outputFileMap.Set(conf2, pFile1);
    p_shared->m_errorConfMap.Set("test1", std::chrono::system_clock::now() - intv);
    p_shared->m_errorConfMap.Set("test2", std::chrono::system_clock::now() - intv);
    p_shared->clearMap(std::chrono::microseconds{6000*1000});
    EXPECT_EQ(size_t(2), p_shared->m_outputFileMap.Size());
    EXPECT_EQ(size_t(2), p_shared->m_errorConfMap.Size());
    p_shared->clearMap(std::chrono::microseconds{1000*1000});
    EXPECT_EQ(size_t(0), p_shared->m_outputFileMap.Size());
    EXPECT_EQ(size_t(0), p_shared->m_errorConfMap.Size());
}

TEST_F(FileChannelTest, addScriptMessge) {
    const std::chrono::seconds intv{10};
    auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::Now<BySeconds>();
    p_shared->m_errorConfMap.Set("test1", now - intv);
    p_shared->addMessage("test", seconds, 1, "test", "test1", false, "mid");
    EXPECT_LT(p_shared->m_errorConfMap["test1"] - now, std::chrono::seconds{1});
    p_shared->addMessage("test", seconds, 1, "test", "test2", false, "mid");
    EXPECT_LT(p_shared->m_errorConfMap["test2"] - now, std::chrono::seconds{1});
    EXPECT_EQ(size_t(2), p_shared->m_errorConfMap.Size());
    //create map
    string conf1 = R"({"path":"/tmp/test/test.log", "size":1024000,"count":7})";
    string conf2 = R"({"size":1024000,"count":7,"path":"/tmp/test/test.log"})";
    FileConf fileConf1;
    EXPECT_EQ(0, p_shared->parseConf(conf1, fileConf1));
    auto pFile1 = std::make_shared<OutputFile>();
    pFile1->fileConf = fileConf1;
    // pFile1->count=1;
    auto pWrite1 = std::make_shared<FileWrite>(fileConf1.size, fileConf1.count);
    pFile1->pFileWrite = pWrite1;
    pFile1->lastTime = now - intv;
    p_shared->m_outputFileMap.Set(conf1, pFile1);
    p_shared->addMessage("name1", seconds, 1, "result1", conf1, false, "mid");
    p_shared->addMessage("name2", seconds, 2, "result2", conf2, false, "mid");
    EXPECT_LT(p_shared->m_outputFileMap[conf1]->lastTime - now, std::chrono::seconds{1});
    EXPECT_LT(p_shared->m_outputFileMap[conf2]->lastTime - now, std::chrono::seconds{1});
    // EXPECT_EQ(2,p_shared->m_outputFileMap[conf2]->count);
    EXPECT_EQ(size_t(2), p_shared->m_outputMsgList.Size());
    const_cast<size_t &>(p_shared->m_maxMsgNumber) = 2;
    p_shared->addMessage("name1", seconds, 1, "result1", conf1, false, "mid");
    EXPECT_EQ(size_t(2), p_shared->m_outputMsgList.Size());
    EXPECT_EQ(1, p_shared->m_discardCount);
}

TEST_F(FileChannelTest, doRun) {
    auto now = std::chrono::Now<ByMicros>();
    string conf1 = R"({"path":"/tmp/test/test.log1", "size":1024000,"count":7})";
    string conf2 = R"({"path":"/tmp/test/test.log2", "size":1024000,"count":7})";
    p_shared->addMessage("name1", now, 1, "result1-1", conf1, false, "mid");
    p_shared->addMessage("name1", now, 1, "result1-2", conf1, false, "mid");
    p_shared->addMessage("name2", now, 2, "result2-1", conf2, false, "mid");
    p_shared->addMessage("name2", now, 2, "result2-2", conf2, false, "mid");
    p_shared->m_clearIntv = std::chrono::microseconds{500 * 1000};
    p_shared->runIt();
    std::this_thread::sleep_for(std::chrono::microseconds{p_shared->m_clearIntv * 2});
    string content1 = ReadFileContent("/tmp/test/test.log1");
    EXPECT_FALSE(content1.empty());
    string content2 = ReadFileContent("/tmp/test/test.log2");
    EXPECT_FALSE(content2.empty());
    EXPECT_NE(content1.find("name1|1|result1-1"), string::npos);
    EXPECT_NE(content1.find("name1|1|result1-2"), string::npos);
    EXPECT_NE(content2.find("name2|2|result2-1"), string::npos);
    EXPECT_NE(content2.find("name2|2|result2-1"), string::npos);
    EXPECT_EQ(size_t(0), p_shared->m_outputMsgList.Size());
    EXPECT_EQ(size_t(0), p_shared->m_errorConfMap.Size());
    EXPECT_EQ(size_t(0), p_shared->m_outputFileMap.Size());

    boost::system::error_code ec;
    fs::remove("/tmp/test/test.log1", ec);
    fs::remove("/tmp/test/test.log2", ec);
    // string cmd1 = "rm /tmp/test/test.log1";
    // string cmd2 = "rm /tmp/test/test.log2";
    // EXPECT_EQ(0, ExecCmd(cmd1));
    // EXPECT_EQ(0, ExecCmd(cmd2));
}
#endif
