#include <gtest/gtest.h>
#include "cloudMonitor/cloud_channel.h"

#include <iostream>
#include <vector>
#include <memory>

#include "common/SystemUtil.h"
#include "common/StringUtils.h"
#include "common/HttpClient.h"
#include "common/Chrono.h"
#include "common/ChronoLiteral.h"
#include "core/TaskManager.h"
#include "common/SyncQueue.h"
#include "common/Logger.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;
using namespace argus;

class Cms_CloudChannelTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(Cms_CloudChannelTest, ToPayloadString) {
    unique_ptr<CloudChannel> pChannel(new CloudChannel());
    EXPECT_EQ("0.11", pChannel->ToPayloadString(0.1123));
    EXPECT_EQ("0.1", pChannel->ToPayloadString(0.10));
    EXPECT_EQ("0", pChannel->ToPayloadString(0.000));
    EXPECT_EQ("6", pChannel->ToPayloadString(6.000));
    EXPECT_EQ("60", pChannel->ToPayloadString(60.000));
}

TEST_F(Cms_CloudChannelTest, GetSendMetricBody1) {
    unique_ptr<CloudChannel> pChannel(new CloudChannel());
    vector<CommonMetric> metrics;
    for (size_t i = 0; i < pChannel->mMetricSendSize + 1; i++) {
        CommonMetric metric;
        metric.name = "test";
        metrics.push_back(metric);
    }
    vector<string> bodyVector;
    CloudMetricConfig metricConfig;
    EXPECT_EQ(size_t(2), pChannel->GetSendMetricBody(metrics, bodyVector, metricConfig));
    EXPECT_EQ(bodyVector[0].size(), strlen("test 0\n") * pChannel->mMetricSendSize);
    EXPECT_EQ(bodyVector[1], "test 0\n");
}

TEST_F(Cms_CloudChannelTest, GetSendMetricBody2) {
    unique_ptr<CloudChannel> pChannel(new CloudChannel());
    vector<CommonMetric> metrics;
    for (size_t i = 0; i < pChannel->mMetricSendSize - 1; i++) {
        CommonMetric metric;
        metric.name = "test";
        metrics.push_back(metric);
    }
    vector<string> bodyVector;
    CloudMetricConfig metricConfig;
    EXPECT_EQ(size_t(1), pChannel->GetSendMetricBody(metrics, bodyVector, metricConfig));
    EXPECT_EQ(bodyVector[0].size(), strlen("test 0\n") * (pChannel->mMetricSendSize - 1));
}

TEST_F(Cms_CloudChannelTest, GetSendMetricBody3) {
    unique_ptr<CloudChannel> pChannel(new CloudChannel());
    vector<CommonMetric> metrics;
    for (size_t i = 0; i < pChannel->mMetricSendSize; i++) {
        CommonMetric metric;
        metric.name = "test";
        metrics.push_back(metric);
    }
    vector<string> bodyVector;
    CloudMetricConfig metricConfig;
    EXPECT_EQ(size_t(1), pChannel->GetSendMetricBody(metrics, bodyVector, metricConfig));
    EXPECT_EQ(bodyVector[0].size(), strlen("test 0\n") * pChannel->mMetricSendSize);
}

TEST_F(Cms_CloudChannelTest, GetSendMetricBody4) {
    unique_ptr<CloudChannel> pChannel(new CloudChannel());
    vector<CommonMetric> metrics;
    for (int i = 0; i < 1; i++) {
        CommonMetric metric;
        metric.name = "test";
        metric.tagMap["label"] = "a\\b\"c\nd";
        metrics.push_back(metric);
    }
    vector<string> bodyVector;
    CloudMetricConfig metricConfig;
    metricConfig.needTimestamp = true;
    EXPECT_EQ(size_t(1), pChannel->GetSendMetricBody(metrics, bodyVector, metricConfig));
    EXPECT_EQ(bodyVector[0], "test{label=\"a\\\\b\\\"c\\nd\"} 0 0\n");
}

TEST_F(Cms_CloudChannelTest, GetIp) {
    std::string ip = SystemUtil::getMainIpInCloud();
    std::cout << "getMainIpInCloud: " << ip << std::endl;
    ip.clear();
    ip = SystemUtil::getMainIp();
    std::cout << "getMainIp: " << ip << std::endl;
}

TEST_F(Cms_CloudChannelTest, ParseCloudMetricConfig) {
    {
        CloudMetricConfig cfg;
        EXPECT_FALSE(CloudChannel::ParseCloudMetricConfig("{]", cfg));
    }
    {
        CloudMetricConfig cfg;
        EXPECT_FALSE(CloudChannel::ParseCloudMetricConfig("[]", cfg));
    }
    {
        CloudMetricConfig cfg;
        EXPECT_FALSE(CloudChannel::ParseCloudMetricConfig("{}", cfg));
    }
    {
        CloudMetricConfig cfg;
        ASSERT_FALSE(cfg.needTimestamp);
        const char *jsonCfg = R"({"acckeyId":"A","accKeySecretSign":"B","uploadEndpoint":"C","needTimestamp":true})";
        EXPECT_TRUE(CloudChannel::ParseCloudMetricConfig(jsonCfg, cfg));
        EXPECT_TRUE(cfg.needTimestamp);
    }
}

class CloudChannelEx : public CloudChannel {
public:
    SyncQueue<std::string, false> msgQueue{1};

    bool SendMsg(size_t count, const std::string &sendBody) override {
        msgQueue << sendBody;
        return true;
    }

    mutable size_t sendMetricCount = 0;

    int SendMetric(const std::string &body, const CloudMetricConfig &metricConfig) const override {
        sendMetricCount++;
        return 0;
    }
};

struct LoggerStub : public Logger {
    SyncQueue<std::string, Nonblock> &queue_;

    explicit LoggerStub(SyncQueue<std::string, Nonblock> &q) : queue_(q) {}

    void doLoggingStr(LogLevel level, const char *file, int line, const std::string &s) override {
        Logger::doLoggingStr(level, file, line, "*" + s); // 给日志加上一个星号，便于辨认
        queue_ << s;
    }
};

// 无异常 即为OK
TEST_F(Cms_CloudChannelTest, StartWithEmptyMetricItems) {
    CloudChannelEx chan;

    SyncQueue<std::string, Nonblock> queue{10};
    auto *logger = SingletonLogger::swap(new LoggerStub(queue));
    defer(delete SingletonLogger::swap(logger));

    chan.Start();
    std::string target;
    int i;
    constexpr const int maxLine = 5;
    for (i = 0; i < maxLine && std::string::npos == target.find("wait for heartbeat ready"); i++) {
        target = "";
        if (!queue.Take(target, 2_s)) {
            EXPECT_FALSE(true); // 超时了,
            break;
        }
    }
    LogInfo("Loop: {}", i + 1);
    EXPECT_LT(i, maxLine);
}

TEST_F(Cms_CloudChannelTest, doRunWithEmptyQueue) {
    auto *taskManager = SingletonTaskManager::Instance();
    std::shared_ptr<std::vector<MetricItem>> old = taskManager->MetricItems().Get();
    defer(taskManager->SetMetricItems(old));

    taskManager->SetMetricItems(std::make_shared<std::vector<MetricItem>>(1));

    CloudChannelEx chan;

    SyncQueue<std::string, Nonblock> queue{10};
    auto *logger = SingletonLogger::swap(new LoggerStub(queue));
    defer(delete SingletonLogger::swap(logger));

    chan.Start();
    std::string target;
    constexpr const int maxLine = 5;
    int i;
    for (i = 0; i < maxLine && std::string::npos == target.find("is empty"); i++) {
        target = "";
        if (!queue.Take(target, 2_s)) {
            EXPECT_FALSE(true); // 超时了,
            break;
        }
    }
    LogInfo("Loop: {}", i + 1);
    EXPECT_LT(i, maxLine);
}

TEST_F(Cms_CloudChannelTest, addMessage) {
    auto *taskManager = SingletonTaskManager::Instance();
    std::shared_ptr<std::vector<MetricItem>> old = taskManager->MetricItems().Get();
    defer(taskManager->SetMetricItems(old));

    taskManager->SetMetricItems(std::make_shared<std::vector<MetricItem>>(1));

    CloudChannelEx chan;
    chan.mMaxMsgSize = 1;

    chan.Start();

    for (int i = 0; i < 2; i++) {
        chan.addMessage("", std::chrono::system_clock::now(), 0, "OK", "{}", true, "mid:1");
    }
    EXPECT_TRUE(chan.msgQueue.Wait()); // 等待SendMsg
}

TEST_F(Cms_CloudChannelTest, AddCommonMetrics) {
    CloudChannelEx chan;

    std::vector<CommonMetric> metrics;
    EXPECT_EQ(-1, chan.AddCommonMetrics(metrics, "[]"));
    metrics.resize(1);
    EXPECT_EQ(-2, chan.AddCommonMetrics(metrics, "[]"));
    EXPECT_EQ(-2, chan.AddCommonMetrics(metrics, "{}"));

    NodeItem old;
    SingletonTaskManager::Instance()->GetNodeItem(old);
    defer(SingletonTaskManager::Instance()->SetNodeItem(old));

    NodeItem emptyNode;
    SingletonTaskManager::Instance()->SetNodeItem(emptyNode);
    const char *szConfJson = R"XX({
"acckeyId": "1",
"accKeySecretSign": "2",
"uploadEndpoint": "3",
})XX";
    EXPECT_EQ(-3, chan.AddCommonMetrics(metrics, szConfJson));

    NodeItem nonEmptyNode;
    nonEmptyNode.instanceId = "i-xxxx";
    SingletonTaskManager::Instance()->SetNodeItem(nonEmptyNode);

    EXPECT_EQ(0, chan.AddCommonMetrics(metrics, szConfJson));
    EXPECT_EQ(size_t(1), chan.sendMetricCount);
}

TEST_F(Cms_CloudChannelTest, GetSendMetricHeader) {
    CloudChannel chan;
    chan.mNodeItem.instanceId = "i-123";

    CloudMetricConfig metricConfig;
    metricConfig.secureToken = "abc";

    std::map<std::string, std::string> header;
    EXPECT_EQ(0, chan.GetSendMetricHeader("{}", metricConfig, header));
    EXPECT_FALSE(header.empty());
}

static void doTestSendMetric(const char *name, int expect, const std::function<CloudChannel::THttpPost> &post) {
    std::cout << std::string(30, '-') << name << std::string(90, '-') << std::endl;
    CloudChannel chan(post);
    chan.mNodeItem.instanceId = "i-123";

    CloudMetricConfig metricConfig;
    metricConfig.secureToken = "abc";
    metricConfig.uploadEndpoint = "https://mock-metrichub.com";

    EXPECT_EQ(expect, chan.SendMetric("{}", metricConfig));
}

TEST_F(Cms_CloudChannelTest, SendMetric) {
    doTestSendMetric("HttpClient:-1", -1, [&](const HttpRequest &request, HttpResponse &resp) {
        return -1;
    });

    doTestSendMetric("HttpClient:404", -1, [&](const HttpRequest &request, HttpResponse &resp) {
        resp.resCode = (int) HttpStatus::NotFound;
        return HttpClient::Success;
    });

    doTestSendMetric("HttpClient:OK,[]", -1, [&](const HttpRequest &request, HttpResponse &resp) {
        resp.resCode = (int) HttpStatus::OK;
        resp.result = "[]";  // not json object
        return HttpClient::Success;
    });

    doTestSendMetric("HttpClient:OK,NoResult", 0, [&](const HttpRequest &request, HttpResponse &resp) {
        resp.resCode = (int) HttpStatus::OK;
        resp.result = "";
        return HttpClient::Success;
    });

    doTestSendMetric("HttpClient:OK,Success", 0, [&](const HttpRequest &request, HttpResponse &resp) {
        resp.resCode = (int) HttpStatus::OK;
        resp.result = R"XX({"code":"Success"})XX";  // not json object
        return HttpClient::Success;
    });
}

TEST_F(Cms_CloudChannelTest, SendMsg00) {
    CloudChannel chan;
    EXPECT_TRUE(chan.SendMsg(0, ""));
}

TEST_F(Cms_CloudChannelTest, SendMsg01) {
    std::string actualUrl;
    auto post = [&](const HttpRequest &request, HttpResponse &resp) {
        actualUrl = request.url;
        resp.resCode = (int) HttpStatus::OK;
        resp.result = R"XX({"code":"Success"})XX";  // not json object
        return (int) HttpClient::Success;
    };
    CloudChannel chan(post);
    chan.mCloudAgentInfo.accessKeyId = "A";
    chan.mCloudAgentInfo.accessSecret = "B";
    chan.mCurrentMetricItemTryTimes = 0; // 重试达3次时切换上报地址
    chan.mMetricItems.resize(2);
    chan.mMetricItems[0].url = "https://test-metrichub-00.com";
    chan.mMetricItems[0].gzip = false;
    // chan.mMetricItems[1].url = "https://test-metrichub-01.com";
    // chan.mMetricItems[1].gzip = true;

    EXPECT_TRUE(chan.SendMsg(0, std::string(50, '-')));
    EXPECT_NE(std::string::npos, HasPrefix(actualUrl, chan.mMetricItems[0].url));
}

TEST_F(Cms_CloudChannelTest, SendMsg02) {
    std::string actualUrl;
    auto post = [&](const HttpRequest &request, HttpResponse &resp) {
        actualUrl = request.url;
        resp.resCode = (int) HttpStatus::NotFound;
        return -1;
    };
    CloudChannel chan(post);
    chan.mCloudAgentInfo.accessKeyId = "A";
    chan.mCloudAgentInfo.accessSecret = "B";
    chan.mCurrentMetricItemTryTimes = 3; // 重试达3次时切换上报地址
    chan.mMetricItems.resize(2);
    // chan.mMetricItems[0].url = "https://test-metrichub-00.com";
    // chan.mMetricItems[0].gzip = false;
    chan.mMetricItems[1].url = "https://test-metrichub-01.com";
    chan.mMetricItems[1].gzip = true;

    EXPECT_FALSE(chan.SendMsg(0, ""));
    EXPECT_NE(std::string::npos, HasPrefix(actualUrl, chan.mMetricItems[1].url));
}

TEST_F(Cms_CloudChannelTest, ToPayloadMetricData01) {
    CloudChannel chan;
    EXPECT_EQ("", chan.ToPayloadMetricData(MetricData{}, std::chrono::system_clock::now()));
}

TEST_F(Cms_CloudChannelTest, ToPayloadMetricData02) {
    CloudChannel chan;
    chan.mNodeItem.instanceId = "i-123";
    chan.mNodeItem.aliUid = "1234567";

    MetricData metricData;
    metricData.tagMap["metricName"] = "detect";
    metricData.tagMap["ns"] = "loss";
    metricData.tagMap["targetIP"] = "127.0.0.1";
    metricData.valueMap["metricValue"] = 2.2;
    metricData.valueMap["jumps"] = 23;

    const int64_t seconds = 1706368026;;
    std::string payload = chan.ToPayloadMetricData(metricData, std::chrono::FromSeconds(seconds));
    LogInfo("Payload: {}", TrimRightSpace(payload));
    const char *szExpect = "detect 1706368026000 2.2 ns=loss targetIP=127.0.0.1 jumps=23 instanceId=i-123 userId=1234567\n";
    EXPECT_EQ(payload, szExpect);
}

TEST_F(Cms_CloudChannelTest, ToPayloadMsgs_SkipInvalid) {
    CloudChannel chan;

    std::vector<CloudMsg> cloudMsgs;
    cloudMsgs.resize(1);
    cloudMsgs[0].name = "cpu";

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));
    std::string body;
    EXPECT_EQ(size_t(0), chan.ToPayloadMsgs(cloudMsgs, body));
    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("skip invalid"));
}

TEST_F(Cms_CloudChannelTest, ToPayloadMsgs_SkipEmpty) {
    CloudMsg msg; // empty data
    msg.name = "cpu";
    msg.timestamp = std::chrono::system_clock::now();
    CollectData data;
    data.moduleName = msg.name;
    ModuleData::convertCollectDataToString(data, msg.msg);
    std::vector<CloudMsg> cloudMsgs{msg};

    CloudChannel chan;

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));
    std::string body;
    EXPECT_EQ(size_t(0), chan.ToPayloadMsgs(cloudMsgs, body));
    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("skip empty"));
}

TEST_F(Cms_CloudChannelTest, ToPayloadMsgs_ZeroLine) {
    CloudMsg msg; // empty data
    msg.name = "cpu";
    msg.timestamp = std::chrono::system_clock::now();
    {
        CollectData data;
        data.moduleName = msg.name;
        data.dataVector.resize(1);  // invalid
        ModuleData::convertCollectDataToString(data, msg.msg);
    }
    std::vector<CloudMsg> cloudMsgs{msg};

    CloudChannel chan;

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));
    std::string body;
    EXPECT_EQ(size_t(0), chan.ToPayloadMsgs(cloudMsgs, body));
    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("skip empty invalid"));
}

TEST_F(Cms_CloudChannelTest, ToPayloadMsgs_Success) {
    CloudMsg msg; // empty data
    msg.name = "cpu";
    msg.timestamp = std::chrono::FromSeconds(1706367779);
    {
        CollectData data;
        data.moduleName = msg.name;
        data.dataVector.resize(1);

        MetricData &metricData = data.dataVector[0];
        metricData.tagMap["metricName"] = "cpu_utilization";
        metricData.tagMap["ns"] = "acs_host";
        metricData.tagMap["targetIP"] = "127.0.0.1";
        metricData.valueMap["metricValue"] = 2.2;
        metricData.valueMap["jumps"] = 23;

        ModuleData::convertCollectDataToString(data, msg.msg);
    }
    std::vector<CloudMsg> cloudMsgs{msg};

    CloudChannel chan;
    chan.mNodeItem.instanceId = "i-123";
    chan.mNodeItem.aliUid = "1234567";

    std::string body;
    EXPECT_EQ(size_t(1), chan.ToPayloadMsgs(cloudMsgs, body));
    std::cout << body;
    const char *szExpect = "cpu_utilization 1706367779000 2.2 ns=acs_host targetIP=127.0.0.1 jumps=23 instanceId=i-123 userId=1234567\n";
    EXPECT_EQ(body, szExpect);
}
