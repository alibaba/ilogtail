#include <gtest/gtest.h>
#include "core/http_metric_collect.h"
#include "core/http_metric_listen.h"
#include <thread>
#include "common/HttpClient.h"
#include "common/FileUtils.h"
#include "common/CPoll.h"
#include "common/UnitTestEnv.h"

#include "core/task_monitor.h"

using namespace common;
using namespace argus;
using namespace std;

class HttpMetricCollectTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        // 该测试用例完成之后应当清理线程，防止污染其他测试用例
        SingletonHttpMetricCollect::destroyInstance();
        std::this_thread::sleep_for(100_ms);
    }
};

TEST_F(HttpMetricCollectTest, Test) {
    auto pCollect = std::make_shared<HttpMetricCollect>();
    string header = "POST /metrics/test_tag/test?test=hello";
    map<string, string> tagMap;
    string errorMsg;
    string type;
    EXPECT_TRUE(pCollect->ParseHeader(header, type, tagMap, errorMsg));
    EXPECT_EQ(type, "metrics");
    EXPECT_EQ(tagMap.size(), size_t(1));
    EXPECT_EQ(tagMap["test_tag"], "test");
    header = "POST /shennong/test_tag/test/helle/world/test";
    tagMap.clear();
    type.clear();
    EXPECT_FALSE(pCollect->ParseHeader(header, type, tagMap, errorMsg));
    header = "POST /shennong/test_tag/test/helle/world";
    tagMap.clear();
    type.clear();
    EXPECT_TRUE(pCollect->ParseHeader(header, type, tagMap, errorMsg));
    EXPECT_EQ(tagMap.size(), size_t(2));
    EXPECT_EQ(type, "shennong");
    header = "POST /shennong/test_tag@base64/dGVzdA==";
    tagMap.clear();
    type.clear();
    EXPECT_TRUE(pCollect->ParseHeader(header, type, tagMap, errorMsg));
    EXPECT_EQ(tagMap.size(), size_t(1));
    EXPECT_EQ(tagMap["test_tag"], "test");
    EXPECT_EQ(type, "shennong");
}

static void SendPostMetricRequest(int port) {
    HttpRequest request;
    HttpResponse response;
    request.url = fmt::format("localhost:{}/metrics/type/alimonitor", port);
    request.body = "test{tag1=\"test_tag1\"} 11.0";
    HttpClient::HttpPost(request, response);
}

static void SendPostShennongRequest(int port) {
    HttpRequest request;
    HttpResponse response;
    request.url = fmt::format("localhost:{}/shennong/type/shennong", port);
    request.body = "test{tag2=\"test_tag2\"} 22.0";
    HttpClient::HttpPost(request, response);
}

TEST_F(HttpMetricCollectTest, doRun) {
    string taskJson;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "httpReceiveTask1.json").string(), taskJson);

    TaskMonitor pTask;
    pTask.ParseHttpReceiveTaskInfo(taskJson);

    StartGlobalPoll();
    SingletonHttpMetricCollect::Instance()->Start();
    auto listen = SingletonPoll::Instance()->find<HttpMetricListen>();
    ASSERT_NE(listen.get(), nullptr);
    EXPECT_EQ("127.0.0.1", listen->getListenIp());

    thread d1(SendPostMetricRequest, listen->getListenPort());
    thread d2(SendPostShennongRequest, listen->getListenPort());

    d1.join();
    d2.join();

    SingletonHttpMetricCollect::destroyInstance();
}
